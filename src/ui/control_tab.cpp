#include "control_tab.h"

#include <afxdlgs.h>
#include <algorithm>

#include "../resource.h"
#include "layout_utils.h"

BEGIN_MESSAGE_MAP(CControlTab, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_BN_CLICKED(IDC_CONTROL_CONNECT, &CControlTab::OnConnect)
    ON_BN_CLICKED(IDC_CONTROL_DISCONNECT, &CControlTab::OnDisconnect)
    ON_BN_CLICKED(IDC_CONTROL_SEND_CONFIG, &CControlTab::OnSendConfig)
    ON_BN_CLICKED(IDC_CONTROL_APPLY, &CControlTab::OnApplyEffect)
    ON_BN_CLICKED(IDC_CONTROL_STOP, &CControlTab::OnStop)
    ON_CBN_SELCHANGE(IDC_CONTROL_EFFECT, &CControlTab::OnEffectChanged)
    ON_BN_CLICKED(IDC_CONTROL_CHOOSE_COLOR, &CControlTab::OnChooseColor)
    ON_BN_CLICKED(IDC_CONTROL_RESET_VALUES, &CControlTab::OnResetValues)
    ON_EN_KILLFOCUS(IDC_CONTROL_RED, &CControlTab::OnNumericEditCommit)
    ON_EN_KILLFOCUS(IDC_CONTROL_GREEN, &CControlTab::OnNumericEditCommit)
    ON_EN_KILLFOCUS(IDC_CONTROL_BLUE, &CControlTab::OnNumericEditCommit)
    ON_EN_KILLFOCUS(IDC_CONTROL_SPEED, &CControlTab::OnNumericEditCommit)
    ON_WM_HSCROLL()
END_MESSAGE_MAP()

BOOL CControlTab::Create(CWnd* parent, const RECT& rect, UINT id) {
    CString cls = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, ::LoadCursor(nullptr, IDC_ARROW),
                                      reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), nullptr);
    return CWnd::CreateEx(0, cls, L"USB RGB LED Control", WS_CHILD | WS_CLIPCHILDREN, rect, parent, id);
}

void CControlTab::Bind(std::function<void(const std::wstring&)> logger) { log_ = std::move(logger); }

