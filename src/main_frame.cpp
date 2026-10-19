#include "main_frame.h"

#include <afxdlgs.h>

#include <array>
#include <algorithm>
#include <cwchar>

#include "build_info.generated.h"
#include "resource.h"
#include "ui/layout_utils.h"

namespace {

constexpr int kUiLogEditLimitChars = 512 * 1024;
constexpr int kUiLogTrimThresholdChars = 160 * 1024;
constexpr int kUiLogTrimTargetChars = 120 * 1024;

std::wstring NowTimeText() {
    SYSTEMTIME st = {};
    GetLocalTime(&st);

    wchar_t buf[32] = {};
    swprintf_s(buf, L"%02u:%02u:%02u", st.wHour, st.wMinute, st.wSecond);
    return std::wstring(buf);
}

} // namespace

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_BN_CLICKED(ID_TOP_SAVE_INI_BTN, &CMainFrame::OnBnClickedSaveIni)
    ON_BN_CLICKED(ID_TOP_LOAD_INI_BTN, &CMainFrame::OnBnClickedLoadIni)
    ON_BN_CLICKED(ID_TOP_RESET_INI_BTN, &CMainFrame::OnBnClickedResetIni)
    ON_BN_CLICKED(ID_TOP_LOAD_PATTERN_BTN, &CMainFrame::OnBnClickedLoadPattern)
    ON_BN_CLICKED(ID_TOP_SAVE_PATTERN_BTN, &CMainFrame::OnBnClickedSavePattern)
    ON_BN_CLICKED(ID_TOP_COPY_CODE_BTN, &CMainFrame::OnBnClickedCopyCode)
    ON_BN_CLICKED(ID_TOP_SAVE_CODE_BTN, &CMainFrame::OnBnClickedSaveCode)
    ON_BN_CLICKED(ID_LOG_SAVE_BTN, &CMainFrame::OnBnClickedSaveLog)
    ON_BN_CLICKED(ID_LOG_SAVE_CHECK, &CMainFrame::OnBnClickedSaveLogCheck)
    ON_BN_CLICKED(ID_LOG_CLEAR_BTN, &CMainFrame::OnBnClickedClearLog)
    ON_NOTIFY(TCN_SELCHANGE, ID_TAB_CTRL, &CMainFrame::OnTabChanged)
    ON_WM_CLOSE()
END_MESSAGE_MAP()

