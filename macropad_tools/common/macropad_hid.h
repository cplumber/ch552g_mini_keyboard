#pragma once

#include <array>
#include <cstdint>
#include <string>

class MacropadHid
{
public:
    MacropadHid();
    ~MacropadHid();

    MacropadHid(const MacropadHid &) = delete;
    MacropadHid &operator=(const MacropadHid &) = delete;

    void configure(uint16_t vendor_id, uint16_t product_id);
    bool open(std::string *error = nullptr);
    bool send_mic_state(bool muted, std::string *error = nullptr);
    bool exchange_config(const std::array<uint8_t, 9> &request,
                         std::array<uint8_t, 9> &response,
                         std::string *error = nullptr);
    bool request_bootloader(std::string *error = nullptr);

private:
    bool ensure_open(std::string *error, uint16_t usage);
    void close();

    void *handle_ = nullptr;
    uint16_t vendor_id_ = 0;
    uint16_t product_id_ = 0;
    uint16_t usage_ = 0;
};