int CControlTab::OnCreate(LPCREATESTRUCT create_struct) {
    if (CWnd::OnCreate(create_struct) == -1) return -1;
    font_.CreatePointFont(95, L"Segoe UI");
    auto group = [this](CButton& item, const wchar_t* text, UINT id) { item.Create(text, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, CRect(), this, id); };
    auto label = [this](CStatic& item, const wchar_t* text, UINT id) { item.Create(text, WS_CHILD | WS_VISIBLE, CRect(), this, id); };
    auto edit = [this](CEdit& item, const wchar_t* text, UINT id) { item.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, CRect(), this, id); item.SetWindowTextW(text); };
    auto button = [this](CButton& item, const wchar_t* text, UINT id) { item.Create(text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, CRect(), this, id); };
    auto slider = [this](CSliderCtrl& item, UINT id, int minimum, int maximum, int value) {
        item.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_HORZ | TBS_AUTOTICKS,
                    CRect(), this, id);
        item.SetRange(minimum, maximum);
        item.SetTicFreq(32);
        item.SetPageSize(16);
        item.SetPos(value);
    };

    group(connection_group_, L"USB HID Device", IDC_CONTROL_CONNECTION_GROUP);
    label(status_title_, L"Status:", IDC_CONTROL_STATUS_TITLE);
    label(status_value_, L"Disconnected", IDC_CONTROL_STATUS_VALUE);
    button(connect_btn_, L"Connect", IDC_CONTROL_CONNECT);
    button(disconnect_btn_, L"Disconnect", IDC_CONTROL_DISCONNECT);
    group(config_group_, L"Strip Configuration", IDC_CONTROL_CONFIG_GROUP);
    label(led_count_title_, L"LED count (1-10)", IDC_CONTROL_LED_COUNT_TITLE); edit(led_count_edit_, L"8", IDC_CONTROL_LED_COUNT);
    label(order_title_, L"Wire order", IDC_CONTROL_ORDER_TITLE);
    order_combo_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                        CRect(), this, IDC_CONTROL_ORDER);
    order_combo_.AddString(L"GRB (WS2812)"); order_combo_.AddString(L"RGB (LTST-E563CEGBW)"); order_combo_.SetCurSel(0);
    label(brightness_title_, L"Brightness (0-255)", IDC_CONTROL_BRIGHTNESS_TITLE); edit(brightness_edit_, L"128", IDC_CONTROL_BRIGHTNESS);
    button(send_config_btn_, L"Apply configuration", IDC_CONTROL_SEND_CONFIG);
    group(effect_group_, L"Live Effect", IDC_CONTROL_EFFECT_GROUP);
    label(effect_title_, L"Effect", IDC_CONTROL_EFFECT_TITLE);
    effect_combo_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                         CRect(), this, IDC_CONTROL_EFFECT);
    for (const wchar_t* name : {L"Static", L"Breathing", L"Blink", L"Gradient", L"Flow (rainbow)"}) effect_combo_.AddString(name);
    effect_combo_.SetCurSel(0);
    label(effect_help_, L"Static uses the selected RGB color.", IDC_CONTROL_EFFECT_HELP);
    button(choose_color_btn_, L"Choose color...  #FF5000", IDC_CONTROL_CHOOSE_COLOR);
    button(reset_values_btn_, L"Reset RGB / Speed", IDC_CONTROL_RESET_VALUES);
    label(red_title_, L"Red (0-255)", IDC_CONTROL_RED_TITLE); edit(red_edit_, L"255", IDC_CONTROL_RED);
    slider(red_slider_, IDC_CONTROL_RED_SLIDER, 0, 255, 255);
    label(green_title_, L"Green (0-255)", IDC_CONTROL_GREEN_TITLE); edit(green_edit_, L"80", IDC_CONTROL_GREEN);
    slider(green_slider_, IDC_CONTROL_GREEN_SLIDER, 0, 255, 80);
    label(blue_title_, L"Blue (0-255)", IDC_CONTROL_BLUE_TITLE); edit(blue_edit_, L"0", IDC_CONTROL_BLUE);
    slider(blue_slider_, IDC_CONTROL_BLUE_SLIDER, 0, 255, 0);
    label(speed_title_, L"Speed (1-255)", IDC_CONTROL_SPEED_TITLE); edit(speed_edit_, L"128", IDC_CONTROL_SPEED);
    slider(speed_slider_, IDC_CONTROL_SPEED_SLIDER, 1, 255, 128);
    label(direction_title_, L"Direction", IDC_CONTROL_DIRECTION_TITLE);
    direction_combo_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                            CRect(), this, IDC_CONTROL_DIRECTION);
    direction_combo_.AddString(L"Forward"); direction_combo_.AddString(L"Reverse"); direction_combo_.SetCurSel(0);
    button(apply_btn_, L"Apply effect", IDC_CONTROL_APPLY); button(stop_btn_, L"Stop / LEDs off", IDC_CONTROL_STOP);

    CWnd* controls[] = {&connection_group_, &status_title_, &status_value_, &connect_btn_, &disconnect_btn_, &config_group_,
        &led_count_title_, &led_count_edit_, &order_title_, &order_combo_, &brightness_title_, &brightness_edit_, &send_config_btn_,
        &effect_group_, &effect_title_, &effect_combo_, &effect_help_, &choose_color_btn_, &reset_values_btn_, &red_title_, &red_edit_, &red_slider_,
        &green_title_, &green_edit_, &green_slider_, &blue_title_, &blue_edit_, &blue_slider_,
        &speed_title_, &speed_edit_, &speed_slider_, &direction_title_, &direction_combo_, &apply_btn_, &stop_btn_};
    for (CWnd* control : controls) mfc_tool::ui::SafeSetFont(*control, &font_);
    red_edit_.LimitText(3); green_edit_.LimitText(3); blue_edit_.LimitText(3); speed_edit_.LimitText(3);
    UpdateEffectUi();
    UpdateConnectionUi();
    return 0;
}

