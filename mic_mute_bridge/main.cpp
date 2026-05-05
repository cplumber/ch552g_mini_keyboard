#define WIN32_LEAN_AND_MEAN
#define INITGUID

#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <cstdint>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <condition_variable>
#include <string>
#include <mutex>
#include <thread>
#include <vector>

namespace
{
constexpr int kHotkeyId = 1;
bool g_windowless = false;
constexpr wchar_t kSingletonMutexName[] = L"Local\\MicMuteBridgeSingleton";
constexpr wchar_t kStopEventName[] = L"Local\\MicMuteBridgeStopAll";
HANDLE g_stop_event = nullptr;

std::string timestamp_now()
{
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto tt = system_clock::to_time_t(now);

    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &tt);
#else
    localtime_r(&tt, &tm_buf);
#endif

    const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    char buffer[32];
    std::snprintf(buffer,
                  sizeof(buffer),
                  "%02d:%02d:%02d.%03d",
                  tm_buf.tm_hour,
                  tm_buf.tm_min,
                  tm_buf.tm_sec,
                  static_cast<int>(ms.count()));
    return buffer;
}

void log_info(const char *fmt, ...)
{
    if (g_windowless)
    {
        return;
    }

    va_list args;
    va_start(args, fmt);
    std::fprintf(stdout, "[%s] ", timestamp_now().c_str());
    std::vfprintf(stdout, fmt, args);
    std::fprintf(stdout, "\n");
    std::fflush(stdout);
    va_end(args);
}

void log_error(const char *fmt, ...)
{
    if (g_windowless)
    {
        return;
    }

    va_list args;
    va_start(args, fmt);
    std::fprintf(stderr, "[%s] ", timestamp_now().c_str());
    std::vfprintf(stderr, fmt, args);
    std::fprintf(stderr, "\n");
    std::fflush(stderr);
    va_end(args);
}

template <typename T>
void release_com(T *&ptr)
{
    if (ptr)
    {
        ptr->Release();
        ptr = nullptr;
    }
}

class ComInit
{
public:
    ComInit()
    {
        hr_ = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    }

    ~ComInit()
    {
        if (SUCCEEDED(hr_))
        {
            CoUninitialize();
        }
    }

    HRESULT hr() const
    {
        return hr_;
    }

private:
    HRESULT hr_ = E_FAIL;
};

class AudioController
{
public:
    bool init()
    {
        IMMDevice *device = nullptr;

        HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator,
                                      nullptr,
                                      CLSCTX_ALL,
                                      IID_PPV_ARGS(&enumerator_));
        if (FAILED(hr))
        {
            log_error("CoCreateInstance(MMDeviceEnumerator) failed: 0x%08lx",
                      static_cast<unsigned long>(hr));
            return false;
        }

        hr = enumerator_->GetDefaultAudioEndpoint(eCapture, eCommunications, &device);
        if (FAILED(hr))
        {
            log_error("GetDefaultAudioEndpoint failed: 0x%08lx",
                      static_cast<unsigned long>(hr));
            return false;
        }

        hr = device->Activate(IID_IAudioEndpointVolume,
                              CLSCTX_ALL,
                              nullptr,
                              reinterpret_cast<void **>(&endpoint_));
        device->Release();
        if (FAILED(hr))
        {
            log_error("Activate(IAudioEndpointVolume) failed: 0x%08lx",
                      static_cast<unsigned long>(hr));
            return false;
        }

