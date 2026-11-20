#include "rgb_pattern.h"

#include <algorithm>
#include <cstdio>
#include <cwctype>
#include <sstream>

#include "text_utils.h"

namespace rgb_tool::core {
namespace {

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

std::vector<std::wstring> PresetNames() {
    return {
        L"Static",
        L"Breathing",
        L"Blink",
        L"Gradient",
        L"Flow (Rainbow)",
        L"Moving Dot",
        L"Theater Chase",
        L"Color Wipe",
        L"Blue Comet",
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

bool ValidatePattern(const RgbPattern& pattern, std::wstring* error) {
    const auto reject = [error](const wchar_t* message) {
        if (error != nullptr) *error = message;
        return false;
    };
    if (pattern.led_count < kMinLedCount || pattern.led_count > kMaxLedCount ||
        pattern.start_index < 0 || pattern.start_index >= pattern.led_count) {
        return reject(L"Invalid LED count or start index.");
    }
    if (pattern.shape < LedShape::Line || pattern.shape > LedShape::UShape ||
        (pattern.direction != LedDirection::Forward && pattern.direction != LedDirection::Reverse) ||
        (pattern.path_mode != LedPathMode::Loop && pattern.path_mode != LedPathMode::Bounce)) {
        return reject(L"Invalid pattern layout or path option.");
    }
    if (pattern.name.size() > kMaxPatternNameLength ||
        pattern.name.find_first_of(std::wstring(L"\r\n\0", 3u)) != std::wstring::npos) {
        return reject(L"Pattern name must be a single line of at most 255 characters.");
    }
    if (pattern.frames.empty() || pattern.frames.size() > kMaxPatternFrames) {
        return reject(L"Pattern must contain 1 to 4096 frames.");
    }
    for (const PatternFrame& frame : pattern.frames) {
        if (frame.leds.size() != static_cast<std::size_t>(pattern.led_count) ||
            frame.duration_ms < 1u || frame.duration_ms > 60000u) {
            return reject(L"Invalid frame size or duration (1 to 60000 ms).");
        }
    }
    if (error != nullptr) error->clear();
    return true;
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
    LedPathMode path_mode,
    RgbColor primary,
    RgbColor secondary,
    int tail_length,
    int trail_fade,
    int chase_spacing,
    int repeat_count
) {
    const int count = ClampInt(led_count, kMinLedCount, kMaxLedCount);
    const std::wstring name = preset_name.empty() ? L"Moving Dot" : preset_name;
    const int safe_start = NormalizeStartIndex(start_index, count);
    const std::vector<int> loop_order = BuildLoopLedOrder(count, safe_start, direction);
    const std::vector<int> motion_path = BuildMotionLedPath(count, safe_start, direction, path_mode);
    RgbPattern pattern;
    int i = 0;
    tail_length = ClampInt(tail_length, 1, count);
    trail_fade = ClampInt(trail_fade, 1, 255);
    chase_spacing = ClampInt(chase_spacing, 1, count);
    repeat_count = ClampInt(repeat_count, 1, 16);

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

    if (name == L"Static") {
        return MakeSolid(name, count, shape, primary, safe_start, direction, path_mode);
    }

    if (name == L"Breathing") {
        for (i = 0; i < 32; ++i) {
            const int level = (i < 16) ? i * 17 : (31 - i) * 17;
            pattern.frames.push_back(MakeFrame(count, 55u, RgbColor{
                static_cast<std::uint8_t>((static_cast<unsigned int>(primary.r) * level) / 255u),
                static_cast<std::uint8_t>((static_cast<unsigned int>(primary.g) * level) / 255u),
                static_cast<std::uint8_t>((static_cast<unsigned int>(primary.b) * level) / 255u)}));
        }
        return pattern;
    }

    if (name == L"Blink") {
        for (i = 0; i < repeat_count; ++i) {
            pattern.frames.push_back(MakeFrame(count, 250u, primary));
            pattern.frames.push_back(MakeFrame(count, 250u, secondary));
        }
        return pattern;
    }

    if (name == L"Gradient") {
        for (i = 0; i < 16; ++i) {
            PatternFrame frame = MakeFrame(count, 70u, secondary);
            int led = 0;
            for (led = 0; led < count; ++led) {
                const int phase = ((led * 256) / (std::max)(1, count) + i * 16) & 255;
                const int level = (phase < 128) ? phase * 2 : (255 - phase) * 2;
                frame.leds[static_cast<size_t>(loop_order[static_cast<size_t>(led)])] = RgbColor{
                    static_cast<std::uint8_t>((static_cast<unsigned int>(primary.r) * level) / 255u),
                    static_cast<std::uint8_t>((static_cast<unsigned int>(primary.g) * level) / 255u),
                    static_cast<std::uint8_t>((static_cast<unsigned int>(primary.b) * level) / 255u)};
            }
            pattern.frames.push_back(frame);
        }
        return pattern;
    }

    if (name == L"Flow (Rainbow)" || name == L"Rainbow Cycle") {
        for (i = 0; i < 16; ++i) {
            AddRainbowFrame(&pattern, i, 16);
        }
        return pattern;
    }

    if (name == L"Theater Chase") {
        for (i = 0; i < chase_spacing * repeat_count; ++i) {
            PatternFrame frame = MakeFrame(count, 90u, RgbColor{0u, 0u, 0u});
            int led = 0;
            for (led = 0; led < count; ++led) {
                if (((led + i) % chase_spacing) == 0) {
                    frame.leds[static_cast<size_t>(loop_order[static_cast<size_t>(led)])] = primary;
                }
            }
            pattern.frames.push_back(frame);
        }
        return pattern;
    }

    if (name == L"Color Wipe" || name == L"Color Wipe Red") {
        for (i = 0; i <= static_cast<int>(motion_path.size()); ++i) {
            PatternFrame frame = MakeFrame(count, 55u, RgbColor{0u, 0u, 0u});
            int led = 0;
            for (led = 0; led < i && led < static_cast<int>(motion_path.size()); ++led) {
                frame.leds[static_cast<size_t>(motion_path[static_cast<size_t>(led)])] = primary;
            }
            pattern.frames.push_back(frame);
        }
        return pattern;
    }

    if (name == L"Blue Comet" || name == L"Breathing Blue") {
        for (i = 0; i < static_cast<int>(motion_path.size()); ++i) {
            PatternFrame frame = MakeFrame(count, 55u, RgbColor{0u, 0u, 0u});
            int tail = 0;
            if (!motion_path.empty()) {
                for (tail = 0; tail < tail_length; ++tail) {
                    const int path_index = (i - tail + static_cast<int>(motion_path.size())) %
                                           static_cast<int>(motion_path.size());
                    int level = 255;
                    int step = 0;
                    for (step = 0; step < tail; ++step) level = (level * trail_fade) / 255;
                    frame.leds[static_cast<size_t>(motion_path[static_cast<size_t>(path_index)])] = RgbColor{
                        static_cast<std::uint8_t>((static_cast<unsigned int>(primary.r) * level) / 255u),
                        static_cast<std::uint8_t>((static_cast<unsigned int>(primary.g) * level) / 255u),
                        static_cast<std::uint8_t>((static_cast<unsigned int>(primary.b) * level) / 255u)};
                }
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
        frame.leds[static_cast<size_t>(motion_path[static_cast<size_t>(i)])] = primary;
        pattern.frames.push_back(frame);
    }
    return pattern;
}

} // namespace rgb_tool::core