void CControlTab::OnSize(UINT type, int cx, int cy) { CWnd::OnSize(type, cx, cy); LayoutControls(cx, cy); }

void CControlTab::LayoutControls(int cx, int cy) {
    if (cx < 200 || cy < 120) return;
    const int m = 14, g = 10, h = 26, label_w = 145;
    int y = m + 20;
    mfc_tool::ui::SafeMoveWindow(connection_group_, m, m, cx - 2 * m, 70);
    mfc_tool::ui::SafeMoveWindow(status_title_, m + 14, y + 5, 55, 20);
    mfc_tool::ui::SafeMoveWindow(status_value_, m + 72, y + 5, (std::max)(120, cx - 390), 20);
    mfc_tool::ui::SafeMoveWindow(connect_btn_, cx - m - 210, y, 95, h);
    mfc_tool::ui::SafeMoveWindow(disconnect_btn_, cx - m - 105, y, 95, h);
    y = m + 84;
    mfc_tool::ui::SafeMoveWindow(config_group_, m, y, cx - 2 * m, 120);
    int row = y + 28, x = m + 18;
    mfc_tool::ui::SafeMoveWindow(led_count_title_, x, row + 5, label_w, 20); x += label_w;
    mfc_tool::ui::SafeMoveWindow(led_count_edit_, x, row, 64, h); x += 82;
    mfc_tool::ui::SafeMoveWindow(order_title_, x, row + 5, 75, 20); x += 78;
    mfc_tool::ui::SafeMoveWindow(order_combo_, x, row, 210, 200);
    row += h + g; x = m + 18;
    mfc_tool::ui::SafeMoveWindow(brightness_title_, x, row + 5, label_w, 20); x += label_w;
    mfc_tool::ui::SafeMoveWindow(brightness_edit_, x, row, 64, h);
    mfc_tool::ui::SafeMoveWindow(send_config_btn_, x + 82, row, 150, h);
    y += 134;
    mfc_tool::ui::SafeMoveWindow(effect_group_, m, y, cx - 2 * m, (std::max)(150, cy - y - m));
    row = y + 30; x = m + 18;
    auto pair = [&](CStatic& title, CWnd& control, int control_width, int control_height) {
        mfc_tool::ui::SafeMoveWindow(title, x, row + 5, 85, 20);
        x += 88;
        mfc_tool::ui::SafeMoveWindow(control, x, row, control_width, control_height);
        x += control_width + 22;
    };
    pair(effect_title_, effect_combo_, 170, 180);
    pair(direction_title_, direction_combo_, 110, 120);
    row += h + 4; x = m + 18;
    mfc_tool::ui::SafeMoveWindow(effect_help_, x, row, (std::max)(320, cx - 2 * m - 36), 20);
    row += h + 2;
    mfc_tool::ui::SafeMoveWindow(choose_color_btn_, x, row, 205, h);
    mfc_tool::ui::SafeMoveWindow(reset_values_btn_, x + 215, row, 155, h);
    auto numeric_row = [&](CStatic& title, CEdit& edit_control, CSliderCtrl& slider_control) {
        row += h + 4;
        mfc_tool::ui::SafeMoveWindow(title, x, row + 5, 105, 20);
        mfc_tool::ui::SafeMoveWindow(edit_control, x + 108, row, 62, h);
        mfc_tool::ui::SafeMoveWindow(slider_control, x + 180, row, (std::max)(180, cx - x - 220), h);
    };
    numeric_row(speed_title_, speed_edit_, speed_slider_);
    numeric_row(red_title_, red_edit_, red_slider_);
    numeric_row(green_title_, green_edit_, green_slider_);
    numeric_row(blue_title_, blue_edit_, blue_slider_);
    row += h + 14; x = m + 18;
    mfc_tool::ui::SafeMoveWindow(apply_btn_, x, row, 130, 34); mfc_tool::ui::SafeMoveWindow(stop_btn_, x + 145, row, 145, 34);
}

