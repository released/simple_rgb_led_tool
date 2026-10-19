#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace rgb_tool::core {

constexpr int kMinLedCount = 1;
constexpr int kMaxLedCount = 64;
constexpr int kDefaultLedCount = 24;
constexpr int kDefaultFrameDurationMs = 80;

enum class LedShape {
    Line = 0,
    Ring,
    Square,
    UShape
};

enum class LedDirection {
    Forward = 0,
    Reverse
};

enum class LedPathMode {
    Loop = 0,
    Bounce
};

enum class LedOutputBackend {
    Llsi = 0,
    Spi,
    SpiPdma,
    Gpio
};

struct RgbColor {
    std::uint8_t r = 0u;
    std::uint8_t g = 0u;
    std::uint8_t b = 0u;
};

struct PatternFrame {
    std::uint16_t duration_ms = kDefaultFrameDurationMs;
    std::vector<RgbColor> leds;
};

struct RgbPattern {
    std::wstring name = L"Moving Dot";
    int led_count = kDefaultLedCount;
    LedShape shape = LedShape::Line;
    int start_index = 0;
    LedDirection direction = LedDirection::Forward;
    LedPathMode path_mode = LedPathMode::Loop;
    std::vector<PatternFrame> frames;
};

struct GeneratedSourceFile {
    std::wstring file_name;
    std::wstring text;
};

std::wstring ShapeDisplayName(LedShape shape);
std::wstring ShapePersistName(LedShape shape);
LedShape ParseShapeName(const std::wstring& text);
std::wstring DirectionDisplayName(LedDirection direction);
std::wstring DirectionPersistName(LedDirection direction);
LedDirection ParseDirectionName(const std::wstring& text);
std::wstring PathModeDisplayName(LedPathMode path_mode);
std::wstring PathModePersistName(LedPathMode path_mode);
LedPathMode ParsePathModeName(const std::wstring& text);
std::wstring BackendDisplayName(LedOutputBackend backend);
std::wstring BackendPersistName(LedOutputBackend backend);
LedOutputBackend ParseBackendName(const std::wstring& text);

std::vector<std::wstring> PresetNames();
RgbPattern MakePreset(
    const std::wstring& preset_name,
    int led_count,
    LedShape shape,
    int start_index = 0,
    LedDirection direction = LedDirection::Forward,
    LedPathMode path_mode = LedPathMode::Loop
);
void EnsurePatternShape(RgbPattern* pattern, int led_count, LedShape shape);
void ApplyPatternPathSettings(
    RgbPattern* pattern,
    int start_index,
    LedDirection direction,
    LedPathMode path_mode
);

std::vector<GeneratedSourceFile> GenerateSourceFiles(
    const RgbPattern& pattern,
    const std::wstring& symbol_name,
    LedOutputBackend backend
);
std::vector<GeneratedSourceFile> GenerateLlsiSourceFiles(const RgbPattern& pattern, const std::wstring& symbol_name);
std::wstring GeneratePatternCode(
    const RgbPattern& pattern,
    const std::wstring& symbol_name,
    LedOutputBackend backend
);
bool SaveGeneratedSourceFiles(const std::wstring& directory, const std::vector<GeneratedSourceFile>& files, std::wstring* error);
bool SavePatternFile(const RgbPattern& pattern, const std::wstring& path, std::wstring* error);
bool LoadPatternFile(const std::wstring& path, RgbPattern* out, std::wstring* error);
bool SaveTextFile(const std::wstring& path, const std::wstring& text, std::wstring* error);

RgbColor ColorWheel(int position);

} // namespace rgb_tool::core
