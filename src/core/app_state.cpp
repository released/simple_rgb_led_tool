#include "app_state.h"

#include <cwctype>

namespace rgb_tool::core {
namespace {

bool IsAbsolutePath(const std::wstring& path) {
    if (path.size() >= 2u && path[1] == L':') {
        return true;
    }
    return path.size() >= 2u &&
           ((path[0] == L'\\' && path[1] == L'\\') ||
            (path[0] == L'/' && path[1] == L'/'));
}

std::wstring LowerPath(std::wstring path) {
    for (wchar_t& ch : path) {
        if (ch == L'/') {
            ch = L'\\';
        } else {
            ch = static_cast<wchar_t>(towlower(ch));
        }
    }
    return path;
}

std::wstring ToExeRelativePath(const std::wstring& path) {
    if (path.empty() || !IsAbsolutePath(path)) {
        return path;
    }

    const std::wstring root = mfc_tool::config::IniManager::DefaultIniPath(L"");
    const std::wstring lower_path = LowerPath(path);
    const std::wstring lower_root = LowerPath(root);
    if (lower_root.empty() || lower_path.find(lower_root) != 0u) {
        return path;
    }
    return path.substr(root.size());
}

std::wstring GetValue(
    const mfc_tool::config::IniData& data,
    const std::wstring& section,
    const std::wstring& key,
    const std::wstring& fallback
) {
    auto sec_it = data.find(section);
    if (sec_it == data.end()) {
        return fallback;
    }
    auto key_it = sec_it->second.find(key);
    if (key_it == sec_it->second.end()) {
        return fallback;
    }
    return key_it->second;
}

} // namespace

AppState AppState::Default() {
    return AppState{};
}

mfc_tool::config::IniData AppState::ToIniData(const std::wstring& ini_path) const {
    mfc_tool::config::IniData out;
    out[L"APP"] = {
        {L"ini_path", ToExeRelativePath(ini_path)}
    };

    out[L"UI"] = {
        {L"save_log_checked", ui.save_log_checked ? L"1" : L"0"},
        {L"last_pattern_path", ToExeRelativePath(ui.last_pattern_path)},
        {L"last_code_path", ToExeRelativePath(ui.last_code_path)}
    };

    out[L"PATTERN"] = {
        {L"led_count", pattern.led_count},
        {L"shape", pattern.shape},
        {L"preset", pattern.preset},
        {L"symbol_name", pattern.symbol_name},
        {L"start_index", pattern.start_index},
        {L"direction", pattern.direction},
        {L"path_mode", pattern.path_mode},
        {L"backend", pattern.backend},
        {L"color2_red", pattern.color2_red},
        {L"color2_green", pattern.color2_green},
        {L"color2_blue", pattern.color2_blue},
        {L"tail_length", pattern.tail_length},
        {L"trail_fade", pattern.trail_fade},
        {L"chase_spacing", pattern.chase_spacing},
        {L"repeat_count", pattern.repeat_count}
    };

    out[L"CONTROL"] = {
        {L"led_count", control.led_count},
        {L"order", control.order},
        {L"brightness", control.brightness},
        {L"effect", control.effect},
        {L"red", control.red},
        {L"green", control.green},
        {L"blue", control.blue},
        {L"speed", control.speed},
        {L"direction", control.direction}
    };

    return out;
}

void AppState::ApplyIniData(const mfc_tool::config::IniData& data) {
    ui.save_log_checked = GetValue(data, L"UI", L"save_log_checked", ui.save_log_checked ? L"1" : L"0") == L"1";
    ui.last_pattern_path = GetValue(data, L"UI", L"last_pattern_path", ui.last_pattern_path);
    ui.last_code_path = GetValue(data, L"UI", L"last_code_path", ui.last_code_path);

    pattern.led_count = GetValue(data, L"PATTERN", L"led_count", pattern.led_count);
    pattern.shape = GetValue(data, L"PATTERN", L"shape", pattern.shape);
    pattern.preset = GetValue(data, L"PATTERN", L"preset", pattern.preset);
    pattern.symbol_name = GetValue(data, L"PATTERN", L"symbol_name", pattern.symbol_name);
    pattern.start_index = GetValue(data, L"PATTERN", L"start_index", pattern.start_index);
    pattern.direction = GetValue(data, L"PATTERN", L"direction", pattern.direction);
    pattern.path_mode = GetValue(data, L"PATTERN", L"path_mode", pattern.path_mode);
    pattern.backend = GetValue(data, L"PATTERN", L"backend", pattern.backend);
    pattern.color2_red = GetValue(data, L"PATTERN", L"color2_red", pattern.color2_red);
    pattern.color2_green = GetValue(data, L"PATTERN", L"color2_green", pattern.color2_green);
    pattern.color2_blue = GetValue(data, L"PATTERN", L"color2_blue", pattern.color2_blue);
    pattern.tail_length = GetValue(data, L"PATTERN", L"tail_length", pattern.tail_length);
    pattern.trail_fade = GetValue(data, L"PATTERN", L"trail_fade", pattern.trail_fade);
    pattern.chase_spacing = GetValue(data, L"PATTERN", L"chase_spacing", pattern.chase_spacing);
    pattern.repeat_count = GetValue(data, L"PATTERN", L"repeat_count", pattern.repeat_count);

    control.led_count = GetValue(data, L"CONTROL", L"led_count", control.led_count);
    control.order = GetValue(data, L"CONTROL", L"order", control.order);
    control.brightness = GetValue(data, L"CONTROL", L"brightness", control.brightness);
    control.effect = GetValue(data, L"CONTROL", L"effect", control.effect);
    control.red = GetValue(data, L"CONTROL", L"red", control.red);
    control.green = GetValue(data, L"CONTROL", L"green", control.green);
    control.blue = GetValue(data, L"CONTROL", L"blue", control.blue);
    control.speed = GetValue(data, L"CONTROL", L"speed", control.speed);
    control.direction = GetValue(data, L"CONTROL", L"direction", control.direction);
}

} // namespace rgb_tool::core