int CControlTab::ReadInt(const CEdit& edit, int fallback, int minimum, int maximum) const {
    CString value; const_cast<CEdit&>(edit).GetWindowTextW(value);
    int parsed = _wtoi(value); if (value.IsEmpty()) parsed = fallback;
    return (std::max)(minimum, (std::min)(maximum, parsed));
}

int CControlTab::NormalizeEdit(CEdit& edit, CSliderCtrl& slider, int fallback, int minimum, int maximum) {
    const int value = ReadInt(edit, fallback, minimum, maximum);
    updating_ui_ = true;
    SetEditValue(edit, value);
    slider.SetPos(value);
    updating_ui_ = false;
    return value;
}

void CControlTab::SetEditValue(CEdit& edit, int value) {
    CString text;
    text.Format(L"%d", value);
    edit.SetWindowTextW(text);
}

void CControlTab::SetColorValues(int red, int green, int blue) {
    updating_ui_ = true;
    SetEditValue(red_edit_, red);
    SetEditValue(green_edit_, green);
    SetEditValue(blue_edit_, blue);
    red_slider_.SetPos(red);
    green_slider_.SetPos(green);
    blue_slider_.SetPos(blue);
    updating_ui_ = false;
    UpdateColorButtonText();
}

void CControlTab::UpdateColorButtonText() {
    CString text;
    const int red = ReadInt(red_edit_, 255, 0, 255);
    const int green = ReadInt(green_edit_, 80, 0, 255);
    const int blue = ReadInt(blue_edit_, 0, 0, 255);
    text.Format(L"Choose color...  #%02X%02X%02X", red, green, blue);
    if (::IsWindow(choose_color_btn_.GetSafeHwnd())) choose_color_btn_.SetWindowTextW(text);
}

void CControlTab::UpdateEffectUi() {
    const int effect = (std::max)(0, effect_combo_.GetCurSel());
    const BOOL rgb_enabled = (effect != 4) ? TRUE : FALSE;
    const BOOL speed_enabled = (effect != 0) ? TRUE : FALSE;
    const BOOL direction_enabled = (effect >= 3) ? TRUE : FALSE;
    const wchar_t* help = L"Static uses the selected RGB color.";
    if (effect == 1) help = L"Breathing fades the selected RGB color in and out.";
    else if (effect == 2) help = L"Blink switches the selected RGB color on and off.";
    else if (effect == 3) help = L"Gradient moves a brightness gradient of the selected RGB color.";
    else if (effect == 4) help = L"Flow uses an internal rainbow; RGB values are not used.";
    for (CWnd* control : {static_cast<CWnd*>(&red_title_), static_cast<CWnd*>(&red_edit_), static_cast<CWnd*>(&red_slider_),
                          static_cast<CWnd*>(&green_title_), static_cast<CWnd*>(&green_edit_), static_cast<CWnd*>(&green_slider_),
                          static_cast<CWnd*>(&blue_title_), static_cast<CWnd*>(&blue_edit_), static_cast<CWnd*>(&blue_slider_),
                          static_cast<CWnd*>(&choose_color_btn_)})
        mfc_tool::ui::SafeEnableWindow(*control, rgb_enabled);
    for (CWnd* control : {static_cast<CWnd*>(&speed_title_), static_cast<CWnd*>(&speed_edit_), static_cast<CWnd*>(&speed_slider_)})
        mfc_tool::ui::SafeEnableWindow(*control, speed_enabled);
    mfc_tool::ui::SafeEnableWindow(direction_title_, direction_enabled);
    mfc_tool::ui::SafeEnableWindow(direction_combo_, direction_enabled);
    if (::IsWindow(effect_help_.GetSafeHwnd())) effect_help_.SetWindowTextW(help);
}

void CControlTab::OnEffectChanged() { UpdateEffectUi(); }

