#define WIN32_LEAN_AND_MEAN

#include "macropad_hid.h"

#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <vector>
#include <cctype>

namespace
{
constexpr uint8_t kMicStateReportId = 5;
constexpr uint8_t kConfigReportId = 6;

bool uid_matches(const std::array<uint8_t, 9> &response, const std::string &wanted)
{
    if (wanted.size() != 10) return false;
    static const char hex[] = "0123456789ABCDEF";
    for (unsigned i = 0; i < 5; ++i)
    {
        const char high = hex[response[3 + i] >> 4];
        const char low = hex[response[3 + i] & 0x0F];
        if (static_cast<char>(std::toupper(static_cast<unsigned char>(wanted[i * 2]))) != high ||
            static_cast<char>(std::toupper(static_cast<unsigned char>(wanted[i * 2 + 1]))) != low)
            return false;
    }
    return true;
}

void set_error(std::string *error, const char *message)
{
    if (error)
    {
        *error = message;
    }
}

void set_win32_error(std::string *error, const char *operation)
{
    if (error)
    {
        *error = std::string(operation) + " failed: " + std::to_string(GetLastError());
    }
}
} // namespace

MacropadHid::MacropadHid() = default;

MacropadHid::~MacropadHid()
{
    close();
}

void MacropadHid::configure(uint16_t vendor_id, uint16_t product_id)
{
    vendor_id_ = vendor_id;
    product_id_ = product_id;
}

void MacropadHid::set_target_uid(const std::string &uid)
{
    target_uid_ = uid;
}

