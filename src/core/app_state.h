#pragma once

#include <string>

#include "../config/ini_manager.h"

namespace rgb_tool::core {

struct UiState {
    bool save_log_checked = false;
    std::wstring last_pattern_path;
    std::wstring last_code_path;
};

struct PatternUiState {
    std::wstring led_count = L"24";
    std::wstring shape = L"Line";
    std::wstring preset = L"Moving Dot";
    std::wstring symbol_name = L"g_rgb_pattern";
    std::wstring start_index = L"0";
    std::wstring direction = L"Forward";
    std::wstring path_mode = L"Loop";
    std::wstring backend = L"LLSI";
};

struct ControlUiState {
    std::wstring led_count = L"8";
    std::wstring order = L"GRB";
    std::wstring brightness = L"128";
    std::wstring effect = L"Static";
    std::wstring red = L"255";
    std::wstring green = L"80";
    std::wstring blue = L"0";
    std::wstring speed = L"128";
    std::wstring direction = L"Forward";
};

struct AppState {
    UiState ui;
    PatternUiState pattern;
    ControlUiState control;

    static AppState Default();

    mfc_tool::config::IniData ToIniData(const std::wstring& ini_path) const;
    void ApplyIniData(const mfc_tool::config::IniData& data);
};

} // namespace rgb_tool::core
