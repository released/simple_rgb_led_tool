#pragma once

#include <array>
#include <cstdint>

namespace rgb_tool::hid {

constexpr unsigned short kVendorId = 0x0416;
constexpr unsigned short kProductId = 0x0125;
constexpr std::uint8_t kMagic = 0xA5;
constexpr std::uint8_t kProtocolVersion = 1;
constexpr std::size_t kReportSize = 64;
constexpr std::size_t kPayloadOffset = 5;

enum class Command : std::uint8_t {
    GetInfo = 0x01,
    SetConfig = 0x02,
    SetStatic = 0x03,
    SetPixels = 0x04,
    SetEffect = 0x05,
    Stop = 0x06
};

enum class Effect : std::uint8_t {
    Static = 0,
    Breathing = 1,
    Blink = 2,
    Gradient = 3,
    Flow = 4
};

inline std::uint8_t Crc8(const std::uint8_t* data, std::size_t length) {
    std::uint8_t crc = 0;
    for (std::size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x80u) ? static_cast<std::uint8_t>((crc << 1) ^ 0x07u)
                                : static_cast<std::uint8_t>(crc << 1);
        }
    }
    return crc;
}

inline std::array<std::uint8_t, kReportSize> MakeReport(Command command, std::uint8_t sequence,
                                                        const std::uint8_t* payload, std::uint8_t length) {
    std::array<std::uint8_t, kReportSize> report{};
    if (length > kReportSize - kPayloadOffset - 1) length = static_cast<std::uint8_t>(kReportSize - kPayloadOffset - 1);
    report[0] = kMagic;
    report[1] = kProtocolVersion;
    report[2] = static_cast<std::uint8_t>(command);
    report[3] = sequence;
    report[4] = length;
    for (std::uint8_t i = 0; i < length; ++i) report[kPayloadOffset + i] = payload[i];
    report[kReportSize - 1] = Crc8(report.data(), kReportSize - 1);
    return report;
}

} // namespace rgb_tool::hid
