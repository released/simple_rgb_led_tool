#include "rgb_pattern.h"

#include <algorithm>
#include <cstdio>
#include <cwctype>
#include <sstream>

#include "text_utils.h"

namespace rgb_tool::core {
namespace {

std::wstring Trim(const std::wstring& text) {
    size_t begin = 0u;
    size_t end = text.size();
    while (begin < end && iswspace(text[begin])) {
        ++begin;
    }
    while (end > begin && iswspace(text[end - 1u])) {
        --end;
    }
    return text.substr(begin, end - begin);
}

bool StartsWith(const std::wstring& text, const std::wstring& prefix) {
    return text.size() >= prefix.size() && text.compare(0u, prefix.size(), prefix) == 0;
}

std::wstring ValueAfter(const std::wstring& line, const std::wstring& prefix) {
    if (!StartsWith(line, prefix)) {
        return L"";
    }
    return Trim(line.substr(prefix.size()));
}

int HexDigit(wchar_t ch) {
    if (ch >= L'0' && ch <= L'9') {
        return ch - L'0';
    }
    if (ch >= L'a' && ch <= L'f') {
        return 10 + ch - L'a';
    }
    if (ch >= L'A' && ch <= L'F') {
        return 10 + ch - L'A';
    }
    return -1;
}

bool ParseColorToken(const std::wstring& token, RgbColor* color) {
    int digits[6] = {};
    size_t start = 0u;
    size_t i = 0u;

    if (color == nullptr) {
        return false;
    }
    if (token.size() >= 2u && token[0] == L'0' && (token[1] == L'x' || token[1] == L'X')) {
        start = 2u;
    }
    if (token.size() - start != 6u) {
        return false;
    }
    for (i = 0u; i < 6u; ++i) {
        digits[i] = HexDigit(token[start + i]);
        if (digits[i] < 0) {
            return false;
        }
    }
    color->r = static_cast<std::uint8_t>((digits[0] << 4) | digits[1]);
    color->g = static_cast<std::uint8_t>((digits[2] << 4) | digits[3]);
    color->b = static_cast<std::uint8_t>((digits[4] << 4) | digits[5]);
    return true;
}

std::wstring SanitizeSymbol(const std::wstring& symbol_name) {
    std::wstring out;
    bool last_underscore = false;

    for (wchar_t ch : symbol_name) {
        const bool ok = (ch >= L'a' && ch <= L'z') ||
                        (ch >= L'A' && ch <= L'Z') ||
                        (ch >= L'0' && ch <= L'9') ||
                        ch == L'_';
        if (ok) {
            out.push_back(ch);
            last_underscore = false;
        } else if (!last_underscore) {
            out.push_back(L'_');
            last_underscore = true;
        }
    }
    while (!out.empty() && out.front() >= L'0' && out.front() <= L'9') {
        out.insert(out.begin(), L'_');
    }
    if (out.empty()) {
        out = L"g_rgb_pattern";
    }
    return out;
}

std::wstring ToUpperIdentifier(std::wstring text) {
    for (wchar_t& ch : text) {
        if (ch >= L'a' && ch <= L'z') {
            ch = static_cast<wchar_t>(ch - L'a' + L'A');
        } else if (!((ch >= L'A' && ch <= L'Z') || (ch >= L'0' && ch <= L'9'))) {
            ch = L'_';
        }
    }
    return text;
}

std::wstring JoinPath(const std::wstring& directory, const std::wstring& file_name) {
    if (directory.empty()) {
        return file_name;
    }
    if (directory.back() == L'\\' || directory.back() == L'/') {
        return directory + file_name;
    }
    return directory + L"\\" + file_name;
}

PatternFrame MakeFrame(int led_count, std::uint16_t duration_ms, RgbColor fill) {
    PatternFrame frame;
    frame.duration_ms = duration_ms;
    frame.leds.assign(static_cast<size_t>(led_count), fill);
    return frame;
}

int NormalizeStartIndex(int start_index, int led_count) {
    if (led_count <= 0) {
        return 0;
    }
    start_index %= led_count;
    if (start_index < 0) {
        start_index += led_count;
    }
    return start_index;
}

std::vector<int> BuildLoopLedOrder(int led_count, int start_index, LedDirection direction) {
    std::vector<int> order;
    int i = 0;
    int index = NormalizeStartIndex(start_index, led_count);
    const int step = (direction == LedDirection::Reverse) ? -1 : 1;

    order.reserve(static_cast<size_t>((std::max)(0, led_count)));
    for (i = 0; i < led_count; ++i) {
        order.push_back(index);
        index += step;
        if (index >= led_count) {
            index = 0;
        } else if (index < 0) {
            index = led_count - 1;
        }
    }
    return order;
}

std::vector<int> BuildMotionLedPath(int led_count, int start_index, LedDirection direction, LedPathMode path_mode) {
    std::vector<int> path;
    int index = NormalizeStartIndex(start_index, led_count);
    int step = (direction == LedDirection::Reverse) ? -1 : 1;
    int i = 0;
    const int path_len = (path_mode == LedPathMode::Bounce && led_count > 1) ? (led_count - 1) * 2 : led_count;

    path.reserve(static_cast<size_t>((std::max)(0, path_len)));
    for (i = 0; i < path_len; ++i) {
        path.push_back(index);
        if (path_mode == LedPathMode::Bounce) {
            if ((index == 0 && step < 0) || (index == led_count - 1 && step > 0)) {
                step = -step;
            }
            index += step;
        } else {
            index += step;
            if (index >= led_count) {
                index = 0;
            } else if (index < 0) {
                index = led_count - 1;
            }
        }
    }
    return path;
}

RgbPattern MakeSolid(
    const std::wstring& name,
    int led_count,
    LedShape shape,
    RgbColor color,
    int start_index,
    LedDirection direction,
    LedPathMode path_mode
) {
    RgbPattern pattern;
    pattern.name = name;
    pattern.led_count = ClampInt(led_count, kMinLedCount, kMaxLedCount);
    pattern.shape = shape;
    pattern.start_index = NormalizeStartIndex(start_index, pattern.led_count);
    pattern.direction = direction;
    pattern.path_mode = path_mode;
    pattern.frames.push_back(MakeFrame(pattern.led_count, 250u, color));
    return pattern;
}

void AddRainbowFrame(RgbPattern* pattern, int frame_index, int frame_count) {
    PatternFrame frame;
    const std::vector<int> order = BuildLoopLedOrder(pattern->led_count, pattern->start_index, pattern->direction);
    int i = 0;

    frame.duration_ms = 80u;
    frame.leds.resize(static_cast<size_t>(pattern->led_count));
    for (i = 0; i < pattern->led_count; ++i) {
        const int wheel = ((i * 256) / (std::max)(1, pattern->led_count) +
                           (frame_index * 256) / (std::max)(1, frame_count)) & 255;
        frame.leds[static_cast<size_t>(order[static_cast<size_t>(i)])] = ColorWheel(wheel);
    }
    pattern->frames.push_back(frame);
}

struct BackendSpec {
    LedOutputBackend backend = LedOutputBackend::Llsi;
    const wchar_t* prefix = L"llsi";
    const wchar_t* display_name = L"LLSI";
    bool llsi_word_buffer = true;
    bool spi_polling = false;
    bool spi_pdma = false;
    bool gpio = false;
};

BackendSpec GetBackendSpec(LedOutputBackend backend) {
    switch (backend) {
    case LedOutputBackend::Spi:
        return BackendSpec{backend, L"spi", L"SPI", false, true, false, false};
    case LedOutputBackend::SpiPdma:
        return BackendSpec{backend, L"spi_pdma", L"SPI + PDMA", false, false, true, false};
    case LedOutputBackend::Gpio:
        return BackendSpec{backend, L"gpio", L"GPIO", false, false, false, true};
    case LedOutputBackend::Llsi:
    default:
        return BackendSpec{LedOutputBackend::Llsi, L"llsi", L"LLSI", true, false, false, false};
    }
}

std::wstring PrefixUpper(const BackendSpec& spec) {
    return ToUpperIdentifier(spec.prefix);
}

std::wstring DriverPrefix(const BackendSpec& spec) {
    return PrefixUpper(spec) + L"_LED_PATTERN_DRV";
}

std::wstring PatternPrefix(const BackendSpec& spec) {
    return PrefixUpper(spec) + L"_LED_PATTERN";
}

} // namespace

std::wstring ShapeDisplayName(LedShape shape) {
    switch (shape) {
    case LedShape::Line: return L"Line";
    case LedShape::Ring: return L"Ring";
    case LedShape::Square: return L"Square";
    case LedShape::UShape: return L"U Shape";
    default: return L"Line";
    }
}

std::wstring ShapePersistName(LedShape shape) {
    switch (shape) {
    case LedShape::Line: return L"Line";
    case LedShape::Ring: return L"Ring";
    case LedShape::Square: return L"Square";
    case LedShape::UShape: return L"UShape";
    default: return L"Line";
    }
}

LedShape ParseShapeName(const std::wstring& text) {
    std::wstring s = text;
    for (wchar_t& ch : s) {
        ch = static_cast<wchar_t>(towupper(ch));
    }
    if (s == L"RING" || s == L"CIRCLE") {
        return LedShape::Ring;
    }
    if (s == L"SQUARE") {
        return LedShape::Square;
    }
    if (s == L"USHAPE" || s == L"U SHAPE" || s == L"U") {
        return LedShape::UShape;
    }
    return LedShape::Line;
}

std::wstring DirectionDisplayName(LedDirection direction) {
    return direction == LedDirection::Reverse ? L"Reverse" : L"Forward";
}

std::wstring DirectionPersistName(LedDirection direction) {
    return DirectionDisplayName(direction);
}

LedDirection ParseDirectionName(const std::wstring& text) {
    std::wstring s = text;
    for (wchar_t& ch : s) {
        ch = static_cast<wchar_t>(towupper(ch));
    }
    if (s == L"REVERSE" || s == L"REV" || s == L"CCW") {
        return LedDirection::Reverse;
    }
    return LedDirection::Forward;
}

std::wstring PathModeDisplayName(LedPathMode path_mode) {
    return path_mode == LedPathMode::Bounce ? L"Bounce" : L"Loop";
}

std::wstring PathModePersistName(LedPathMode path_mode) {
    return PathModeDisplayName(path_mode);
}

LedPathMode ParsePathModeName(const std::wstring& text) {
    std::wstring s = text;
    for (wchar_t& ch : s) {
        ch = static_cast<wchar_t>(towupper(ch));
    }
    if (s == L"BOUNCE" || s == L"PINGPONG" || s == L"PING-PONG") {
        return LedPathMode::Bounce;
    }
    return LedPathMode::Loop;
}

std::wstring BackendDisplayName(LedOutputBackend backend) {
    return GetBackendSpec(backend).display_name;
}

std::wstring BackendPersistName(LedOutputBackend backend) {
    switch (backend) {
    case LedOutputBackend::Spi: return L"SPI";
    case LedOutputBackend::SpiPdma: return L"SPI_PDMA";
    case LedOutputBackend::Gpio: return L"GPIO";
    case LedOutputBackend::Llsi:
    default: return L"LLSI";
    }
}

LedOutputBackend ParseBackendName(const std::wstring& text) {
    std::wstring s = text;
    for (wchar_t& ch : s) {
        ch = static_cast<wchar_t>(towupper(ch));
        if (ch == L'+' || ch == L'-' || iswspace(ch)) {
            ch = L'_';
        }
    }
    if (s.find(L"SPI") != std::wstring::npos && s.find(L"PDMA") != std::wstring::npos) {
        return LedOutputBackend::SpiPdma;
    }
    if (s == L"SPI") {
        return LedOutputBackend::Spi;
    }
    if (s == L"GPIO") {
        return LedOutputBackend::Gpio;
    }
    return LedOutputBackend::Llsi;
}

std::vector<std::wstring> PresetNames() {
    return {
        L"Moving Dot",
        L"Rainbow Cycle",
        L"Theater Chase",
        L"Color Wipe Red",
        L"Breathing Blue",
        L"Alert Alternating",
        L"Solid Red",
        L"Solid Green",
        L"Solid Blue",
        L"Solid White",
        L"Off"
    };
}

RgbColor ColorWheel(int position) {
    int pos = 255 - (position & 255);
    RgbColor color;

    if (pos < 85) {
        color.r = static_cast<std::uint8_t>(255 - pos * 3);
        color.g = 0u;
        color.b = static_cast<std::uint8_t>(pos * 3);
        return color;
    }
    if (pos < 170) {
        pos -= 85;
        color.r = 0u;
        color.g = static_cast<std::uint8_t>(pos * 3);
        color.b = static_cast<std::uint8_t>(255 - pos * 3);
        return color;
    }
    pos -= 170;
    color.r = static_cast<std::uint8_t>(pos * 3);
    color.g = static_cast<std::uint8_t>(255 - pos * 3);
    color.b = 0u;
    return color;
}

void EnsurePatternShape(RgbPattern* pattern, int led_count, LedShape shape) {
    int safe_count = ClampInt(led_count, kMinLedCount, kMaxLedCount);

    if (pattern == nullptr) {
        return;
    }
    pattern->led_count = safe_count;
    pattern->shape = shape;
    if (pattern->frames.empty()) {
        pattern->frames.push_back(MakeFrame(safe_count, kDefaultFrameDurationMs, RgbColor{}));
    }
    for (PatternFrame& frame : pattern->frames) {
        frame.leds.resize(static_cast<size_t>(safe_count));
        if (frame.duration_ms == 0u) {
            frame.duration_ms = kDefaultFrameDurationMs;
        }
    }
    pattern->start_index = NormalizeStartIndex(pattern->start_index, safe_count);
}

void ApplyPatternPathSettings(
    RgbPattern* pattern,
    int start_index,
    LedDirection direction,
    LedPathMode path_mode
) {
    std::vector<int> old_order;
    std::vector<int> new_order;
    size_t frame_index = 0u;
    int i = 0;
    int safe_start = 0;

    if (pattern == nullptr) {
        return;
    }
    safe_start = NormalizeStartIndex(start_index, pattern->led_count);
    old_order = BuildLoopLedOrder(pattern->led_count, pattern->start_index, pattern->direction);
    new_order = BuildLoopLedOrder(pattern->led_count, safe_start, direction);
    if (old_order.size() == new_order.size() && pattern->led_count > 0 &&
        (pattern->start_index != safe_start || pattern->direction != direction)) {
        for (frame_index = 0u; frame_index < pattern->frames.size(); ++frame_index) {
            PatternFrame& frame = pattern->frames[frame_index];
            std::vector<RgbColor> mapped = frame.leds;
            if (frame.leds.size() < static_cast<size_t>(pattern->led_count)) {
                frame.leds.resize(static_cast<size_t>(pattern->led_count));
                mapped.resize(static_cast<size_t>(pattern->led_count));
            }
            for (i = 0; i < pattern->led_count; ++i) {
                mapped[static_cast<size_t>(new_order[static_cast<size_t>(i)])] =
                    frame.leds[static_cast<size_t>(old_order[static_cast<size_t>(i)])];
            }
            frame.leds = mapped;
        }
    }
    pattern->start_index = safe_start;
    pattern->direction = direction;
    pattern->path_mode = path_mode;
}

RgbPattern MakePreset(
    const std::wstring& preset_name,
    int led_count,
    LedShape shape,
    int start_index,
    LedDirection direction,
    LedPathMode path_mode
) {
    const int count = ClampInt(led_count, kMinLedCount, kMaxLedCount);
    const std::wstring name = preset_name.empty() ? L"Moving Dot" : preset_name;
    const int safe_start = NormalizeStartIndex(start_index, count);
    const std::vector<int> loop_order = BuildLoopLedOrder(count, safe_start, direction);
    const std::vector<int> motion_path = BuildMotionLedPath(count, safe_start, direction, path_mode);
    RgbPattern pattern;
    int i = 0;

    if (name == L"Solid Red") {
        return MakeSolid(name, count, shape, RgbColor{255u, 0u, 0u}, safe_start, direction, path_mode);
    }
    if (name == L"Solid Green") {
        return MakeSolid(name, count, shape, RgbColor{0u, 255u, 0u}, safe_start, direction, path_mode);
    }
    if (name == L"Solid Blue") {
        return MakeSolid(name, count, shape, RgbColor{0u, 0u, 255u}, safe_start, direction, path_mode);
    }
    if (name == L"Solid White") {
        return MakeSolid(name, count, shape, RgbColor{255u, 255u, 255u}, safe_start, direction, path_mode);
    }
    if (name == L"Off") {
        return MakeSolid(name, count, shape, RgbColor{0u, 0u, 0u}, safe_start, direction, path_mode);
    }

    pattern.name = name;
    pattern.led_count = count;
    pattern.shape = shape;
    pattern.start_index = safe_start;
    pattern.direction = direction;
    pattern.path_mode = path_mode;

    if (name == L"Rainbow Cycle") {
        for (i = 0; i < 16; ++i) {
            AddRainbowFrame(&pattern, i, 16);
        }
        return pattern;
    }

    if (name == L"Theater Chase") {
        for (i = 0; i < 6; ++i) {
            PatternFrame frame = MakeFrame(count, 90u, RgbColor{0u, 0u, 0u});
            int led = 0;
            for (led = 0; led < count; ++led) {
                if (((led + i) % 3) == 0) {
                    frame.leds[static_cast<size_t>(loop_order[static_cast<size_t>(led)])] = RgbColor{255u, 80u, 0u};
                }
            }
            pattern.frames.push_back(frame);
        }
        return pattern;
    }

    if (name == L"Color Wipe Red") {
        for (i = 0; i <= static_cast<int>(motion_path.size()); ++i) {
            PatternFrame frame = MakeFrame(count, 55u, RgbColor{0u, 0u, 0u});
            int led = 0;
            for (led = 0; led < i && led < static_cast<int>(motion_path.size()); ++led) {
                frame.leds[static_cast<size_t>(motion_path[static_cast<size_t>(led)])] = RgbColor{255u, 0u, 0u};
            }
            pattern.frames.push_back(frame);
        }
        return pattern;
    }

    if (name == L"Breathing Blue") {
        for (i = 0; i < static_cast<int>(motion_path.size()); ++i) {
            PatternFrame frame = MakeFrame(count, 55u, RgbColor{0u, 0u, 0u});
            const int previous = (i == 0) ? static_cast<int>(motion_path.size()) - 1 : i - 1;
            const int next = (i + 1 >= static_cast<int>(motion_path.size())) ? 0 : i + 1;
            if (!motion_path.empty()) {
                frame.leds[static_cast<size_t>(motion_path[static_cast<size_t>(previous)])] = RgbColor{0u, 0u, 48u};
                frame.leds[static_cast<size_t>(motion_path[static_cast<size_t>(i)])] = RgbColor{0u, 0u, 255u};
                frame.leds[static_cast<size_t>(motion_path[static_cast<size_t>(next)])] = RgbColor{0u, 0u, 96u};
            }
            pattern.frames.push_back(frame);
        }
        return pattern;
    }

    if (name == L"Alert Alternating") {
        for (i = 0; i < 8; ++i) {
            PatternFrame frame = MakeFrame(count, 120u, RgbColor{0u, 0u, 0u});
            int led = 0;
            for (led = 0; led < count; ++led) {
                const RgbColor color = ((led + i) & 1) ? RgbColor{255u, 255u, 255u} : RgbColor{255u, 0u, 0u};
                frame.leds[static_cast<size_t>(loop_order[static_cast<size_t>(led)])] = color;
            }
            pattern.frames.push_back(frame);
        }
        return pattern;
    }

    for (i = 0; i < static_cast<int>(motion_path.size()); ++i) {
        PatternFrame frame = MakeFrame(count, 75u, RgbColor{0u, 0u, 0u});
        frame.leds[static_cast<size_t>(motion_path[static_cast<size_t>(i)])] = RgbColor{0u, 180u, 255u};
        pattern.frames.push_back(frame);
    }
    return pattern;
}

std::vector<GeneratedSourceFile> GenerateSourceFiles(
    const RgbPattern& pattern,
    const std::wstring& symbol_name,
    LedOutputBackend backend
) {
    const BackendSpec spec = GetBackendSpec(backend);
    const std::wstring symbol = SanitizeSymbol(symbol_name);
    const std::wstring upper_symbol = ToUpperIdentifier(symbol);
    const std::wstring led_count_macro = upper_symbol + L"_LED_COUNT";
    const std::wstring frame_count_macro = upper_symbol + L"_FRAME_COUNT";
    const std::wstring prefix = spec.prefix;
    const std::wstring pattern_prefix = PatternPrefix(spec);
    const std::wstring drv_prefix = DriverPrefix(spec);
    const std::wstring drv_h_name = prefix + L"_led_pattern_drv.h";
    const std::wstring drv_c_name = prefix + L"_led_pattern_drv.c";
    const std::wstring player_h_name = prefix + L"_led_pattern_player.h";
    const std::wstring player_c_name = prefix + L"_led_pattern_player.c";
    const std::wstring drv_guard = pattern_prefix + L"_DRV_H";
    const std::wstring player_guard = pattern_prefix + L"_PLAYER_H";
    const std::wstring color_type = prefix + L"_led_pattern_color_t";
    const std::wstring frame_type = prefix + L"_led_pattern_frame_t";
    const std::wstring data_name = L"s_" + prefix + L"_led_data";
    const std::wstring spi_data_len_macro = pattern_prefix + L"_SPI_DATA_LEN";
    const std::wstring spi_encoded_name = L"s_" + prefix + L"_spi_data";
    std::vector<GeneratedSourceFile> files;
    std::wstringstream drv_h;
    std::wstringstream drv_c;
    std::wstringstream player_h;
    std::wstringstream player_c;
    size_t frame_index = 0u;

    drv_h << L"#ifndef " << drv_guard << L"\r\n";
    drv_h << L"#define " << drv_guard << L"\r\n\r\n";
    drv_h << L"#include <stdint.h>\r\n\r\n";
    drv_h << L"#ifndef LED_NUM\r\n";
    drv_h << L"#define LED_NUM (" << pattern.led_count << L"u)\r\n";
    drv_h << L"#endif\r\n\r\n";
    drv_h << L"#ifndef LED_DATA_LEN\r\n";
    if (spec.llsi_word_buffer) {
        drv_h << L"#define LED_DATA_LEN (((LED_NUM * 3u) + 3u) / 4u)\r\n";
    } else {
        drv_h << L"#define LED_DATA_LEN (LED_NUM * 3u)\r\n";
    }
    drv_h << L"#endif\r\n\r\n";
    if (spec.spi_polling || spec.spi_pdma) {
        drv_h << L"#ifndef " << spi_data_len_macro << L"\r\n";
        drv_h << L"#define " << spi_data_len_macro << L" (LED_DATA_LEN * 8u)\r\n";
        drv_h << L"#endif\r\n\r\n";
        drv_h << L"#ifndef " << pattern_prefix << L"_SPI_HIGH_CODE\r\n";
        drv_h << L"#define " << pattern_prefix << L"_SPI_HIGH_CODE (0xFCu)\r\n";
        drv_h << L"#endif\r\n\r\n";
        drv_h << L"#ifndef " << pattern_prefix << L"_SPI_LOW_CODE\r\n";
        drv_h << L"#define " << pattern_prefix << L"_SPI_LOW_CODE (0xC0u)\r\n";
        drv_h << L"#endif\r\n\r\n";
        drv_h << L"#ifndef " << pattern_prefix << L"_RESET_FRONT_DELAY\r\n";
        drv_h << L"#define " << pattern_prefix << L"_RESET_FRONT_DELAY (400u)\r\n";
        drv_h << L"#endif\r\n\r\n";
        drv_h << L"#ifndef " << pattern_prefix << L"_RESET_BACK_DELAY\r\n";
        drv_h << L"#define " << pattern_prefix << L"_RESET_BACK_DELAY (2000u)\r\n";
        drv_h << L"#endif\r\n\r\n";
    } else if (spec.gpio) {
        drv_h << L"/* Default GPIO macros match the MS51 P12 sample. Edit for the target pin and timing. */\r\n";
        drv_h << L"#ifndef GPIO_LED_PATTERN_WRITE\r\n";
        drv_h << L"#define GPIO_LED_PATTERN_WRITE(level) (P12 = (level))\r\n";
        drv_h << L"#endif\r\n\r\n";
        drv_h << L"#ifndef GPIO_LED_PATTERN_PIN_INIT\r\n";
        drv_h << L"#define GPIO_LED_PATTERN_PIN_INIT() P12_PUSHPULL_MODE\r\n";
        drv_h << L"#endif\r\n\r\n";
        drv_h << L"#ifndef GPIO_LED_PATTERN_NOP\r\n";
        drv_h << L"#define GPIO_LED_PATTERN_NOP() nop\r\n";
        drv_h << L"#endif\r\n\r\n";
        drv_h << L"#ifndef GPIO_LED_PATTERN_ENTER_CRITICAL\r\n";
        drv_h << L"#define GPIO_LED_PATTERN_ENTER_CRITICAL() clr_EA\r\n";
        drv_h << L"#endif\r\n\r\n";
        drv_h << L"#ifndef GPIO_LED_PATTERN_EXIT_CRITICAL\r\n";
        drv_h << L"#define GPIO_LED_PATTERN_EXIT_CRITICAL() set_EA\r\n";
        drv_h << L"#endif\r\n\r\n";
        drv_h << L"#ifndef " << pattern_prefix << L"_RESET_FRONT_DELAY\r\n";
        drv_h << L"#define " << pattern_prefix << L"_RESET_FRONT_DELAY (100u)\r\n";
        drv_h << L"#endif\r\n\r\n";
        drv_h << L"#ifndef " << pattern_prefix << L"_RESET_BACK_DELAY\r\n";
        drv_h << L"#define " << pattern_prefix << L"_RESET_BACK_DELAY (450u)\r\n";
        drv_h << L"#endif\r\n\r\n";
    }
    drv_h << L"#define " << pattern_prefix << L"_COLOR_ORDER_RGB (0u)\r\n";
    drv_h << L"#define " << pattern_prefix << L"_COLOR_ORDER_GRB (1u)\r\n\r\n";
    drv_h << L"#ifndef " << pattern_prefix << L"_COLOR_ORDER\r\n";
    drv_h << L"#define " << pattern_prefix << L"_COLOR_ORDER "
          << pattern_prefix << (spec.llsi_word_buffer ? L"_COLOR_ORDER_RGB" : L"_COLOR_ORDER_GRB") << L"\r\n";
    drv_h << L"#endif\r\n\r\n";
    drv_h << L"void " << drv_prefix << L"_Init(void);\r\n";
    drv_h << L"void " << drv_prefix << L"_Clear(void);\r\n";
    drv_h << L"void " << drv_prefix << L"_SetPixel(uint8_t index, uint8_t red, uint8_t green, uint8_t blue);\r\n";
    drv_h << L"void " << drv_prefix << L"_Show(void);\r\n";
    drv_h << L"void " << drv_prefix << L"_DelayMs(uint16_t ms);\r\n\r\n";
    drv_h << L"#endif\r\n";

    drv_c << L"/* Generated by Simple RGB LED Tool. */\r\n";
    drv_c << L"#include \"" << drv_h_name << L"\"\r\n";
    drv_c << L"/* LED_NUM and LED_DATA_LEN default to the values generated in the driver header. */\r\n\r\n";
    if (spec.llsi_word_buffer) {
        drv_c << L"extern void LLSI_PDMA_Transfer(uint32_t *LLSIDataBuffer, uint32_t len);\r\n";
        drv_c << L"extern void delay_ms(uint16_t ms);\r\n\r\n";
        drv_c << L"static uint32_t " << data_name << L"[LED_DATA_LEN];\r\n\r\n";
        drv_c << L"void " << drv_prefix << L"_Init(void)\r\n";
        drv_c << L"{\r\n";
        drv_c << L"}\r\n\r\n";
        drv_c << L"static void " << drv_prefix << L"_WriteByte(uint16_t byte_index, uint8_t value)\r\n";
        drv_c << L"{\r\n";
        drv_c << L"    uint8_t *bytes;\r\n";
        drv_c << L"    uint16_t max_bytes;\r\n\r\n";
        drv_c << L"    bytes = (uint8_t *)" << data_name << L";\r\n";
        drv_c << L"    max_bytes = (uint16_t)(LED_DATA_LEN * 4u);\r\n";
        drv_c << L"    if (byte_index >= max_bytes)\r\n";
        drv_c << L"    {\r\n";
        drv_c << L"        return;\r\n";
        drv_c << L"    }\r\n";
        drv_c << L"    bytes[byte_index] = value;\r\n";
        drv_c << L"}\r\n\r\n";
        drv_c << L"void " << drv_prefix << L"_Clear(void)\r\n";
        drv_c << L"{\r\n";
        drv_c << L"    uint16_t i;\r\n\r\n";
        drv_c << L"    for (i = 0u; i < LED_DATA_LEN; i++)\r\n";
        drv_c << L"    {\r\n";
        drv_c << L"        " << data_name << L"[i] = 0u;\r\n";
        drv_c << L"    }\r\n";
        drv_c << L"}\r\n\r\n";
        drv_c << L"void " << drv_prefix << L"_SetPixel(uint8_t index, uint8_t red, uint8_t green, uint8_t blue)\r\n";
        drv_c << L"{\r\n";
        drv_c << L"    uint16_t base;\r\n";
        drv_c << L"    uint16_t max_bytes;\r\n\r\n";
        drv_c << L"    if (index >= LED_NUM)\r\n";
        drv_c << L"    {\r\n";
        drv_c << L"        return;\r\n";
        drv_c << L"    }\r\n";
        drv_c << L"    base = (uint16_t)(index * 3u);\r\n";
        drv_c << L"    max_bytes = (uint16_t)(LED_DATA_LEN * 4u);\r\n";
        drv_c << L"    if ((uint16_t)(base + 2u) >= max_bytes)\r\n";
        drv_c << L"    {\r\n";
        drv_c << L"        return;\r\n";
        drv_c << L"    }\r\n";
        drv_c << L"#if (" << pattern_prefix << L"_COLOR_ORDER == " << pattern_prefix << L"_COLOR_ORDER_GRB)\r\n";
        drv_c << L"    " << drv_prefix << L"_WriteByte(base, green);\r\n";
        drv_c << L"    " << drv_prefix << L"_WriteByte((uint16_t)(base + 1u), red);\r\n";
        drv_c << L"    " << drv_prefix << L"_WriteByte((uint16_t)(base + 2u), blue);\r\n";
        drv_c << L"#else\r\n";
        drv_c << L"    " << drv_prefix << L"_WriteByte(base, red);\r\n";
        drv_c << L"    " << drv_prefix << L"_WriteByte((uint16_t)(base + 1u), green);\r\n";
        drv_c << L"    " << drv_prefix << L"_WriteByte((uint16_t)(base + 2u), blue);\r\n";
        drv_c << L"#endif\r\n";
        drv_c << L"}\r\n\r\n";
        drv_c << L"void " << drv_prefix << L"_Show(void)\r\n";
        drv_c << L"{\r\n";
        drv_c << L"    LLSI_PDMA_Transfer(" << data_name << L", LED_DATA_LEN);\r\n";
        drv_c << L"}\r\n\r\n";
        drv_c << L"void " << drv_prefix << L"_DelayMs(uint16_t ms)\r\n";
        drv_c << L"{\r\n";
        drv_c << L"    delay_ms(ms);\r\n";
        drv_c << L"}\r\n";
    } else {
        drv_c << L"#ifndef " << drv_prefix << L"_DELAY\r\n";
        drv_c << L"extern void delay(uint16_t dly);\r\n";
        drv_c << L"#define " << drv_prefix << L"_DELAY(count) delay((uint16_t)(count))\r\n";
        drv_c << L"#endif\r\n\r\n";
        drv_c << L"#ifndef " << drv_prefix << L"_DELAY_MS\r\n";
        drv_c << L"extern void delay_ms(uint16_t ms);\r\n";
        drv_c << L"#define " << drv_prefix << L"_DELAY_MS(ms) delay_ms((uint16_t)(ms))\r\n";
        drv_c << L"#endif\r\n\r\n";
        if (spec.spi_polling) {
            drv_c << L"#ifndef " << drv_prefix << L"_WRITE_BYTE\r\n";
            drv_c << L"extern void " << drv_prefix << L"_WriteByte(uint8_t data);\r\n";
            drv_c << L"#define " << drv_prefix << L"_WRITE_BYTE(data) " << drv_prefix << L"_WriteByte((uint8_t)(data))\r\n";
            drv_c << L"#endif\r\n\r\n";
        } else if (spec.spi_pdma) {
            drv_c << L"#ifndef " << drv_prefix << L"_TRANSFER\r\n";
            drv_c << L"extern void " << drv_prefix << L"_Transfer(uint8_t *data, uint16_t len);\r\n";
            drv_c << L"#define " << drv_prefix << L"_TRANSFER(data, len) " << drv_prefix << L"_Transfer((data), (uint16_t)(len))\r\n";
            drv_c << L"#endif\r\n\r\n";
            drv_c << L"static uint8_t " << spi_encoded_name << L"[" << spi_data_len_macro << L"];\r\n";
        }
        drv_c << L"static uint8_t " << data_name << L"[LED_DATA_LEN];\r\n\r\n";
        drv_c << L"void " << drv_prefix << L"_Init(void)\r\n";
        drv_c << L"{\r\n";
        if (spec.gpio) {
            drv_c << L"    GPIO_LED_PATTERN_PIN_INIT();\r\n";
            drv_c << L"    GPIO_LED_PATTERN_WRITE(0u);\r\n";
        }
        drv_c << L"}\r\n\r\n";
        if (spec.spi_polling || spec.spi_pdma) {
            drv_c << L"static void " << drv_prefix << L"_EncodeByte(uint8_t value";
            if (spec.spi_pdma) {
                drv_c << L", uint16_t *out_index";
            }
            drv_c << L")\r\n";
            drv_c << L"{\r\n";
            drv_c << L"    uint8_t bit;\r\n\r\n";
            drv_c << L"    for (bit = 0u; bit < 8u; bit++)\r\n";
            drv_c << L"    {\r\n";
            if (spec.spi_polling) {
                drv_c << L"        if ((value & 0x80u) != 0u)\r\n";
                drv_c << L"        {\r\n";
                drv_c << L"            " << drv_prefix << L"_WRITE_BYTE(" << pattern_prefix << L"_SPI_HIGH_CODE);\r\n";
                drv_c << L"        }\r\n";
                drv_c << L"        else\r\n";
                drv_c << L"        {\r\n";
                drv_c << L"            " << drv_prefix << L"_WRITE_BYTE(" << pattern_prefix << L"_SPI_LOW_CODE);\r\n";
                drv_c << L"        }\r\n";
            } else {
                drv_c << L"        if (*out_index >= " << spi_data_len_macro << L")\r\n";
                drv_c << L"        {\r\n";
                drv_c << L"            return;\r\n";
                drv_c << L"        }\r\n";
                drv_c << L"        if ((value & 0x80u) != 0u)\r\n";
                drv_c << L"        {\r\n";
                drv_c << L"            " << spi_encoded_name << L"[*out_index] = (uint8_t)" << pattern_prefix << L"_SPI_HIGH_CODE;\r\n";
                drv_c << L"        }\r\n";
                drv_c << L"        else\r\n";
                drv_c << L"        {\r\n";
                drv_c << L"            " << spi_encoded_name << L"[*out_index] = (uint8_t)" << pattern_prefix << L"_SPI_LOW_CODE;\r\n";
                drv_c << L"        }\r\n";
                drv_c << L"        *out_index = (uint16_t)(*out_index + 1u);\r\n";
            }
            drv_c << L"        value = (uint8_t)(value << 1);\r\n";
            drv_c << L"    }\r\n";
            drv_c << L"}\r\n\r\n";
        } else if (spec.gpio) {
            drv_c << L"static void " << drv_prefix << L"_Send1(void)\r\n";
            drv_c << L"{\r\n";
            drv_c << L"    GPIO_LED_PATTERN_WRITE(1u);\r\n";
            drv_c << L"    GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP();\r\n";
            drv_c << L"    GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP();\r\n";
            drv_c << L"    GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP();\r\n";
            drv_c << L"    GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP();\r\n";
            drv_c << L"    GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP();\r\n";
            drv_c << L"    GPIO_LED_PATTERN_WRITE(0u);\r\n";
            drv_c << L"    GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP();\r\n";
            drv_c << L"}\r\n\r\n";
            drv_c << L"static void " << drv_prefix << L"_Send0(void)\r\n";
            drv_c << L"{\r\n";
            drv_c << L"    GPIO_LED_PATTERN_WRITE(1u);\r\n";
            drv_c << L"    GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP();\r\n";
            drv_c << L"    GPIO_LED_PATTERN_WRITE(0u);\r\n";
            drv_c << L"    GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP();\r\n";
            drv_c << L"    GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP();\r\n";
            drv_c << L"    GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP();\r\n";
            drv_c << L"    GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP();\r\n";
            drv_c << L"    GPIO_LED_PATTERN_NOP(); GPIO_LED_PATTERN_NOP();\r\n";
            drv_c << L"}\r\n\r\n";
            drv_c << L"static void " << drv_prefix << L"_SendByte(uint8_t value)\r\n";
            drv_c << L"{\r\n";
            drv_c << L"    uint8_t bit;\r\n\r\n";
            drv_c << L"    for (bit = 0u; bit < 8u; bit++)\r\n";
            drv_c << L"    {\r\n";
            drv_c << L"        if ((value & 0x80u) != 0u)\r\n";
            drv_c << L"        {\r\n";
            drv_c << L"            " << drv_prefix << L"_Send1();\r\n";
            drv_c << L"        }\r\n";
            drv_c << L"        else\r\n";
            drv_c << L"        {\r\n";
            drv_c << L"            " << drv_prefix << L"_Send0();\r\n";
            drv_c << L"        }\r\n";
            drv_c << L"        value = (uint8_t)(value << 1);\r\n";
            drv_c << L"    }\r\n";
            drv_c << L"}\r\n\r\n";
        }
        drv_c << L"void " << drv_prefix << L"_Clear(void)\r\n";
        drv_c << L"{\r\n";
        drv_c << L"    uint16_t i;\r\n\r\n";
        drv_c << L"    for (i = 0u; i < LED_DATA_LEN; i++)\r\n";
        drv_c << L"    {\r\n";
        drv_c << L"        " << data_name << L"[i] = 0u;\r\n";
        drv_c << L"    }\r\n";
        drv_c << L"}\r\n\r\n";
        drv_c << L"void " << drv_prefix << L"_SetPixel(uint8_t index, uint8_t red, uint8_t green, uint8_t blue)\r\n";
        drv_c << L"{\r\n";
        drv_c << L"    uint16_t base;\r\n\r\n";
        drv_c << L"    if (index >= LED_NUM)\r\n";
        drv_c << L"    {\r\n";
        drv_c << L"        return;\r\n";
        drv_c << L"    }\r\n";
        drv_c << L"    base = (uint16_t)(index * 3u);\r\n";
        drv_c << L"    if ((uint16_t)(base + 2u) >= LED_DATA_LEN)\r\n";
        drv_c << L"    {\r\n";
        drv_c << L"        return;\r\n";
        drv_c << L"    }\r\n";
        drv_c << L"#if (" << pattern_prefix << L"_COLOR_ORDER == " << pattern_prefix << L"_COLOR_ORDER_RGB)\r\n";
        drv_c << L"    " << data_name << L"[base] = red;\r\n";
        drv_c << L"    " << data_name << L"[(uint16_t)(base + 1u)] = green;\r\n";
        drv_c << L"    " << data_name << L"[(uint16_t)(base + 2u)] = blue;\r\n";
        drv_c << L"#else\r\n";
        drv_c << L"    " << data_name << L"[base] = green;\r\n";
        drv_c << L"    " << data_name << L"[(uint16_t)(base + 1u)] = red;\r\n";
        drv_c << L"    " << data_name << L"[(uint16_t)(base + 2u)] = blue;\r\n";
        drv_c << L"#endif\r\n";
        drv_c << L"}\r\n\r\n";
        drv_c << L"void " << drv_prefix << L"_Show(void)\r\n";
        drv_c << L"{\r\n";
        drv_c << L"    uint16_t i;\r\n";
        if (spec.spi_pdma) {
            drv_c << L"    uint16_t out_index;\r\n";
        }
        drv_c << L"\r\n";
        if (spec.spi_pdma) {
            drv_c << L"    out_index = 0u;\r\n";
        }
        drv_c << L"    " << drv_prefix << L"_DELAY(" << pattern_prefix << L"_RESET_FRONT_DELAY);\r\n";
        if (spec.gpio) {
            drv_c << L"    GPIO_LED_PATTERN_ENTER_CRITICAL();\r\n";
        }
        drv_c << L"    for (i = 0u; i < LED_DATA_LEN; i++)\r\n";
        drv_c << L"    {\r\n";
        if (spec.spi_polling) {
            drv_c << L"        " << drv_prefix << L"_EncodeByte(" << data_name << L"[i]);\r\n";
        } else if (spec.spi_pdma) {
            drv_c << L"        " << drv_prefix << L"_EncodeByte(" << data_name << L"[i], &out_index);\r\n";
        } else {
            drv_c << L"        " << drv_prefix << L"_SendByte(" << data_name << L"[i]);\r\n";
        }
        drv_c << L"    }\r\n";
        if (spec.gpio) {
            drv_c << L"    GPIO_LED_PATTERN_EXIT_CRITICAL();\r\n";
        }
        if (spec.spi_pdma) {
            drv_c << L"    " << drv_prefix << L"_TRANSFER(" << spi_encoded_name << L", out_index);\r\n";
        }
        drv_c << L"    " << drv_prefix << L"_DELAY(" << pattern_prefix << L"_RESET_BACK_DELAY);\r\n";
        drv_c << L"}\r\n\r\n";
        drv_c << L"void " << drv_prefix << L"_DelayMs(uint16_t ms)\r\n";
        drv_c << L"{\r\n";
        drv_c << L"    " << drv_prefix << L"_DELAY_MS(ms);\r\n";
        drv_c << L"}\r\n";
    }

    player_h << L"#ifndef " << player_guard << L"\r\n";
    player_h << L"#define " << player_guard << L"\r\n\r\n";
    player_h << L"#include <stdint.h>\r\n";
    player_h << L"#include \"" << drv_h_name << L"\"\r\n\r\n";
    player_h << L"#define " << led_count_macro << L" (" << pattern.led_count << L"u)\r\n";
    player_h << L"#define " << frame_count_macro << L" (" << pattern.frames.size() << L"u)\r\n\r\n";
    player_h << L"#define " << upper_symbol << L"_START_INDEX (" << pattern.start_index << L"u)\r\n";
    player_h << L"#define " << upper_symbol << L"_DIRECTION_FORWARD (0u)\r\n";
    player_h << L"#define " << upper_symbol << L"_DIRECTION_REVERSE (1u)\r\n";
    player_h << L"#define " << upper_symbol << L"_DIRECTION "
             << ((pattern.direction == LedDirection::Reverse) ? upper_symbol + L"_DIRECTION_REVERSE" : upper_symbol + L"_DIRECTION_FORWARD") << L"\r\n";
    player_h << L"#define " << upper_symbol << L"_PATH_MODE_LOOP (0u)\r\n";
    player_h << L"#define " << upper_symbol << L"_PATH_MODE_BOUNCE (1u)\r\n";
    player_h << L"#define " << upper_symbol << L"_PATH_MODE "
             << ((pattern.path_mode == LedPathMode::Bounce) ? upper_symbol + L"_PATH_MODE_BOUNCE" : upper_symbol + L"_PATH_MODE_LOOP") << L"\r\n\r\n";
    player_h << L"typedef struct\r\n";
    player_h << L"{\r\n";
    player_h << L"    uint8_t r;\r\n";
    player_h << L"    uint8_t g;\r\n";
    player_h << L"    uint8_t b;\r\n";
    player_h << L"} " << color_type << L";\r\n\r\n";
    player_h << L"typedef struct\r\n";
    player_h << L"{\r\n";
    player_h << L"    uint16_t duration_ms;\r\n";
    player_h << L"    " << color_type << L" led[" << led_count_macro << L"];\r\n";
    player_h << L"} " << frame_type << L";\r\n\r\n";
    player_h << L"void " << symbol << L"_render_frame(const " << frame_type << L" *frame);\r\n";
    player_h << L"void " << symbol << L"_play_once(void);\r\n";
    player_h << L"uint16_t " << symbol << L"_step(uint16_t frame_index);\r\n\r\n";
    player_h << L"#endif\r\n";

    player_c << L"/* Generated by Simple RGB LED Tool. */\r\n";
    player_c << L"#include \"" << player_h_name << L"\"\r\n\r\n";
    player_c << L"static const " << frame_type << L" " << symbol << L"[" << frame_count_macro << L"] =\r\n";
    player_c << L"{\r\n";

    for (frame_index = 0u; frame_index < pattern.frames.size(); ++frame_index) {
        const PatternFrame& frame = pattern.frames[frame_index];
        size_t led = 0u;
        player_c << L"    { " << static_cast<unsigned int>(frame.duration_ms) << L"u, {\r\n";
        for (led = 0u; led < frame.leds.size(); ++led) {
            const RgbColor& c = frame.leds[led];
            wchar_t line[96] = {};
            swprintf_s(line, L"        { 0x%02Xu, 0x%02Xu, 0x%02Xu }%ls\r\n",
                       static_cast<unsigned int>(c.r),
                       static_cast<unsigned int>(c.g),
                       static_cast<unsigned int>(c.b),
                       (led + 1u == frame.leds.size()) ? L"" : L",");
            player_c << line;
        }
        player_c << L"    } }" << ((frame_index + 1u == pattern.frames.size()) ? L"" : L",") << L"\r\n";
    }

    player_c << L"};\r\n\r\n";
    player_c << L"void " << symbol << L"_render_frame(const " << frame_type << L" *frame)\r\n";
    player_c << L"{\r\n";
    player_c << L"    uint8_t i;\r\n\r\n";
    player_c << L"    if (frame == 0)\r\n";
    player_c << L"    {\r\n";
    player_c << L"        return;\r\n";
    player_c << L"    }\r\n";
    player_c << L"    " << drv_prefix << L"_Clear();\r\n";
    player_c << L"    for (i = 0u; i < " << led_count_macro << L"; i++)\r\n";
    player_c << L"    {\r\n";
    player_c << L"        " << drv_prefix << L"_SetPixel(i, frame->led[i].r, frame->led[i].g, frame->led[i].b);\r\n";
    player_c << L"    }\r\n";
    player_c << L"    " << drv_prefix << L"_Show();\r\n";
    player_c << L"}\r\n\r\n";
    player_c << L"void " << symbol << L"_play_once(void)\r\n";
    player_c << L"{\r\n";
    player_c << L"    uint16_t frame_index;\r\n\r\n";
    player_c << L"    for (frame_index = 0u; frame_index < " << frame_count_macro << L"; frame_index++)\r\n";
    player_c << L"    {\r\n";
    player_c << L"        " << symbol << L"_render_frame(&" << symbol << L"[frame_index]);\r\n";
    player_c << L"        " << drv_prefix << L"_DelayMs(" << symbol << L"[frame_index].duration_ms);\r\n";
    player_c << L"    }\r\n";
    player_c << L"}\r\n\r\n";
    player_c << L"uint16_t " << symbol << L"_step(uint16_t frame_index)\r\n";
    player_c << L"{\r\n";
    player_c << L"    uint16_t next;\r\n\r\n";
    player_c << L"    if (frame_index >= " << frame_count_macro << L")\r\n";
    player_c << L"    {\r\n";
    player_c << L"        frame_index = 0u;\r\n";
    player_c << L"    }\r\n";
    player_c << L"    " << symbol << L"_render_frame(&" << symbol << L"[frame_index]);\r\n";
    player_c << L"    next = (uint16_t)(frame_index + 1u);\r\n";
    player_c << L"    if (next >= " << frame_count_macro << L")\r\n";
    player_c << L"    {\r\n";
    player_c << L"        next = 0u;\r\n";
    player_c << L"    }\r\n";
    player_c << L"    return next;\r\n";
    player_c << L"}\r\n";

    files.push_back(GeneratedSourceFile{drv_h_name, drv_h.str()});
    files.push_back(GeneratedSourceFile{drv_c_name, drv_c.str()});
    files.push_back(GeneratedSourceFile{player_h_name, player_h.str()});
    files.push_back(GeneratedSourceFile{player_c_name, player_c.str()});
    return files;
}

std::vector<GeneratedSourceFile> GenerateLlsiSourceFiles(const RgbPattern& pattern, const std::wstring& symbol_name) {
    return GenerateSourceFiles(pattern, symbol_name, LedOutputBackend::Llsi);
}

std::wstring GeneratePatternCode(
    const RgbPattern& pattern,
    const std::wstring& symbol_name,
    LedOutputBackend backend
) {
    const std::vector<GeneratedSourceFile> files = GenerateSourceFiles(pattern, symbol_name, backend);
    std::wstringstream ss;
    size_t i = 0u;

    for (i = 0u; i < files.size(); ++i) {
        if (i > 0u) {
            ss << L"\r\n\r\n";
        }
        ss << L"/* ===== " << files[i].file_name << L" ===== */\r\n";
        ss << files[i].text;
    }
    return ss.str();
}

bool SaveTextFile(const std::wstring& path, const std::wstring& text, std::wstring* error) {
    FILE* fp = nullptr;

    if (_wfopen_s(&fp, path.c_str(), L"wt") != 0 || fp == nullptr) {
        if (error != nullptr) {
            *error = L"Cannot open file: " + path;
        }
        return false;
    }
    if (fwprintf(fp, L"%ls", text.c_str()) < 0) {
        fclose(fp);
        if (error != nullptr) {
            *error = L"Write failed: " + path;
        }
        return false;
    }
    fclose(fp);
    return true;
}

bool SaveGeneratedSourceFiles(const std::wstring& directory, const std::vector<GeneratedSourceFile>& files, std::wstring* error) {
    size_t i = 0u;

    if (directory.empty()) {
        if (error != nullptr) {
            *error = L"Output directory is empty.";
        }
        return false;
    }
    for (i = 0u; i < files.size(); ++i) {
        const std::wstring path = JoinPath(directory, files[i].file_name);
        if (!SaveTextFile(path, files[i].text, error)) {
            return false;
        }
    }
    return true;
}

bool SavePatternFile(const RgbPattern& pattern, const std::wstring& path, std::wstring* error) {
    std::wstringstream ss;
    size_t frame_index = 0u;

    ss << L"RGB_PATTERN_V1\r\n";
    ss << L"name=" << pattern.name << L"\r\n";
    ss << L"led_count=" << pattern.led_count << L"\r\n";
    ss << L"shape=" << ShapePersistName(pattern.shape) << L"\r\n";
    ss << L"start_index=" << pattern.start_index << L"\r\n";
    ss << L"direction=" << DirectionPersistName(pattern.direction) << L"\r\n";
    ss << L"path_mode=" << PathModePersistName(pattern.path_mode) << L"\r\n";
    for (frame_index = 0u; frame_index < pattern.frames.size(); ++frame_index) {
        const PatternFrame& frame = pattern.frames[frame_index];
        size_t led = 0u;
        ss << L"frame " << frame_index << L" duration=" << static_cast<unsigned int>(frame.duration_ms) << L"\r\n";
        ss << L"leds=";
        for (led = 0u; led < frame.leds.size(); ++led) {
            const RgbColor& c = frame.leds[led];
            wchar_t color_text[16] = {};
            swprintf_s(color_text, L"%02X%02X%02X",
                       static_cast<unsigned int>(c.r),
                       static_cast<unsigned int>(c.g),
                       static_cast<unsigned int>(c.b));
            if (led > 0u) {
                ss << L" ";
            }
            ss << color_text;
        }
        ss << L"\r\n";
    }
    return SaveTextFile(path, ss.str(), error);
}

bool LoadPatternFile(const std::wstring& path, RgbPattern* out, std::wstring* error) {
    FILE* fp = nullptr;
    wchar_t buffer[8192] = {};
    RgbPattern pattern;
    int current_frame = -1;
    bool saw_header = false;

    if (out == nullptr) {
        if (error != nullptr) {
            *error = L"Output pointer is null.";
        }
        return false;
    }
    if (_wfopen_s(&fp, path.c_str(), L"rt") != 0 || fp == nullptr) {
        if (error != nullptr) {
            *error = L"Cannot open file: " + path;
        }
        return false;
    }

    pattern.frames.clear();
    try {
        while (fgetws(buffer, static_cast<int>(sizeof(buffer) / sizeof(buffer[0])), fp) != nullptr) {
            std::wstring line = Trim(buffer);
            if (line.empty()) {
                continue;
            }
            if (!saw_header) {
                if (line != L"RGB_PATTERN_V1") {
                    fclose(fp);
                    if (error != nullptr) {
                        *error = L"Unsupported pattern file header.";
                    }
                    return false;
                }
                saw_header = true;
                continue;
            }

            if (StartsWith(line, L"name=")) {
                pattern.name = ValueAfter(line, L"name=");
            } else if (StartsWith(line, L"led_count=")) {
                pattern.led_count = ClampInt(ParseInt(ValueAfter(line, L"led_count=")), kMinLedCount, kMaxLedCount);
            } else if (StartsWith(line, L"shape=")) {
                pattern.shape = ParseShapeName(ValueAfter(line, L"shape="));
            } else if (StartsWith(line, L"start_index=")) {
                pattern.start_index = ParseInt(ValueAfter(line, L"start_index="));
            } else if (StartsWith(line, L"direction=")) {
                pattern.direction = ParseDirectionName(ValueAfter(line, L"direction="));
            } else if (StartsWith(line, L"path_mode=")) {
                pattern.path_mode = ParsePathModeName(ValueAfter(line, L"path_mode="));
            } else if (StartsWith(line, L"frame ")) {
                const size_t pos = line.find(L"duration=");
                PatternFrame frame;
                frame.duration_ms = kDefaultFrameDurationMs;
                frame.leds.assign(static_cast<size_t>(pattern.led_count), RgbColor{});
                if (pos != std::wstring::npos) {
                    frame.duration_ms = static_cast<std::uint16_t>(
                        ClampInt(ParseInt(line.substr(pos + 9u)), 1, 60000));
                }
                pattern.frames.push_back(frame);
                current_frame = static_cast<int>(pattern.frames.size()) - 1;
            } else if (StartsWith(line, L"leds=") && current_frame >= 0) {
                std::wistringstream ls(ValueAfter(line, L"leds="));
                std::wstring token;
                size_t led = 0u;
                while (ls >> token && led < pattern.frames[static_cast<size_t>(current_frame)].leds.size()) {
                    RgbColor color;
                    if (ParseColorToken(token, &color)) {
                        pattern.frames[static_cast<size_t>(current_frame)].leds[led] = color;
                    }
                    ++led;
                }
            }
        }
    } catch (...) {
        fclose(fp);
        if (error != nullptr) {
            *error = L"Invalid numeric field in pattern file.";
        }
        return false;
    }
    fclose(fp);

    if (!saw_header) {
        if (error != nullptr) {
            *error = L"Empty pattern file.";
        }
        return false;
    }
    EnsurePatternShape(&pattern, pattern.led_count, pattern.shape);
    *out = pattern;
    return true;
}

} // namespace rgb_tool::core