CMainFrame::CMainFrame()
    : ini_(mfc_tool::config::IniManager::DefaultIniPath(L"rgb_led_tool.ini")),
      state_(rgb_tool::core::AppState::Default()) {
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs) {
    if (!CFrameWnd::PreCreateWindow(cs)) {
        return FALSE;
    }
    cs.style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    HICON app_icon = AfxGetApp()->LoadIconW(IDR_MAINFRAME);
    cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, ::LoadCursor(nullptr, IDC_ARROW),
                                       reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), app_icon);
    return TRUE;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) {
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1) {
        return -1;
    }

    ui_font_.CreatePointFont(90, L"Segoe UI");
    HICON app_icon = AfxGetApp()->LoadIconW(IDR_MAINFRAME);
    SetIcon(app_icon, TRUE);
    SetIcon(app_icon, FALSE);

    auto create_or_fail = [this](BOOL ok, const wchar_t* name) -> bool {
        if (!ok) {
            AppendLog(std::wstring(L"Create control failed: ") + name);
            return false;
        }
        return true;
    };

    if (!create_or_fail(save_ini_btn_.Create(L"Save INI", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, ID_TOP_SAVE_INI_BTN), L"Save INI")) return -1;
    if (!create_or_fail(load_ini_btn_.Create(L"Load INI", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, ID_TOP_LOAD_INI_BTN), L"Load INI")) return -1;
    if (!create_or_fail(reset_ini_btn_.Create(L"Reset INI", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, ID_TOP_RESET_INI_BTN), L"Reset INI")) return -1;
    if (!create_or_fail(load_pattern_btn_.Create(L"Load Pattern", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, ID_TOP_LOAD_PATTERN_BTN), L"Load Pattern")) return -1;
    if (!create_or_fail(save_pattern_btn_.Create(L"Save Pattern", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, ID_TOP_SAVE_PATTERN_BTN), L"Save Pattern")) return -1;
    if (!create_or_fail(copy_code_btn_.Create(L"Copy Code", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, ID_TOP_COPY_CODE_BTN), L"Copy Code")) return -1;
    if (!create_or_fail(save_code_btn_.Create(L"Save Code", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, ID_TOP_SAVE_CODE_BTN), L"Save Code")) return -1;
    if (!create_or_fail(ini_path_title_.Create(L"INI:", WS_CHILD | WS_VISIBLE, CRect(), this, ID_TOP_INI_PATH_TITLE), L"INI path title")) return -1;
    if (!create_or_fail(ini_path_value_.Create(L"-", WS_CHILD | WS_VISIBLE | SS_PATHELLIPSIS, CRect(), this, ID_TOP_INI_PATH_VALUE), L"INI path value")) return -1;
    if (!create_or_fail(build_info_title_.Create(L"Build:", WS_CHILD | WS_VISIBLE, CRect(), this, ID_TOP_BUILD_INFO_TITLE), L"Build info title")) return -1;
    if (!create_or_fail(build_info_value_.Create(L"-", WS_CHILD | WS_VISIBLE | SS_PATHELLIPSIS, CRect(), this, ID_TOP_BUILD_INFO_VALUE), L"Build info value")) return -1;
    if (!create_or_fail(tab_ctrl_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | TCS_FIXEDWIDTH, CRect(), this, ID_TAB_CTRL), L"Tab ctrl")) return -1;
    if (!create_or_fail(save_log_check_.Create(L"Save Log", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, CRect(), this, ID_LOG_SAVE_CHECK), L"Save Log checkbox")) return -1;
    if (!create_or_fail(save_log_btn_.Create(L"Save Log", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, ID_LOG_SAVE_BTN), L"Save Log button")) return -1;
    if (!create_or_fail(clear_log_btn_.Create(L"Clear Log", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, ID_LOG_CLEAR_BTN), L"Clear Log button")) return -1;
    if (!create_or_fail(log_edit_.Create(WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_BORDER,
                                         CRect(), this, ID_LOG_EDIT), L"Log edit")) return -1;
    log_edit_.LimitText(kUiLogEditLimitChars);

    CRect tab_dummy(0, 0, 100, 100);
    if (!control_tab_.Create(&tab_ctrl_, tab_dummy, ID_TAB_CONTROL)) return -1;
    if (!rgb_tab_.Create(&tab_ctrl_, tab_dummy, ID_TAB_RGB_LED)) return -1;
    control_tab_.Bind([this](const std::wstring& msg) { AppendLog(msg); });
    rgb_tab_.Bind([this](const std::wstring& msg) { AppendLog(msg); });

    const std::array<CWnd*, 19> controls = {
        &save_ini_btn_, &load_ini_btn_, &reset_ini_btn_, &load_pattern_btn_, &save_pattern_btn_,
        &copy_code_btn_, &save_code_btn_, &ini_path_title_, &ini_path_value_, &build_info_title_,
        &build_info_value_, &tab_ctrl_, &save_log_check_, &save_log_btn_, &clear_log_btn_,
        &log_edit_, &control_tab_, &rgb_tab_, nullptr
    };
    for (CWnd* w : controls) {
        if (w != nullptr) {
            mfc_tool::ui::SafeSetFont(*w, &ui_font_);
        }
    }

    tab_ctrl_.DeleteAllItems();
    tab_ctrl_.InsertItem(0, L"Control");
    tab_ctrl_.InsertItem(1, L"Pattern / Code");
    tab_ctrl_.SetItemSize(CSize(150, 24));
    tab_ctrl_.SetCurSel(0);

    LoadIni();
    save_log_check_.SetCheck(state_.ui.save_log_checked ? BST_CHECKED : BST_UNCHECKED);
    OnBnClickedSaveLogCheck();
    SetControlText(build_info_value_, std::wstring(L"v") + RGB_LED_TOOL_BUILD_VERSION + L" | " + RGB_LED_TOOL_BUILD_TIME);

    AppendLog(L"Simple RGB LED Tool initialized.");
    AppendLog(L"Connect the M032 USB HID controller, then configure and apply a live LED effect.");
    UpdateIniPathUi();

    CRect rc;
    GetClientRect(&rc);
    LayoutControls(rc.Width(), rc.Height());
    RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    return 0;
}