void CControlTab::OnChooseColor() {
    const COLORREF initial = RGB(ReadInt(red_edit_, 255, 0, 255),
                                 ReadInt(green_edit_, 80, 0, 255),
                                 ReadInt(blue_edit_, 0, 0, 255));
    CColorDialog dialog(initial, CC_FULLOPEN | CC_RGBINIT, this);
    if (dialog.DoModal() == IDOK) {
        const COLORREF selected = dialog.GetColor();
        SetColorValues(GetRValue(selected), GetGValue(selected), GetBValue(selected));
    }
}

void CControlTab::OnResetValues() {
    SetColorValues(255, 80, 0);
    updating_ui_ = true;
    SetEditValue(speed_edit_, 128);
    speed_slider_.SetPos(128);
    updating_ui_ = false;
}

void CControlTab::OnNumericEditCommit() {
    if (updating_ui_) return;
    NormalizeEdit(red_edit_, red_slider_, 255, 0, 255);
    NormalizeEdit(green_edit_, green_slider_, 80, 0, 255);
    NormalizeEdit(blue_edit_, blue_slider_, 0, 0, 255);
    NormalizeEdit(speed_edit_, speed_slider_, 128, 1, 255);
    UpdateColorButtonText();
}

void CControlTab::OnHScroll(UINT code, UINT position, CScrollBar* scroll_bar) {
    CSliderCtrl* slider = reinterpret_cast<CSliderCtrl*>(scroll_bar);
    if (slider != nullptr && ::IsWindow(slider->GetSafeHwnd())) {
        updating_ui_ = true;
        if (slider == &red_slider_) SetEditValue(red_edit_, red_slider_.GetPos());
        else if (slider == &green_slider_) SetEditValue(green_edit_, green_slider_.GetPos());
        else if (slider == &blue_slider_) SetEditValue(blue_edit_, blue_slider_.GetPos());
        else if (slider == &speed_slider_) SetEditValue(speed_edit_, speed_slider_.GetPos());
        updating_ui_ = false;
        if (slider == &red_slider_ || slider == &green_slider_ || slider == &blue_slider_) UpdateColorButtonText();
    }
    CWnd::OnHScroll(code, position, scroll_bar);
}

void CControlTab::UpdateConnectionUi() {
    const bool connected = device_.IsOpen();
    if (::IsWindow(status_value_.GetSafeHwnd())) status_value_.SetWindowTextW(connected ? L"Connected - M032 RGB Controller" : L"Disconnected");
    mfc_tool::ui::SafeEnableWindow(connect_btn_, !connected);
    mfc_tool::ui::SafeEnableWindow(disconnect_btn_, connected);
    mfc_tool::ui::SafeEnableWindow(send_config_btn_, connected);
    mfc_tool::ui::SafeEnableWindow(apply_btn_, connected);
    mfc_tool::ui::SafeEnableWindow(stop_btn_, connected);
}

void CControlTab::OnConnect() {
    std::wstring error;
    if (!device_.Open(&error)) { if (log_) log_(error); MessageBoxW(error.c_str(), L"USB HID", MB_OK | MB_ICONERROR); return; }
    UpdateConnectionUi(); if (log_) log_(L"USB HID connected: " + device_.Path());
    Send(rgb_tool::hid::Command::GetInfo, nullptr, 0, L"Get firmware info");
}
void CControlTab::OnDisconnect() { Disconnect(); }
void CControlTab::Disconnect() { if (device_.IsOpen() && log_) log_(L"USB HID disconnected."); device_.Close(); UpdateConnectionUi(); }

bool CControlTab::Send(rgb_tool::hid::Command command, const std::uint8_t* payload, std::uint8_t length, const wchar_t* label) {
    std::wstring error;
    if (!device_.Send(command, payload, length, &error)) { UpdateConnectionUi(); if (log_) log_(std::wstring(label) + L" failed: " + error); return false; }
    if (log_) log_(std::wstring(label) + L" sent."); return true;
}