        return true;
    }

    bool get_mute(bool &muted)
    {
        if (!endpoint_)
        {
            return false;
        }

        BOOL current = FALSE;
        HRESULT hr = endpoint_->GetMute(&current);
        if (FAILED(hr))
        {
            log_error("GetMute failed: 0x%08lx", static_cast<unsigned long>(hr));
            return false;
        }

        muted = (current != FALSE);
        return true;
    }

    bool toggle_mic(bool &muted_now)
    {
        if (!endpoint_)
        {
            return false;
        }

        bool muted = false;
        if (!get_mute(muted))
        {
            return false;
        }

        const BOOL next = muted ? FALSE : TRUE;
        HRESULT hr = endpoint_->SetMute(next, nullptr);
        if (FAILED(hr))
        {
            log_error("SetMute failed: 0x%08lx", static_cast<unsigned long>(hr));
            return false;
        }

        muted_now = (next != FALSE);
        log_info("microphone muted: %s", muted_now ? "true" : "false");
        return true;
    }

    ~AudioController()
    {
        release_com(endpoint_);
        release_com(enumerator_);
    }

private:
    IMMDeviceEnumerator *enumerator_ = nullptr;
    IAudioEndpointVolume *endpoint_ = nullptr;
};

class HidFeedback
{
public:
    void configure(uint16_t vendor_id, uint16_t product_id)
    {
        vendor_id_ = vendor_id;
        product_id_ = product_id;
    }

    bool open()
    {
        return ensure_open();
    }

    bool is_open() const
    {
        return handle_ != INVALID_HANDLE_VALUE;
    }

    bool send_mic_state(bool muted)
    {
        if (!ensure_open())
        {
            return false;
        }

        const BYTE value = static_cast<BYTE>(muted ? 1 : 0);
        if (!send_feature_report(kMicStateReportId,
                                 value,
                                 muted ? "muted/yellow" : "live/green"))
        {
            return false;
        }

        return true;
    }

    ~HidFeedback()
    {
        close();
    }

private:
    static constexpr BYTE kMicStateReportId = 5;

    bool send_feature_report(BYTE report_id, BYTE value, const char *label)
    {
        BYTE report[2] = {report_id, value};
        log_info("HID feature write start: %s", label);
        if (!HidD_SetFeature(handle_, report, sizeof(report)))
        {
            log_error("HidD_SetFeature failed: %lu",
                      static_cast<unsigned long>(GetLastError()));
            close();
            return false;
        }
        log_info("HID feature write done: %s", label);
        return true;
    }

    bool ensure_open()
    {
        if (handle_ != INVALID_HANDLE_VALUE)
        {
            return true;
        }

        GUID hid_guid;
        HidD_GetHidGuid(&hid_guid);

        HDEVINFO dev_info = SetupDiGetClassDevsW(&hid_guid,
                                                 nullptr,
                                                 nullptr,
                                                 DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if (dev_info == INVALID_HANDLE_VALUE)
        {
            log_error("SetupDiGetClassDevsW failed: %lu",
                      static_cast<unsigned long>(GetLastError()));
            return false;
        }

        bool found = false;
        for (DWORD index = 0;; ++index)
        {
            SP_DEVICE_INTERFACE_DATA interface_data = {};
            interface_data.cbSize = sizeof(interface_data);

            if (!SetupDiEnumDeviceInterfaces(dev_info,
                                             nullptr,
                                             &hid_guid,
                                             index,
                                             &interface_data))
            {
                break;
            }

            DWORD required_size = 0;
            SetupDiGetDeviceInterfaceDetailW(dev_info,
                                             &interface_data,
                                             nullptr,
                                             0,
                                             &required_size,
                                             nullptr);
            if (required_size == 0)
            {
                continue;
            }

            std::vector<BYTE> detail_buffer(required_size);
            auto *detail =
                reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W *>(detail_buffer.data());
            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

            if (!SetupDiGetDeviceInterfaceDetailW(dev_info,
                                                  &interface_data,
                                                  detail,
                                                  required_size,
                                                  nullptr,
                                                  nullptr))
            {
                continue;
            }

            HANDLE candidate = CreateFileW(detail->DevicePath,
                                           GENERIC_READ | GENERIC_WRITE,
                                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                                           nullptr,
                                           OPEN_EXISTING,
                                           FILE_ATTRIBUTE_NORMAL,
                                           nullptr);
            if (candidate == INVALID_HANDLE_VALUE)
            {
                continue;
            }

            HIDD_ATTRIBUTES attributes = {};
            attributes.Size = sizeof(attributes);
            if (!HidD_GetAttributes(candidate, &attributes))
            {
                CloseHandle(candidate);
                continue;
            }

            if (attributes.VendorID == vendor_id_ &&
                attributes.ProductID == product_id_)
            {
                handle_ = candidate;
                found = true;
                break;
            }

            CloseHandle(candidate);
        }

        SetupDiDestroyDeviceInfoList(dev_info);

        if (!found)
        {
            log_error("HID feedback device not found for VID/PID %04x:%04x",
                      static_cast<unsigned>(vendor_id_),
                      static_cast<unsigned>(product_id_));
        }

        return found;
    }

    void close()
    {
        if (handle_ != INVALID_HANDLE_VALUE)
        {
            CloseHandle(handle_);
            handle_ = INVALID_HANDLE_VALUE;
        }
    }

    HANDLE handle_ = INVALID_HANDLE_VALUE;
    uint16_t vendor_id_ = 0;
    uint16_t product_id_ = 0;
};

class MicStateSyncWorker
{
public:
    bool start(uint16_t vendor_id, uint16_t product_id)
    {
        feedback_.configure(vendor_id, product_id);
        stop_ = false;
        worker_ = std::thread(&MicStateSyncWorker::run, this);
        return true;
    }