void CMainFrame::OnSize(UINT nType, int cx, int cy) {
    CFrameWnd::OnSize(nType, cx, cy);
    LayoutControls(cx, cy);
}

void CMainFrame::LayoutControls(int cx, int cy) {
    if (cx <= 0 || cy <= 0) {
        return;
    }

    const int margin = 8;
    const int gap = 6;
    const int row_h = 26;
    const int log_h = (std::max)(116, cy / 6);
    int btn_w = 96;
    int y = margin;
    int x = margin;

    btn_w = (std::max)(btn_w, mfc_tool::ui::MeasureButtonMinWidth(load_pattern_btn_));
    btn_w = (std::max)(btn_w, mfc_tool::ui::MeasureButtonMinWidth(save_pattern_btn_));

    mfc_tool::ui::SafeMoveWindow(save_ini_btn_, x, y, 86, row_h);
    x += 86 + gap;
    mfc_tool::ui::SafeMoveWindow(load_ini_btn_, x, y, 86, row_h);
    x += 86 + gap;
    mfc_tool::ui::SafeMoveWindow(reset_ini_btn_, x, y, 90, row_h);
    x += 90 + 14;
    mfc_tool::ui::SafeMoveWindow(load_pattern_btn_, x, y, btn_w, row_h);
    x += btn_w + gap;
    mfc_tool::ui::SafeMoveWindow(save_pattern_btn_, x, y, btn_w, row_h);
    x += btn_w + 14;
    mfc_tool::ui::SafeMoveWindow(copy_code_btn_, x, y, 96, row_h);
    x += 96 + gap;
    mfc_tool::ui::SafeMoveWindow(save_code_btn_, x, y, 96, row_h);

    y += row_h + gap;
    x = margin;
    x += mfc_tool::ui::PlaceLabel(ini_path_title_, x, y + 4, 8) + gap;
    {
        const int build_block_w = 360;
        mfc_tool::ui::SafeMoveWindow(ini_path_value_, x, y + 4, (std::max)(120, cx - x - build_block_w - margin), 18);
        x = (std::max)(margin, cx - build_block_w);
    }
    x += mfc_tool::ui::PlaceLabel(build_info_title_, x, y + 4, 8) + gap;
    mfc_tool::ui::SafeMoveWindow(build_info_value_, x, y + 4, cx - x - margin, 18);

    const int top_h = y + row_h + gap;
    const int log_y = cy - log_h - margin;
    const int log_btn_y = log_y - row_h - gap;
    mfc_tool::ui::SafeMoveWindow(save_log_check_, margin, log_btn_y + 3, 110, 20);
    mfc_tool::ui::SafeMoveWindow(save_log_btn_, margin + 116, log_btn_y, 86, row_h);
    mfc_tool::ui::SafeMoveWindow(clear_log_btn_, margin + 208, log_btn_y, 86, row_h);
    mfc_tool::ui::SafeMoveWindow(log_edit_, margin, log_y, cx - margin * 2, cy - log_y - margin);
    mfc_tool::ui::SafeMoveWindow(tab_ctrl_, margin, top_h, cx - margin * 2, (std::max)(180, log_btn_y - top_h - gap));

    CRect rc;
    tab_ctrl_.GetClientRect(&rc);
    tab_ctrl_.AdjustRect(FALSE, &rc);
    rc.DeflateRect(4, 4, 4, 4);
    mfc_tool::ui::SafeMoveWindow(rgb_tab_, rc);
    mfc_tool::ui::SafeMoveWindow(control_tab_, rc);
    UpdateTabVisibility();
}

void CMainFrame::UpdateTabVisibility() {
    if (!::IsWindow(tab_ctrl_.GetSafeHwnd())) {
        return;
    }
    const bool editor = tab_ctrl_.GetCurSel() == 1;
    control_tab_.ShowWindow(editor ? SW_HIDE : SW_SHOW);
    rgb_tab_.ShowWindow(editor ? SW_SHOW : SW_HIDE);
    CWnd* editor_controls[] = {&load_pattern_btn_, &save_pattern_btn_, &copy_code_btn_, &save_code_btn_};
    for (CWnd* control : editor_controls) control->ShowWindow(editor ? SW_SHOW : SW_HIDE);
}

void CMainFrame::OnTabChanged(NMHDR*, LRESULT* result) {
    UpdateTabVisibility();
    Invalidate(TRUE);
    if (result != nullptr) *result = 0;
}

