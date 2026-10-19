#pragma once

#include <Windows.h>
#include <array>
#include <cstdint>
#include <string>

#include "rgb_hid_protocol.h"

namespace rgb_tool::hid {

class HidDevice {
public:
    HidDevice() = default;
    ~HidDevice();
    HidDevice(const HidDevice&) = delete;
    HidDevice& operator=(const HidDevice&) = delete;

    bool Open(std::wstring* error);
    void Close();
    bool IsOpen() const { return handle_ != INVALID_HANDLE_VALUE; }
    bool IsConnected();
    bool Send(Command command, const std::uint8_t* payload, std::uint8_t length, std::wstring* error);
    const std::wstring& Path() const { return path_; }

private:
    HANDLE handle_ = INVALID_HANDLE_VALUE;
    std::wstring path_;
    std::uint8_t sequence_ = 0;
};

} // namespace rgb_tool::hid