    void push_state(bool muted)
    {
        if (send_state(muted))
        {
            log_info("LED sync state: %s", muted ? "muted/yellow" : "live/green");
        }
    }

    void request_refresh()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            wake_now_ = true;
        }
        cv_.notify_one();
    }

    void stop()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;
        }
        cv_.notify_one();
        if (worker_.joinable())
        {
            worker_.join();
        }
    }

    ~MicStateSyncWorker()
    {
        stop();
    }

private:
    bool send_state(bool muted)
    {
        std::lock_guard<std::mutex> lock(feedback_mutex_);
        if (!feedback_.send_mic_state(muted))
        {
            return false;
        }

        std::lock_guard<std::mutex> state_lock(state_mutex_);
        last_sent_state_ = muted;
        have_last_sent_state_ = true;
        return true;
    }

    void run()
    {
        ComInit com;
        if (FAILED(com.hr()))
        {
            log_error("Sync worker COM initialization failed: 0x%08lx",
                      static_cast<unsigned long>(com.hr()));
            return;
        }

        AudioController audio;
        if (!audio.init())
        {
            log_error("Sync worker could not open Core Audio endpoint");
            return;
        }

        while (true)
        {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait_for(lock, std::chrono::seconds(2), [&] { return stop_ || wake_now_; });

                if (stop_)
                {
                    break;
                }

                wake_now_ = false;
            }

            bool current_state = false;
            if (!audio.get_mute(current_state))
            {
                continue;
            }

            bool should_send = false;
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                should_send = !have_last_sent_state_ || current_state != last_sent_state_;
            }

            if (should_send)
            {
                send_state(current_state);
                log_info("LED sync state: %s",
                         current_state ? "muted/yellow" : "live/green");
            }
        }
    }

    HidFeedback feedback_;
    std::mutex mutex_;
    std::mutex feedback_mutex_;
    std::mutex state_mutex_;
    std::condition_variable cv_;
    std::thread worker_;
    bool stop_ = false;
    bool wake_now_ = false;
    bool last_sent_state_ = false;
    bool have_last_sent_state_ = false;
};
struct Options
{
    bool toggle_once = false;
    bool windowless = false;
    bool stop_all = false;
};

bool stop_all_instances()
{
    HANDLE stop_event = OpenEventW(EVENT_MODIFY_STATE, FALSE, kStopEventName);
    if (!stop_event)
    {
        log_error("OpenEventW failed: %lu",
                  static_cast<unsigned long>(GetLastError()));
        return false;
    }

    const BOOL ok = SetEvent(stop_event);
    CloseHandle(stop_event);
    if (!ok)
    {
        log_error("SetEvent failed: %lu",
                  static_cast<unsigned long>(GetLastError()));
        return false;
    }

    return true;
}