void CMainFrame::AppendLog(const std::wstring& text) {
    const std::wstring line = L"[" + NowTimeText() + L"] " + text;
    logger_.AddLine(line);

    if (!::IsWindow(log_edit_.GetSafeHwnd())) {
        return;
    }
    int len = log_edit_.GetWindowTextLengthW();
    log_edit_.SetSel(len, len);
    log_edit_.ReplaceSel((line + L"\r\n").c_str());
    TrimVisibleLogIfNeeded();
}

void CMainFrame::TrimVisibleLogIfNeeded() {
    const int len = log_edit_.GetWindowTextLengthW();
    if (len <= kUiLogTrimThresholdChars) {
        return;
    }

    CString visible_log;
    log_edit_.GetWindowTextW(visible_log);
    std::wstring text = visible_log.GetString();
    int trim_chars = len - kUiLogTrimTargetChars;
    if (trim_chars <= 0) {
        return;
    }
    std::wstring::size_type trim_end = text.find(L'\n', static_cast<std::wstring::size_type>(trim_chars));
    if (trim_end == std::wstring::npos) {
        trim_end = static_cast<std::wstring::size_type>(trim_chars);
    } else {
        ++trim_end;
    }
    if (trim_end == 0 || trim_end >= text.size()) {
        return;
    }

    log_edit_.SetRedraw(FALSE);
    log_edit_.SetSel(0, static_cast<int>(trim_end));
    log_edit_.ReplaceSel(L"");
    log_edit_.SetSel(log_edit_.GetWindowTextLengthW(), log_edit_.GetWindowTextLengthW());
    log_edit_.SetRedraw(TRUE);
    log_edit_.Invalidate(FALSE);
}

void CMainFrame::UpdateIniPathUi() {
    SetControlText(ini_path_value_, ini_.Path());
}

void CMainFrame::LoadIni() {
    UpdateIniPathUi();
    mfc_tool::config::IniData data;
    std::wstring error;
    if (!ini_.Exists()) {
        SaveIni();
        AppendLog(L"INI created with defaults.");
        return;
    }
    if (!ini_.Load(&data, &error)) {
        AppendLog(L"INI load failed: " + error);
        return;
    }

    state_.ApplyIniData(data);
    save_log_check_.SetCheck(state_.ui.save_log_checked ? BST_CHECKED : BST_UNCHECKED);
    control_tab_.LoadState(state_);
    rgb_tab_.LoadState(state_);
    AppendLog(L"INI loaded.");
}

void CMainFrame::SaveIni() {
    control_tab_.SaveState(&state_);
    rgb_tab_.SaveState(&state_);
    state_.ui.save_log_checked = (save_log_check_.GetCheck() == BST_CHECKED);

    std::wstring error;
    auto data = state_.ToIniData(ini_.Path());
    if (!ini_.Save(data, &error)) {
        AppendLog(L"INI save failed: " + error);
        return;
    }
    AppendLog(L"INI saved.");
}

void CMainFrame::SaveIniTo(const std::wstring& path) {
    ini_.SetPath(path);
    UpdateIniPathUi();
    SaveIni();
}

bool CMainFrame::TryLoadIniFrom(const std::wstring& path) {
    ini_.SetPath(path);
    UpdateIniPathUi();
    LoadIni();
    return true;
}

void CMainFrame::ResetIniToDefault() {
    state_ = rgb_tool::core::AppState::Default();
    control_tab_.LoadState(state_);
    rgb_tab_.LoadState(state_);
    save_log_check_.SetCheck(BST_UNCHECKED);
    SaveIni();
}

void CMainFrame::SaveLogToFile(const std::wstring& path) {
    std::wstring error;
    if (!logger_.SaveToFile(path, &error)) {
        ShowErrorBox(L"Save Log Error", error);
        return;
    }
    AppendLog(L"Log saved: " + path);
}

std::wstring CMainFrame::GetControlText(const CWnd& w) const {
    CString s;
    const_cast<CWnd&>(w).GetWindowTextW(s);
    return s.GetString();
}

void CMainFrame::SetControlText(CWnd& w, const std::wstring& text) {
    if (::IsWindow(w.GetSafeHwnd())) {
        w.SetWindowTextW(text.c_str());
    }
}