void CControlTab::OnSendConfig() {
    std::uint8_t payload[3] = {static_cast<std::uint8_t>(ReadInt(led_count_edit_, 8, 1, 10)),
        static_cast<std::uint8_t>((std::max)(0, order_combo_.GetCurSel())),
        static_cast<std::uint8_t>(ReadInt(brightness_edit_, 128, 0, 255))};
    Send(rgb_tool::hid::Command::SetConfig, payload, sizeof(payload), L"Configuration");
}

void CControlTab::OnApplyEffect() {
    const int red = NormalizeEdit(red_edit_, red_slider_, 255, 0, 255);
    const int green = NormalizeEdit(green_edit_, green_slider_, 80, 0, 255);
    const int blue = NormalizeEdit(blue_edit_, blue_slider_, 0, 0, 255);
    const int speed = NormalizeEdit(speed_edit_, speed_slider_, 128, 1, 255);
    std::uint8_t payload[7] = {static_cast<std::uint8_t>((std::max)(0, effect_combo_.GetCurSel())),
        static_cast<std::uint8_t>(red), static_cast<std::uint8_t>(green),
        static_cast<std::uint8_t>(blue), static_cast<std::uint8_t>(speed),
        static_cast<std::uint8_t>(ReadInt(brightness_edit_, 128, 0, 255)), static_cast<std::uint8_t>((std::max)(0, direction_combo_.GetCurSel()))};
    Send(rgb_tool::hid::Command::SetEffect, payload, sizeof(payload), L"Effect");
}

void CControlTab::OnStop() { Send(rgb_tool::hid::Command::Stop, nullptr, 0, L"Stop"); }

void CControlTab::LoadState(const rgb_tool::core::AppState& state) {
    const auto select_text = [](CComboBox& combo, const std::wstring& text, int fallback) {
        const int found = combo.FindStringExact(-1, text.c_str());
        combo.SetCurSel(found == CB_ERR ? fallback : found);
    };
    const int red = (std::max)(0, (std::min)(255, _wtoi(state.control.red.c_str())));
    const int green = (std::max)(0, (std::min)(255, _wtoi(state.control.green.c_str())));
    const int blue = (std::max)(0, (std::min)(255, _wtoi(state.control.blue.c_str())));
    const int speed = (std::max)(1, (std::min)(255, _wtoi(state.control.speed.c_str())));
    const int count = (std::max)(1, (std::min)(10, _wtoi(state.control.led_count.c_str())));
    const int brightness = (std::max)(0, (std::min)(255, _wtoi(state.control.brightness.c_str())));

    SetEditValue(led_count_edit_, count);
    SetEditValue(brightness_edit_, brightness);
    select_text(order_combo_, state.control.order == L"RGB" ? L"RGB (LTST-E563CEGBW)" : L"GRB (WS2812)", 0);
    select_text(effect_combo_, state.control.effect, 0);
    select_text(direction_combo_, state.control.direction, 0);
    SetColorValues(red, green, blue);
    SetEditValue(speed_edit_, speed);
    speed_slider_.SetPos(speed);
    UpdateEffectUi();
}

void CControlTab::SaveState(rgb_tool::core::AppState* state) const {
    CString text;
    if (state == nullptr) return;
    state->control.led_count = std::to_wstring(ReadInt(led_count_edit_, 8, 1, 10));
    state->control.order = order_combo_.GetCurSel() == 1 ? L"RGB" : L"GRB";
    state->control.brightness = std::to_wstring(ReadInt(brightness_edit_, 128, 0, 255));
    effect_combo_.GetWindowTextW(text);
    state->control.effect = text.GetString();
    state->control.red = std::to_wstring(ReadInt(red_edit_, 255, 0, 255));
    state->control.green = std::to_wstring(ReadInt(green_edit_, 80, 0, 255));
    state->control.blue = std::to_wstring(ReadInt(blue_edit_, 0, 0, 255));
    state->control.speed = std::to_wstring(ReadInt(speed_edit_, 128, 1, 255));
    direction_combo_.GetWindowTextW(text);
    state->control.direction = text.GetString();
}