Options parse_args(int argc, char **argv)
{
    Options options;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--toggle" || arg == "--once")
        {
            options.toggle_once = true;
        }
        else if (arg == "--windowless")
        {
            options.windowless = true;
        }
        else if (arg == "--stop-all")
        {
            options.stop_all = true;
        }
    }
    return options;
}
} // namespace

int main(int argc, char **argv)
{
    const Options options = parse_args(argc, argv);

    if (options.stop_all)
    {
        stop_all_instances();
        return 0;
    }

    HANDLE singleton_mutex = CreateMutexW(nullptr, TRUE, kSingletonMutexName);
    if (!singleton_mutex)
    {
        log_error("CreateMutexW failed: %lu",
                  static_cast<unsigned long>(GetLastError()));
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        log_error("Mic mute bridge is already running.");
        CloseHandle(singleton_mutex);
        return 1;
    }

    g_stop_event = CreateEventW(nullptr, TRUE, FALSE, kStopEventName);
    if (!g_stop_event)
    {
        log_error("CreateEventW failed: %lu",
                  static_cast<unsigned long>(GetLastError()));
        CloseHandle(singleton_mutex);
        return 1;
    }

    ResetEvent(g_stop_event);

    if (options.windowless)
    {
        g_windowless = true;
        FreeConsole();
    }
    else
    {
        SetConsoleTitleA("Mic Mute Bridge");
    }

    ComInit com;
    if (FAILED(com.hr()))
    {
        log_error("COM initialization failed: 0x%08lx",
                  static_cast<unsigned long>(com.hr()));
        return 1;
    }

    AudioController audio;
    if (!audio.init())
    {
        return 1;
    }

    HidFeedback feedback;
    feedback.configure(0x1209, 0xC55D);

    if (options.toggle_once)
    {
        bool muted_now = false;
        if (!audio.toggle_mic(muted_now))
        {
            return 1;
        }
        if (!feedback.open())
        {
            return 1;
        }
        if (feedback.send_mic_state(muted_now))
        {
            log_info("LED sync state: %s", muted_now ? "muted/yellow" : "live/green");
        }
        return 0;
    }

    MicStateSyncWorker sync_worker;
    sync_worker.start(0x1209, 0xC55D);

    if (!RegisterHotKey(nullptr, kHotkeyId, MOD_NOREPEAT, VK_F24))
    {
        log_error("RegisterHotKey(VK_F24) failed: %lu",
                  static_cast<unsigned long>(GetLastError()));
        CloseHandle(g_stop_event);
        CloseHandle(singleton_mutex);
        return 1;
    }

    log_info("Mic mute bridge running. Waiting for F24.");

    MSG msg;
    while (true)
    {
        const DWORD wait = MsgWaitForMultipleObjects(1,
                                                      &g_stop_event,
                                                      FALSE,
                                                      INFINITE,
                                                      QS_ALLINPUT);

        if (wait == WAIT_OBJECT_0)
        {
            break;
        }

        if (wait != WAIT_OBJECT_0 + 1)
        {
            log_error("MsgWaitForMultipleObjects failed: %lu",
                      static_cast<unsigned long>(GetLastError()));
            break;
        }

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                break;
            }

            if (msg.message == WM_HOTKEY && msg.wParam == kHotkeyId)
            {
                bool muted_now = false;
                if (audio.toggle_mic(muted_now))
                {
                    sync_worker.request_refresh();
                }
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    SetEvent(g_stop_event);
    sync_worker.stop();
    UnregisterHotKey(nullptr, kHotkeyId);
    CloseHandle(g_stop_event);
    CloseHandle(singleton_mutex);
    return 0;
}