void CMainFrame::ShowErrorBox(const std::wstring& title, const std::wstring& message) {
    ::MessageBoxW(nullptr, message.c_str(), title.c_str(), MB_ICONERROR | MB_OK);
}

void CMainFrame::OnBnClickedSaveIni() {
    CFileDialog dlg(FALSE, L"ini", L"rgb_led_tool.ini",
                    OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
                    L"INI Files (*.ini)|*.ini|All Files (*.*)|*.*||", this);
    if (dlg.DoModal() == IDOK) {
        SaveIniTo(dlg.GetPathName().GetString());
    }
}

void CMainFrame::OnBnClickedLoadIni() {
    CFileDialog dlg(TRUE, L"ini", nullptr,
                    OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
                    L"INI Files (*.ini)|*.ini|All Files (*.*)|*.*||", this);
    if (dlg.DoModal() == IDOK) {
        TryLoadIniFrom(dlg.GetPathName().GetString());
    }
}

void CMainFrame::OnBnClickedResetIni() {
    if (MessageBoxW(L"Reset INI settings to defaults?", L"Reset INI", MB_ICONQUESTION | MB_YESNO) != IDYES) {
        return;
    }
    ResetIniToDefault();
    AppendLog(L"INI reset to defaults.");
}

void CMainFrame::OnBnClickedLoadPattern() {
    CFileDialog dlg(TRUE, L"rgbpat", nullptr,
                    OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
                    L"RGB Pattern (*.rgbpat)|*.rgbpat|All Files (*.*)|*.*||", this);
    if (dlg.DoModal() == IDOK) {
        std::wstring error;
        const std::wstring path = dlg.GetPathName().GetString();
        if (!rgb_tab_.LoadPatternFromFile(path, &error)) {
            ShowErrorBox(L"Load Pattern Error", error);
            return;
        }
        state_.ui.last_pattern_path = path;
    }
}

void CMainFrame::OnBnClickedSavePattern() {
    CFileDialog dlg(FALSE, L"rgbpat", L"rgb_pattern.rgbpat",
                    OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
                    L"RGB Pattern (*.rgbpat)|*.rgbpat|All Files (*.*)|*.*||", this);
    if (dlg.DoModal() == IDOK) {
        std::wstring error;
        const std::wstring path = dlg.GetPathName().GetString();
        if (!rgb_tab_.SavePatternToFile(path, &error)) {
            ShowErrorBox(L"Save Pattern Error", error);
            return;
        }
        state_.ui.last_pattern_path = path;
        AppendLog(L"Pattern saved: " + path);
    }
}

void CMainFrame::OnBnClickedCopyCode() {
    if (!rgb_tab_.CopyCodeToClipboard()) {
        ShowErrorBox(L"Copy Code", L"Clipboard copy failed.");
        return;
    }
    AppendLog(L"Generated code copied to clipboard.");
}

void CMainFrame::OnBnClickedSaveCode() {
    CFolderPickerDialog dlg(nullptr, 0, this);
    if (dlg.DoModal() == IDOK) {
        std::wstring error;
        const std::wstring path = dlg.GetPathName().GetString();
        if (!rgb_tab_.SaveGeneratedCodeToFolder(path, &error)) {
            ShowErrorBox(L"Save Code Error", error);
            return;
        }
        state_.ui.last_code_path = path;
        AppendLog(L"Generated MCU source files saved: " + path);
    }
}

void CMainFrame::OnBnClickedSaveLog() {
    if (save_log_check_.GetCheck() != BST_CHECKED) {
        return;
    }

    CFileDialog dlg(FALSE, L"log", L"rgb_led_tool.log",
                    OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
                    L"Log Files (*.log)|*.log|Text Files (*.txt)|*.txt|All Files (*.*)|*.*||", this);
    if (dlg.DoModal() == IDOK) {
        SaveLogToFile(dlg.GetPathName().GetString());
    }
}

void CMainFrame::OnBnClickedSaveLogCheck() {
    const bool checked = (save_log_check_.GetCheck() == BST_CHECKED);
    mfc_tool::ui::SafeEnableWindow(save_log_btn_, checked);
}

void CMainFrame::OnBnClickedClearLog() {
    logger_.Clear();
    log_edit_.SetWindowTextW(L"");
}

void CMainFrame::OnClose() {
    control_tab_.Disconnect();
    SaveIni();
    CFrameWnd::OnClose();
}
