#include "src/ui/rgb_led_tab.h"
#include <cstdio>
#include <stdexcept>

static void Check(bool result, const char* name) {
    if (!result) throw std::runtime_error(name);
}
static HWND ChildWithText(HWND parent, const wchar_t* text) {
    for (HWND child = GetWindow(parent, GW_CHILD); child != nullptr; child = GetWindow(child, GW_HWNDNEXT)) {
        wchar_t value[80] = {};
        GetWindowTextW(child, value, 80);
        if (wcscmp(value, text) == 0) return child;
    }
    return nullptr;
}
static void Press(CRgbLedTab& tab, const wchar_t* caption) {
    const HWND button = ChildWithText(tab.GetSafeHwnd(), caption);
    Check(button != nullptr, "button exists");
    tab.SendMessageW(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(button), BN_CLICKED), reinterpret_cast<LPARAM>(button));
}
CWinApp test_app;
int wmain(int argc, wchar_t** argv) {
    if (argc != 2 || !AfxWinInit(GetModuleHandleW(nullptr), nullptr, GetCommandLineW(), 0)) return 2;
    INITCOMMONCONTROLSEX common{sizeof(common), ICC_WIN95_CLASSES | ICC_BAR_CLASSES};
    InitCommonControlsEx(&common);
    CWnd parent;
    CRgbLedTab tab;
    int result = 0;
    try {
        Check(parent.CreateEx(0u, AfxRegisterWndClass(0u), L"RGB contract isolated parent", WS_OVERLAPPED,
                              CRect(0, 0, 1360, 820), nullptr, 0u) != FALSE, "parent create");
        Check(tab.Create(&parent, CRect(0, 0, 1300, 740), 1u) != FALSE, "MFC pattern tab create");
        using namespace rgb_tool::core;
        const auto pattern = MakePreset(L"Moving Dot", 8, LedShape::UShape, 4,
                                        LedDirection::Reverse, LedPathMode::Bounce);
        std::wstring error;
        const auto path = std::wstring(argv[1]) + L"/ui.rgbpat";
        Check(SavePatternFile(pattern, path, &error), "UI fixture save");
        Check(tab.LoadPatternFromFile(path, &error), "UI fixture load");
        const auto expected = GeneratePatternCode(pattern, L"g_rgb_pattern", LedOutputBackend::Llsi);
        Check(tab.CurrentCode() == expected, "loading updates controls without remapping source");
        HWND code = nullptr;
        for (HWND child = GetWindow(tab.GetSafeHwnd(), GW_CHILD); child != nullptr; child = GetWindow(child, GW_HWNDNEXT)) {
            wchar_t cls[30] = {};
            GetClassNameW(child, cls, 30);
            if (_wcsicmp(cls, L"Edit") == 0 && (GetWindowLongPtrW(child, GWL_STYLE) & ES_MULTILINE)) code = child;
        }
        Check(code != nullptr, "code edit exists");
        SendMessageW(code, WM_VSCROLL, SB_BOTTOM, 0);
        const int scroll = GetScrollPos(code, SB_VERT);
        Check(scroll > 0, "code scrolled before playback");
        Press(tab, L"Play");
        for (int tick = 0; tick < 40; ++tick) tab.SendMessageW(WM_TIMER, 1u, 0);
        Check(tab.CurrentCode() == expected, "playback leaves source unchanged");
        Check(GetScrollPos(code, SB_VERT) == scroll, "playback preserves code scroll");
        Press(tab, L"Stop");
        const HWND slider = FindWindowExW(tab.GetSafeHwnd(), nullptr, TRACKBAR_CLASSW, nullptr);
        Check(slider != nullptr, "playback slider exists");
        const LRESULT stopped_position = SendMessageW(slider, TBM_GETPOS, 0, 0);
        for (int tick = 0; tick < 3; ++tick) tab.SendMessageW(WM_TIMER, 1u, 0);
        Check(SendMessageW(slider, TBM_GETPOS, 0, 0) == stopped_position, "late timers preserve stopped position");
        Check(tab.CurrentCode() == expected, "late timers preserve source");
        Check(tab.SavePatternToFile(path, &error), "UI save");
        RgbPattern roundtrip;
        Check(LoadPatternFile(path, &roundtrip, &error), "UI round trip");
        Check(GeneratePatternCode(roundtrip, L"g_rgb_pattern", LedOutputBackend::Llsi) == expected, "UI persisted source unchanged");
        Check(!tab.LoadPatternFromFile(std::wstring(argv[1]) + L"/invalid.rgbpat", &error) && tab.CurrentCode() == expected,
              "failed load preserves last accepted source");
        auto state = rgb_tool::core::AppState::Default();
        state.pattern.preset.assign(1024u, L'x');
        tab.LoadState(state);
        const auto fallback = MakePreset(L"Moving Dot", kDefaultLedCount, LedShape::Line);
        Check(tab.CurrentCode() == GeneratePatternCode(fallback, L"g_rgb_pattern", LedOutputBackend::Llsi),
              "unknown or overlong INI preset restores a consistent safe default");
        std::puts("PASS: hidden MFC load/play/stop/late-timer/save/failure, code-scroll and invalid-INI preset checks");
    } catch (const std::exception& exception) {
        std::fprintf(stderr, "FAIL: %s\n", exception.what());
        result = 1;
    }
    if (IsWindow(tab.GetSafeHwnd())) tab.DestroyWindow();
    if (IsWindow(parent.GetSafeHwnd())) parent.DestroyWindow();
    return result;
}