bool MacropadHid::list_identities(std::vector<MacropadBoardIdentity> &identities,
                                  std::string *error) const
{
    identities.clear();
    GUID hid_guid;
    HidD_GetHidGuid(&hid_guid);
    HDEVINFO dev_info = SetupDiGetClassDevsW(&hid_guid, nullptr, nullptr,
                                             DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (dev_info == INVALID_HANDLE_VALUE)
    {
        set_win32_error(error, "SetupDiGetClassDevsW");
        return false;
    }

    for (DWORD index = 0;; ++index)
    {
        SP_DEVICE_INTERFACE_DATA interface_data = {};
        interface_data.cbSize = sizeof(interface_data);
        if (!SetupDiEnumDeviceInterfaces(dev_info, nullptr, &hid_guid, index,
                                         &interface_data)) break;
        DWORD required_size = 0;
        SetupDiGetDeviceInterfaceDetailW(dev_info, &interface_data, nullptr, 0,
                                         &required_size, nullptr);
        if (required_size == 0) continue;
        std::vector<BYTE> detail_buffer(required_size);
        auto *detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W *>(detail_buffer.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(dev_info, &interface_data, detail,
                                              required_size, nullptr, nullptr)) continue;
        HANDLE candidate = CreateFileW(detail->DevicePath, GENERIC_READ | GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (candidate == INVALID_HANDLE_VALUE) continue;
        HIDD_ATTRIBUTES attributes = {};
        attributes.Size = sizeof(attributes);
        bool matching = HidD_GetAttributes(candidate, &attributes) &&
                        attributes.VendorID == vendor_id_ && attributes.ProductID == product_id_;
        if (matching)
        {
            PHIDP_PREPARSED_DATA preparsed = nullptr;
            HIDP_CAPS caps = {};
            matching = HidD_GetPreparsedData(candidate, &preparsed) &&
                       HidP_GetCaps(preparsed, &caps) == HIDP_STATUS_SUCCESS &&
                       caps.UsagePage == 0xFF00 && caps.Usage == 2 &&
                       caps.OutputReportByteLength >= 9;
            if (preparsed) HidD_FreePreparsedData(preparsed);
        }
        if (matching)
        {
            std::array<uint8_t, 9> request = {};
            std::array<uint8_t, 9> response = {};
            request[0] = kConfigReportId;
            request[1] = 9;
            DWORD written = 0;
            DWORD received = 0;
            if (WriteFile(candidate, request.data(), request.size(), &written, nullptr) &&
                written == request.size() && ReadFile(candidate, response.data(), response.size(),
                                                       &received, nullptr) &&
                received == response.size() && response[0] == kConfigReportId && response[1] == 0)
            {
                MacropadBoardIdentity identity;
                identity.variant = response[2];
                for (unsigned i = 0; i < 5; ++i) identity.uid[i] = response[3 + i];
                identities.push_back(identity);
            }
        }
        CloseHandle(candidate);
    }
    SetupDiDestroyDeviceInfoList(dev_info);
    if (identities.empty())
    {
        set_error(error, "No UID-capable CH552 macropad found");
        return false;
    }
    return true;
}

bool MacropadHid::open(std::string *error)
{
    return ensure_open(error, 2);
}

bool MacropadHid::send_mic_state(bool muted, std::string *error)
{
    if (!ensure_open(error, 1))
    {
        return false;
    }

    BYTE report[2] = {kMicStateReportId, static_cast<BYTE>(muted ? 1 : 0)};
    if (!HidD_SetFeature(static_cast<HANDLE>(handle_), report, sizeof(report)))
    {
        set_win32_error(error, "HidD_SetFeature");
        close();
        return false;
    }
    return true;
}

bool MacropadHid::exchange_config(const std::array<uint8_t, 9> &request,
                                  std::array<uint8_t, 9> &response,
                                  std::string *error)
{
    if (request[0] != kConfigReportId)
    {
        set_error(error, "invalid configuration report ID");
        return false;
    }
    if (!ensure_open(error, 2))
    {
        return false;
    }

    DWORD written = 0;
    if (!WriteFile(static_cast<HANDLE>(handle_), request.data(),
                   static_cast<DWORD>(request.size()), &written, nullptr) ||
        written != request.size())
    {
        set_win32_error(error, "WriteFile(config report)");
        close();
        return false;
    }

    DWORD received = 0;
    if (!ReadFile(static_cast<HANDLE>(handle_), response.data(),
                  static_cast<DWORD>(response.size()), &received, nullptr) ||
        received != response.size() || response[0] != kConfigReportId)
    {
        set_win32_error(error, "ReadFile(config response)");
        close();
        return false;
    }
    return true;
}

bool MacropadHid::request_bootloader(std::string *error)
{
    if (!ensure_open(error, 2))
    {
        return false;
    }

    std::array<uint8_t, 9> request = {};
    request[0] = kConfigReportId;
    request[1] = 7; // MACRO_CONFIG_CMD_ENTER_BOOTLOADER
    request[4] = 0xB0;
    request[5] = 0x07;

    DWORD written = 0;
    if (!WriteFile(static_cast<HANDLE>(handle_), request.data(),
                   static_cast<DWORD>(request.size()), &written, nullptr) ||
        written != request.size())
    {
        set_win32_error(error, "WriteFile(bootloader request)");
        close();
        return false;
    }
    return true;
}

bool MacropadHid::ensure_open(std::string *error, uint16_t usage)
{
    if (handle_ && usage_ == usage)
    {
        return true;
    }
    close();

    GUID hid_guid;
    HidD_GetHidGuid(&hid_guid);
    HDEVINFO dev_info = SetupDiGetClassDevsW(&hid_guid, nullptr, nullptr,
                                             DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (dev_info == INVALID_HANDLE_VALUE)
    {
        set_win32_error(error, "SetupDiGetClassDevsW");
        return false;
    }

    for (DWORD index = 0;; ++index)
    {
        SP_DEVICE_INTERFACE_DATA interface_data = {};
        interface_data.cbSize = sizeof(interface_data);
        if (!SetupDiEnumDeviceInterfaces(dev_info, nullptr, &hid_guid, index,
                                         &interface_data))
        {
            break;
        }

        DWORD required_size = 0;
        SetupDiGetDeviceInterfaceDetailW(dev_info, &interface_data, nullptr, 0,
                                         &required_size, nullptr);
        if (required_size == 0)
        {
            continue;
        }

        std::vector<BYTE> detail_buffer(required_size);
        auto *detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W *>(detail_buffer.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(dev_info, &interface_data, detail,
                                              required_size, nullptr, nullptr))
        {
            continue;
        }

        HANDLE candidate = CreateFileW(detail->DevicePath, GENERIC_READ | GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (candidate == INVALID_HANDLE_VALUE)
        {
            continue;
        }

        HIDD_ATTRIBUTES attributes = {};
        attributes.Size = sizeof(attributes);
        if (HidD_GetAttributes(candidate, &attributes) &&
            attributes.VendorID == vendor_id_ && attributes.ProductID == product_id_)
        {
            PHIDP_PREPARSED_DATA preparsed = nullptr;
            HIDP_CAPS caps = {};
            if (HidD_GetPreparsedData(candidate, &preparsed) &&
                HidP_GetCaps(preparsed, &caps) == HIDP_STATUS_SUCCESS &&
                caps.UsagePage == 0xFF00 && caps.Usage == usage &&
                ((usage == 1 && caps.FeatureReportByteLength >= 2) ||
                 (usage == 2 && caps.OutputReportByteLength >= 9)))
            {
                HidD_FreePreparsedData(preparsed);
                handle_ = candidate;
                usage_ = usage;
                if (!target_uid_.empty() && usage == 2)
                {
                    std::array<uint8_t, 9> request = {};
                    std::array<uint8_t, 9> response = {};
                    request[0] = kConfigReportId;
                    request[1] = 9; // MACRO_CONFIG_CMD_GET_BOARD_ID
                    std::string query_error;
                    if (!exchange_config(request, response, &query_error) ||
                        response[1] != 0 || !uid_matches(response, target_uid_))
                    {
                        close();
                        continue;
                    }
                }
                SetupDiDestroyDeviceInfoList(dev_info);
                return true;
            }
            if (preparsed)
            {
                HidD_FreePreparsedData(preparsed);
            }
        }
        CloseHandle(candidate);
    }

    SetupDiDestroyDeviceInfoList(dev_info);
    set_error(error, "CH552 macropad HID device not found");
    return false;
}

void MacropadHid::close()
{
    if (handle_)
    {
        CloseHandle(static_cast<HANDLE>(handle_));
        handle_ = nullptr;
    }
    usage_ = 0;
}
