#include "rgb_led_tab.h"

#include <afxdlgs.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "../core/text_utils.h"
#include "layout_utils.h"

namespace {

enum : UINT {
    IDC_RGB_GROUP = 13000,
    IDC_RGB_CODE_GROUP,
    IDC_RGB_LED_COUNT_TITLE,
    IDC_RGB_LED_COUNT_EDIT,
    IDC_RGB_SHAPE_TITLE,
    IDC_RGB_SHAPE_COMBO,
    IDC_RGB_APPLY_LAYOUT,
    IDC_RGB_CURRENT_LAYOUT,
    IDC_RGB_PRESET_TITLE,
    IDC_RGB_PRESET_COMBO,
    IDC_RGB_LOAD_PRESET,
    IDC_RGB_CURRENT_PRESET,
    IDC_RGB_SYMBOL_TITLE,
    IDC_RGB_SYMBOL_EDIT,
    IDC_RGB_SYMBOL_HELP,
    IDC_RGB_BACKEND_TITLE,
    IDC_RGB_BACKEND_COMBO,
    IDC_RGB_BACKEND_HELP,
    IDC_RGB_START_TITLE,
    IDC_RGB_START_EDIT,
    IDC_RGB_DIRECTION_TITLE,
    IDC_RGB_DIRECTION_COMBO,
    IDC_RGB_PATH_MODE_TITLE,
    IDC_RGB_PATH_MODE_COMBO,
    IDC_RGB_PATH_HELP,
    IDC_RGB_FRAME_TITLE,
    IDC_RGB_FRAME_COMBO,
    IDC_RGB_ADD_FRAME,
    IDC_RGB_DUP_FRAME,
    IDC_RGB_DEL_FRAME,
    IDC_RGB_DURATION_TITLE,
    IDC_RGB_DURATION_EDIT,
    IDC_RGB_PLAY,
    IDC_RGB_STOP,
    IDC_RGB_RESET_PATTERN,
    IDC_RGB_PREVIOUS_FRAME,
    IDC_RGB_NEXT_FRAME,
    IDC_RGB_PLAYBACK_STATUS,
    IDC_RGB_PLAYBACK_SLIDER,
    IDC_RGB_PLAYBACK_SPEED_TITLE,
    IDC_RGB_PLAYBACK_SPEED,
    IDC_RGB_LED_TITLE,
    IDC_RGB_LED_COMBO,
    IDC_RGB_RED_TITLE,
    IDC_RGB_RED_EDIT,
    IDC_RGB_GREEN_TITLE,
    IDC_RGB_GREEN_EDIT,
    IDC_RGB_BLUE_TITLE,
    IDC_RGB_BLUE_EDIT,
    IDC_RGB_CHOOSE_COLOR,
    IDC_RGB_COLOR2_TITLE, IDC_RGB_COLOR2_RED, IDC_RGB_COLOR2_GREEN, IDC_RGB_COLOR2_BLUE, IDC_RGB_CHOOSE_COLOR2,
    IDC_RGB_TAIL_TITLE, IDC_RGB_TAIL_EDIT, IDC_RGB_FADE_TITLE, IDC_RGB_FADE_EDIT,
    IDC_RGB_SPACING_TITLE, IDC_RGB_SPACING_EDIT, IDC_RGB_REPEAT_TITLE, IDC_RGB_REPEAT_EDIT,
    IDC_RGB_APPLY_LED,
    IDC_RGB_FILL_ALL,
    IDC_RGB_CLEAR_FRAME,
    IDC_RGB_GRADIENT,
    IDC_RGB_SHIFT_LEFT,
    IDC_RGB_SHIFT_RIGHT,
    IDC_RGB_PREVIEW_TITLE,
    IDC_RGB_PREVIEW,
    IDC_RGB_CODE_TITLE,
    IDC_RGB_CODE_EDIT
    ,IDC_RGB_COPY_CODE
    ,IDC_RGB_SAVE_CODE
};

constexpr UINT_PTR kPreviewTimerId = 1u;
constexpr double kPi = 3.14159265358979323846;

COLORREF ToColorRef(const rgb_tool::core::RgbColor& c) {
    return RGB(c.r, c.g, c.b);
}

std::vector<CPoint> BuildLinePoints(int count, const CRect& rc) {
    std::vector<CPoint> points;
    int i = 0;
    const int usable_w = (std::max)(1, rc.Width() - 56);
    const int y = rc.CenterPoint().y;

    points.reserve(static_cast<size_t>(count));
    for (i = 0; i < count; ++i) {
        const int x = rc.left + 28 + (count <= 1 ? usable_w / 2 : (usable_w * i) / (count - 1));
        points.emplace_back(x, y);
    }
    return points;
}

std::vector<CPoint> BuildRingPoints(int count, const CRect& rc) {
    std::vector<CPoint> points;
    int i = 0;
    const CPoint center = rc.CenterPoint();
    const int radius = (std::max)(20, (std::min)(rc.Width(), rc.Height()) / 2 - 34);

    points.reserve(static_cast<size_t>(count));
    for (i = 0; i < count; ++i) {
        const double angle = -kPi / 2.0 + (2.0 * kPi * static_cast<double>(i)) / (std::max)(1, count);
        const int x = center.x + static_cast<int>(std::cos(angle) * radius);
        const int y = center.y + static_cast<int>(std::sin(angle) * radius);
        points.emplace_back(x, y);
    }
    return points;
}

std::vector<CPoint> BuildSquarePoints(int count, const CRect& rc) {
    std::vector<CPoint> points;
    int i = 0;
    CRect box;
    const CPoint center = rc.CenterPoint();
    const int max_side = (std::max)(80, (std::min)(rc.Width() - 72, rc.Height() - 60));
    const int side_len = (std::max)(80, (std::min)(260, (max_side * 3) / 4));
    const int half_side = side_len / 2;

    box.SetRect(center.x - half_side, center.y - half_side, center.x + half_side, center.y + half_side);
    points.reserve(static_cast<size_t>(count));
    for (i = 0; i < count; ++i) {
        const double t = static_cast<double>(i) / (std::max)(1, count);
        const double side = t * 4.0;
        int x = box.left;
        int y = box.top;
        if (side < 1.0) {
            x = box.left + static_cast<int>(side * box.Width());
            y = box.top;
        } else if (side < 2.0) {
            x = box.right;
            y = box.top + static_cast<int>((side - 1.0) * box.Height());
        } else if (side < 3.0) {
            x = box.right - static_cast<int>((side - 2.0) * box.Width());
            y = box.bottom;
        } else {
            x = box.left;
            y = box.bottom - static_cast<int>((side - 3.0) * box.Height());
        }
        points.emplace_back(x, y);
    }
    return points;
}

std::vector<CPoint> BuildUShapePoints(int count, const CRect& rc) {
    std::vector<CPoint> points;
    int i = 0;
    int left_count = 0;
    int top_count = 0;
    int right_count = 0;
    CRect box;
    const CPoint center = rc.CenterPoint();
    const int available_h = (std::max)(90, rc.Height() - 62);
    const int u_h = (std::max)(90, (std::min)(220, (available_h * 3) / 4));
    const int u_w = (std::max)(120, (std::min)(340, (u_h * 3) / 2));
    const int half_w = u_w / 2;
    const int half_h = u_h / 2;

    box.SetRect(center.x - half_w, center.y - half_h, center.x + half_w, center.y + half_h);
    points.reserve(static_cast<size_t>(count));
    if (count <= 0) {
        return points;
    }
    if (count == 1) {
        points.emplace_back(center.x, box.bottom);
        return points;
    }
    if (count == 2) {
        points.emplace_back(box.left, box.bottom);
        points.emplace_back(box.right, box.bottom);
        return points;
    }

    left_count = (std::max)(1, (count + 1) / 3);
    if ((left_count * 2) >= count) {
        left_count = (std::max)(1, (count - 1) / 2);
    }
    right_count = left_count;
    top_count = count - left_count - right_count;

    for (i = 0; i < left_count; ++i) {
        const int y = (left_count <= 1) ? box.bottom : box.bottom - (box.Height() * i) / (left_count - 1);
        points.emplace_back(box.left, y);
    }
    for (i = 0; i < top_count; ++i) {
        const int x = box.left + (box.Width() * (i + 1)) / (top_count + 1);
        points.emplace_back(x, box.top);
    }
    for (i = 0; i < right_count; ++i) {
        const int y = (right_count <= 1) ? box.bottom : box.top + (box.Height() * i) / (right_count - 1);
        points.emplace_back(box.right, y);
    }
    return points;
}

std::vector<CPoint> BuildPreviewPoints(rgb_tool::core::LedShape shape, int count, const CRect& rc) {
    switch (shape) {
    case rgb_tool::core::LedShape::Ring: return BuildRingPoints(count, rc);
    case rgb_tool::core::LedShape::Square: return BuildSquarePoints(count, rc);
    case rgb_tool::core::LedShape::UShape: return BuildUShapePoints(count, rc);
    case rgb_tool::core::LedShape::Line:
    default: return BuildLinePoints(count, rc);
    }
}

int ParseIntOr(const std::wstring& text, int fallback) {
    try {
        return rgb_tool::core::ParseInt(text);
    } catch (...) {
        return fallback;
    }
}

} // namespace

BEGIN_MESSAGE_MAP(CRgbPreviewWnd, CWnd)
    ON_WM_PAINT()
END_MESSAGE_MAP()

BOOL CRgbPreviewWnd::Create(CWnd* parent, const RECT& rect, UINT id) {
    CString cls = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, ::LoadCursor(nullptr, IDC_ARROW),
                                      reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), nullptr);
    return CWnd::CreateEx(WS_EX_CLIENTEDGE, cls, L"", WS_CHILD | WS_VISIBLE, rect, parent, id);
}

void CRgbPreviewWnd::SetPattern(const rgb_tool::core::RgbPattern* pattern, int frame_index) {
    pattern_ = pattern;
    frame_index_ = frame_index;
    Invalidate(FALSE);
}

void CRgbPreviewWnd::OnPaint() {
    CPaintDC dc(this);
    CRect rc;
    CBrush bg(RGB(24, 27, 31));
    CPen dim_pen(PS_SOLID, 1, RGB(70, 76, 84));
    CPen* old_pen = nullptr;
    CFont label_font;
    CFont* old_font = nullptr;

    GetClientRect(&rc);
    dc.FillRect(&rc, &bg);
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(RGB(218, 223, 230));
    label_font.CreatePointFont(72, L"Segoe UI");
    old_font = dc.SelectObject(&label_font);
    old_pen = dc.SelectObject(&dim_pen);

    if (pattern_ == nullptr || pattern_->frames.empty()) {
        dc.DrawTextW(L"No pattern", -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (old_pen != nullptr) {
            dc.SelectObject(old_pen);
        }
        if (old_font != nullptr) {
            dc.SelectObject(old_font);
        }
        return;
    }

    const int safe_frame = rgb_tool::core::ClampInt(frame_index_, 0, static_cast<int>(pattern_->frames.size()) - 1);
    const rgb_tool::core::PatternFrame& frame = pattern_->frames[static_cast<size_t>(safe_frame)];
    const int count = (std::min)(pattern_->led_count, static_cast<int>(frame.leds.size()));
    const std::vector<CPoint> points = BuildPreviewPoints(pattern_->shape, count, rc);
    const int radius = (count > 40) ? 8 : ((count > 28) ? 10 : 13);
    int i = 0;

    for (i = 1; i < static_cast<int>(points.size()); ++i) {
        dc.MoveTo(points[static_cast<size_t>(i - 1)]);
        dc.LineTo(points[static_cast<size_t>(i)]);
    }

    for (i = 0; i < count; ++i) {
        const rgb_tool::core::RgbColor& color = frame.leds[static_cast<size_t>(i)];
        CBrush brush(ToColorRef(color));
        CPen border(PS_SOLID, 1, RGB(210, 214, 220));
        CBrush* old_brush = dc.SelectObject(&brush);
        CPen* old_led_pen = dc.SelectObject(&border);
        const CPoint p = points[static_cast<size_t>(i)];
        CRect dot(p.x - radius, p.y - radius, p.x + radius, p.y + radius);
        dc.Ellipse(&dot);
        if (old_led_pen != nullptr) {
            dc.SelectObject(old_led_pen);
        }
        if (old_brush != nullptr) {
            dc.SelectObject(old_brush);
        }
        if (count <= 32) {
            CString s;
            s.Format(L"%d", i);
            CRect text_rc(p.x - 18, p.y + radius + 2, p.x + 18, p.y + radius + 18);
            dc.DrawTextW(s, &text_rc, DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        }
    }

    if (old_pen != nullptr) {
        dc.SelectObject(old_pen);
    }
    if (old_font != nullptr) {
        dc.SelectObject(old_font);
    }
}

BEGIN_MESSAGE_MAP(CRgbLedTab, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_TIMER()
    ON_WM_HSCROLL()
    ON_BN_CLICKED(IDC_RGB_APPLY_LAYOUT, &CRgbLedTab::OnApplyLayout)
    ON_BN_CLICKED(IDC_RGB_LOAD_PRESET, &CRgbLedTab::OnLoadPreset)
    ON_BN_CLICKED(IDC_RGB_CHOOSE_COLOR, &CRgbLedTab::OnChoosePatternColor)
    ON_BN_CLICKED(IDC_RGB_CHOOSE_COLOR2, &CRgbLedTab::OnChooseSecondColor)
    ON_EN_CHANGE(IDC_RGB_START_EDIT, &CRgbLedTab::OnPathOptionChanged)
    ON_CBN_SELCHANGE(IDC_RGB_DIRECTION_COMBO, &CRgbLedTab::OnPathOptionChanged)
    ON_CBN_SELCHANGE(IDC_RGB_PATH_MODE_COMBO, &CRgbLedTab::OnPathOptionChanged)
    ON_CBN_SELCHANGE(IDC_RGB_FRAME_COMBO, &CRgbLedTab::OnFrameSelChanged)
    ON_BN_CLICKED(IDC_RGB_ADD_FRAME, &CRgbLedTab::OnAddFrame)
    ON_BN_CLICKED(IDC_RGB_DUP_FRAME, &CRgbLedTab::OnDuplicateFrame)
    ON_BN_CLICKED(IDC_RGB_DEL_FRAME, &CRgbLedTab::OnDeleteFrame)
    ON_EN_CHANGE(IDC_RGB_DURATION_EDIT, &CRgbLedTab::OnDurationChanged)
    ON_CBN_SELCHANGE(IDC_RGB_LED_COMBO, &CRgbLedTab::OnLedSelChanged)
    ON_BN_CLICKED(IDC_RGB_APPLY_LED, &CRgbLedTab::OnApplyLed)
    ON_BN_CLICKED(IDC_RGB_FILL_ALL, &CRgbLedTab::OnFillAll)
    ON_BN_CLICKED(IDC_RGB_CLEAR_FRAME, &CRgbLedTab::OnClearFrame)
    ON_BN_CLICKED(IDC_RGB_GRADIENT, &CRgbLedTab::OnGradient)
    ON_BN_CLICKED(IDC_RGB_SHIFT_LEFT, &CRgbLedTab::OnShiftLeft)
    ON_BN_CLICKED(IDC_RGB_SHIFT_RIGHT, &CRgbLedTab::OnShiftRight)
    ON_CBN_SELCHANGE(IDC_RGB_BACKEND_COMBO, &CRgbLedTab::OnBackendSelChanged)
    ON_BN_CLICKED(IDC_RGB_SYMBOL_HELP, &CRgbLedTab::OnHelpSymbol)
    ON_BN_CLICKED(IDC_RGB_BACKEND_HELP, &CRgbLedTab::OnHelpOutput)
    ON_BN_CLICKED(IDC_RGB_PATH_HELP, &CRgbLedTab::OnHelpPath)
    ON_BN_CLICKED(IDC_RGB_PLAY, &CRgbLedTab::OnPlay)
    ON_BN_CLICKED(IDC_RGB_STOP, &CRgbLedTab::OnStop)
    ON_BN_CLICKED(IDC_RGB_RESET_PATTERN, &CRgbLedTab::OnResetPattern)
    ON_BN_CLICKED(IDC_RGB_COPY_CODE, &CRgbLedTab::OnCopyGeneratedCode)
    ON_BN_CLICKED(IDC_RGB_SAVE_CODE, &CRgbLedTab::OnSaveGeneratedCode)
    ON_BN_CLICKED(IDC_RGB_PREVIOUS_FRAME, &CRgbLedTab::OnPreviousFrame)
    ON_BN_CLICKED(IDC_RGB_NEXT_FRAME, &CRgbLedTab::OnNextFrame)
END_MESSAGE_MAP()

BOOL CRgbLedTab::Create(CWnd* parent, const RECT& rect, UINT id) {
    CString cls = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, ::LoadCursor(nullptr, IDC_ARROW),
                                      reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), nullptr);
    return CWnd::CreateEx(0, cls, L"", WS_CHILD | WS_VISIBLE, rect, parent, id);
}

void CRgbLedTab::Bind(std::function<void(const std::wstring&)> logger) {
    log_ = std::move(logger);
}

int CRgbLedTab::OnCreate(LPCREATESTRUCT lpCreateStruct) {
    if (CWnd::OnCreate(lpCreateStruct) == -1) {
        return -1;
    }

    ui_font_.CreatePointFont(86, L"Segoe UI");
    small_font_.CreatePointFont(76, L"Segoe UI");
    updating_ui_ = true;
    CreateControls();
    updating_ui_ = false;
    SetPattern(rgb_tool::core::MakePreset(L"Moving Dot", rgb_tool::core::kDefaultLedCount, rgb_tool::core::LedShape::Line), 0);
    return 0;
}

void CRgbLedTab::CreateControls() {
    auto mk_static = [this](CStatic& s, const wchar_t* text, UINT id) {
        s.Create(text, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, id);
        mfc_tool::ui::SafeSetFont(s, &ui_font_);
    };
    auto mk_edit = [this](CEdit& e, const wchar_t* text, UINT id) {
        e.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(0, 0, 0, 0), this, id);
        mfc_tool::ui::SafeSetFont(e, &ui_font_);
        e.SetWindowTextW(text);
    };
    auto mk_btn = [this](CButton& b, const wchar_t* text, UINT id, DWORD style = BS_PUSHBUTTON) {
        b.Create(text, WS_CHILD | WS_VISIBLE | style, CRect(0, 0, 0, 0), this, id);
        mfc_tool::ui::SafeSetFont(b, &ui_font_);
    };

    mk_btn(group_, L"Pattern Editor", IDC_RGB_GROUP, BS_GROUPBOX);
    mk_btn(code_group_, L"Code Output", IDC_RGB_CODE_GROUP, BS_GROUPBOX);
    mk_static(led_count_title_, L"LED Count", IDC_RGB_LED_COUNT_TITLE);
    mk_edit(led_count_edit_, L"24", IDC_RGB_LED_COUNT_EDIT);
    mk_static(shape_title_, L"Shape", IDC_RGB_SHAPE_TITLE);
    shape_combo_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, CRect(0, 0, 0, 0), this, IDC_RGB_SHAPE_COMBO);
    mfc_tool::ui::SafeSetFont(shape_combo_, &ui_font_);
    shape_combo_.AddString(L"Line");
    shape_combo_.AddString(L"Ring");
    shape_combo_.AddString(L"Square");
    shape_combo_.AddString(L"U Shape");
    shape_combo_.SetCurSel(0);
    mk_btn(apply_layout_btn_, L"Apply", IDC_RGB_APPLY_LAYOUT);
    mk_static(current_layout_value_, L"Current layout: Line / 24 LEDs", IDC_RGB_CURRENT_LAYOUT);

    mk_static(preset_title_, L"Preset", IDC_RGB_PRESET_TITLE);
    preset_combo_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, CRect(0, 0, 0, 0), this, IDC_RGB_PRESET_COMBO);
    mfc_tool::ui::SafeSetFont(preset_combo_, &ui_font_);
    for (const std::wstring& name : rgb_tool::core::PresetNames()) {
        preset_combo_.AddString(name.c_str());
    }
    preset_combo_.SetCurSel(0);
    mk_btn(load_preset_btn_, L"Load", IDC_RGB_LOAD_PRESET);
    mk_static(current_preset_value_, L"Current preset: Moving Dot", IDC_RGB_CURRENT_PRESET);
    mk_static(symbol_title_, L"C Symbol", IDC_RGB_SYMBOL_TITLE);
    mk_edit(symbol_edit_, L"g_rgb_pattern", IDC_RGB_SYMBOL_EDIT);
    mk_btn(symbol_help_btn_, L"?", IDC_RGB_SYMBOL_HELP);
    mk_static(backend_title_, L"Output", IDC_RGB_BACKEND_TITLE);
    backend_combo_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, CRect(0, 0, 0, 0), this, IDC_RGB_BACKEND_COMBO);
    mfc_tool::ui::SafeSetFont(backend_combo_, &ui_font_);
    backend_combo_.AddString(rgb_tool::core::BackendDisplayName(rgb_tool::core::LedOutputBackend::Llsi).c_str());
    backend_combo_.AddString(rgb_tool::core::BackendDisplayName(rgb_tool::core::LedOutputBackend::Spi).c_str());
    backend_combo_.AddString(rgb_tool::core::BackendDisplayName(rgb_tool::core::LedOutputBackend::SpiPdma).c_str());
    backend_combo_.AddString(rgb_tool::core::BackendDisplayName(rgb_tool::core::LedOutputBackend::Gpio).c_str());
    backend_combo_.SetCurSel(0);
    mk_btn(backend_help_btn_, L"?", IDC_RGB_BACKEND_HELP);
    mk_btn(copy_code_btn_, L"Copy Code", IDC_RGB_COPY_CODE);
    mk_btn(save_code_btn_, L"Save Code...", IDC_RGB_SAVE_CODE);
    mk_static(start_title_, L"Start", IDC_RGB_START_TITLE);
    mk_edit(start_edit_, L"0", IDC_RGB_START_EDIT);
    mk_static(direction_title_, L"Dir", IDC_RGB_DIRECTION_TITLE);
    direction_combo_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, CRect(0, 0, 0, 0), this, IDC_RGB_DIRECTION_COMBO);
    mfc_tool::ui::SafeSetFont(direction_combo_, &ui_font_);
    direction_combo_.AddString(rgb_tool::core::DirectionDisplayName(rgb_tool::core::LedDirection::Forward).c_str());
    direction_combo_.AddString(rgb_tool::core::DirectionDisplayName(rgb_tool::core::LedDirection::Reverse).c_str());
    direction_combo_.SetCurSel(0);
    mk_static(path_mode_title_, L"Motion", IDC_RGB_PATH_MODE_TITLE);
    path_mode_combo_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, CRect(0, 0, 0, 0), this, IDC_RGB_PATH_MODE_COMBO);
    mfc_tool::ui::SafeSetFont(path_mode_combo_, &ui_font_);
    path_mode_combo_.AddString(rgb_tool::core::PathModeDisplayName(rgb_tool::core::LedPathMode::Loop).c_str());
    path_mode_combo_.AddString(rgb_tool::core::PathModeDisplayName(rgb_tool::core::LedPathMode::Bounce).c_str());
    path_mode_combo_.SetCurSel(0);
    mk_btn(path_help_btn_, L"?", IDC_RGB_PATH_HELP);

    mk_static(frame_title_, L"Frame", IDC_RGB_FRAME_TITLE);
    frame_combo_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, CRect(0, 0, 0, 0), this, IDC_RGB_FRAME_COMBO);
    mfc_tool::ui::SafeSetFont(frame_combo_, &ui_font_);
    mk_btn(add_frame_btn_, L"Add", IDC_RGB_ADD_FRAME);
    mk_btn(duplicate_frame_btn_, L"Duplicate", IDC_RGB_DUP_FRAME);
    mk_btn(delete_frame_btn_, L"Delete", IDC_RGB_DEL_FRAME);
    mk_static(duration_title_, L"Duration(ms)", IDC_RGB_DURATION_TITLE);
    mk_edit(duration_edit_, L"80", IDC_RGB_DURATION_EDIT);
    mk_btn(play_btn_, L"Play", IDC_RGB_PLAY);
    mk_btn(stop_btn_, L"Stop", IDC_RGB_STOP);
    mk_btn(reset_pattern_btn_, L"Reset", IDC_RGB_RESET_PATTERN);
    mk_btn(previous_frame_btn_, L"< Previous", IDC_RGB_PREVIOUS_FRAME);
    mk_btn(next_frame_btn_, L"Next >", IDC_RGB_NEXT_FRAME);
    mk_static(playback_status_, L"Frame 1 / 1", IDC_RGB_PLAYBACK_STATUS);
    playback_slider_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_HORZ,
                            CRect(0, 0, 0, 0), this, IDC_RGB_PLAYBACK_SLIDER);
    mk_static(playback_speed_title_, L"Preview", IDC_RGB_PLAYBACK_SPEED_TITLE);
    playback_speed_combo_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                                 CRect(0, 0, 0, 0), this, IDC_RGB_PLAYBACK_SPEED);
    mfc_tool::ui::SafeSetFont(playback_speed_combo_, &ui_font_);
    for (const wchar_t* speed : {L"0.25x", L"0.5x", L"1x", L"2x", L"4x"}) {
        playback_speed_combo_.AddString(speed);
    }
    playback_speed_combo_.SetCurSel(2);

    mk_static(led_title_, L"LED", IDC_RGB_LED_TITLE);
    led_combo_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, CRect(0, 0, 0, 0), this, IDC_RGB_LED_COMBO);
    mfc_tool::ui::SafeSetFont(led_combo_, &ui_font_);
    mk_static(red_title_, L"R", IDC_RGB_RED_TITLE);
    mk_edit(red_edit_, L"0", IDC_RGB_RED_EDIT);
    mk_static(green_title_, L"G", IDC_RGB_GREEN_TITLE);
    mk_edit(green_edit_, L"180", IDC_RGB_GREEN_EDIT);
    mk_static(blue_title_, L"B", IDC_RGB_BLUE_TITLE);
    mk_edit(blue_edit_, L"255", IDC_RGB_BLUE_EDIT);
    mk_btn(choose_color_btn_, L"Choose color...", IDC_RGB_CHOOSE_COLOR);
    mk_static(color2_title_, L"Color 2 RGB", IDC_RGB_COLOR2_TITLE);
    mk_edit(color2_red_edit_, L"0", IDC_RGB_COLOR2_RED);
    mk_edit(color2_green_edit_, L"0", IDC_RGB_COLOR2_GREEN);
    mk_edit(color2_blue_edit_, L"0", IDC_RGB_COLOR2_BLUE);
    mk_btn(choose_color2_btn_, L"Choose color 2...", IDC_RGB_CHOOSE_COLOR2);
    mk_static(tail_title_, L"Tail", IDC_RGB_TAIL_TITLE); mk_edit(tail_edit_, L"3", IDC_RGB_TAIL_EDIT);
    mk_static(fade_title_, L"Fade", IDC_RGB_FADE_TITLE); mk_edit(fade_edit_, L"128", IDC_RGB_FADE_EDIT);
    mk_static(spacing_title_, L"Spacing", IDC_RGB_SPACING_TITLE); mk_edit(spacing_edit_, L"3", IDC_RGB_SPACING_EDIT);
    mk_static(repeat_title_, L"Repeat", IDC_RGB_REPEAT_TITLE); mk_edit(repeat_edit_, L"1", IDC_RGB_REPEAT_EDIT);
    mk_btn(apply_led_btn_, L"Apply LED", IDC_RGB_APPLY_LED);
    mk_btn(fill_all_btn_, L"Fill All", IDC_RGB_FILL_ALL);
    mk_btn(clear_frame_btn_, L"Clear Frame", IDC_RGB_CLEAR_FRAME);
    mk_btn(gradient_btn_, L"Rainbow Fill", IDC_RGB_GRADIENT);
    mk_btn(shift_left_btn_, L"Shift Left", IDC_RGB_SHIFT_LEFT);
    mk_btn(shift_right_btn_, L"Shift Right", IDC_RGB_SHIFT_RIGHT);

    mk_static(preview_title_, L"Preview", IDC_RGB_PREVIEW_TITLE);
    preview_.Create(this, CRect(0, 0, 0, 0), IDC_RGB_PREVIEW);
    mk_static(code_title_, L"Generated MCU Source Bundle", IDC_RGB_CODE_TITLE);
    code_edit_.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_HSCROLL |
                          ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY,
                      CRect(0, 0, 0, 0), this, IDC_RGB_CODE_EDIT);
    mfc_tool::ui::SafeSetFont(code_edit_, &small_font_);
}

void CRgbLedTab::OnSize(UINT nType, int cx, int cy) {
    CWnd::OnSize(nType, cx, cy);
    LayoutControls(cx, cy);
}

void CRgbLedTab::LayoutControls(int cx, int cy) {
    if (cx <= 0 || cy <= 0) {
        return;
    }

    const int margin = 8;
    const int gap = 6;
    const int row = 26;
    const int left_w = (std::max)(500, (std::min)(540, cx / 3));
    const int left = margin + 12;
    const int right_x = margin + left_w + 16;
    const int right_w = (std::max)(260, cx - right_x - margin - 10);
    const int left_group_w = right_x - margin - gap;
    const int code_group_h = 100;
    const int code_group_y = cy - margin - code_group_h;
    const int bottom_button_x = margin + 12;
    const int bottom_button_y = code_group_y - row - gap;
    const int bottom_button_w = (std::max)(1, right_x - gap - bottom_button_x);
    const int third_button_w = (std::max)(1, (bottom_button_w - gap * 2) / 3);
    const int playback_y = bottom_button_y - row - gap;
    int y = margin + 26;
    int x = left;
    int btn_w = 92;

    mfc_tool::ui::SafeMoveWindow(group_, margin, margin, left_group_w, code_group_y - margin - gap);
    mfc_tool::ui::SafeMoveWindow(code_group_, margin, code_group_y, left_group_w, code_group_h);

    x = left;
    x = mfc_tool::ui::PlaceLabelAndControl(led_count_title_, led_count_edit_, x, y + 4, y, 54, row, gap) + 10;
    x = mfc_tool::ui::PlaceLabelAndControl(shape_title_, shape_combo_, x, y + 4, y, 104, 200, gap) + 8;
    mfc_tool::ui::SafeMoveWindow(apply_layout_btn_, x, y, 60, row);
    mfc_tool::ui::SafeMoveWindow(current_layout_value_, x + 66, y + 5, left_group_w - (x + 66 - margin) - 10, 18);

    y += row + gap;
    x = left;
    x = mfc_tool::ui::PlaceLabelAndControl(preset_title_, preset_combo_, x, y + 4, y, 170, 220, gap) + gap;
    mfc_tool::ui::SafeMoveWindow(load_preset_btn_, x, y, 60, row);
    mfc_tool::ui::SafeMoveWindow(current_preset_value_, x + 66, y + 5, left_group_w - (x + 66 - margin) - 10, 18);

    y += row + gap + 8;
    x = left;
    x = mfc_tool::ui::PlaceLabelAndControl(start_title_, start_edit_, x, y + 4, y, 46, row, gap) + 10;
    x = mfc_tool::ui::PlaceLabelAndControl(direction_title_, direction_combo_, x, y + 4, y, 90, 120, gap) + 10;
    x = mfc_tool::ui::PlaceLabelAndControl(path_mode_title_, path_mode_combo_, x, y + 4, y, 92, 120, gap) + gap;
    mfc_tool::ui::SafeMoveWindow(path_help_btn_, x, y, 24, row);

    y += row + gap + 8;
    x = left;
    mfc_tool::ui::PlaceLabel(frame_title_, x, y + 4);
    mfc_tool::ui::SafeMoveWindow(frame_combo_, x + 54, y, 164, 220);
    x += 226;
    mfc_tool::ui::SafeMoveWindow(add_frame_btn_, x, y, 58, row);
    x += 64;
    mfc_tool::ui::SafeMoveWindow(duplicate_frame_btn_, x, y, 88, row);
    x += 94;
    mfc_tool::ui::SafeMoveWindow(delete_frame_btn_, x, y, 64, row);

    y += row + gap;
    x = left;
    mfc_tool::ui::PlaceLabelAndControl(duration_title_, duration_edit_, x, y + 4, y, 72, row, gap);

    y += row + gap + 8;
    x = left;
    x = mfc_tool::ui::PlaceLabelAndControl(led_title_, led_combo_, x, y + 4, y, 76, 180, gap) + gap;
    x = mfc_tool::ui::PlaceLabelAndControl(red_title_, red_edit_, x, y + 4, y, 48, row, gap, 4) + gap;
    x = mfc_tool::ui::PlaceLabelAndControl(green_title_, green_edit_, x, y + 4, y, 48, row, gap, 4) + gap;
    x = mfc_tool::ui::PlaceLabelAndControl(blue_title_, blue_edit_, x, y + 4, y, 48, row, gap, 4) + gap;
    mfc_tool::ui::SafeMoveWindow(choose_color_btn_, x, y, 112, row);

    y += row + gap;
    x = left;
    btn_w = 92;
    mfc_tool::ui::SafeMoveWindow(apply_led_btn_, x, y, btn_w, row);
    x += btn_w + gap;
    mfc_tool::ui::SafeMoveWindow(fill_all_btn_, x, y, btn_w, row);
    x += btn_w + gap;
    mfc_tool::ui::SafeMoveWindow(clear_frame_btn_, x, y, 104, row);

    y += row + gap;
    x = left;
    mfc_tool::ui::SafeMoveWindow(gradient_btn_, x, y, 112, row);
    x += 118;
    mfc_tool::ui::SafeMoveWindow(shift_left_btn_, x, y, 92, row);
    x += 98;
    mfc_tool::ui::SafeMoveWindow(shift_right_btn_, x, y, 96, row);

    y += row + gap;
    mfc_tool::ui::SafeMoveWindow(color2_title_, left, y + 4, 76, 18);
    mfc_tool::ui::SafeMoveWindow(color2_red_edit_, left + 80, y, 48, row);
    mfc_tool::ui::SafeMoveWindow(color2_green_edit_, left + 134, y, 48, row);
    mfc_tool::ui::SafeMoveWindow(color2_blue_edit_, left + 188, y, 48, row);
    mfc_tool::ui::SafeMoveWindow(choose_color2_btn_, left + 242, y, 120, row);
    y += row + gap;
    mfc_tool::ui::PlaceLabelAndControl(tail_title_, tail_edit_, left, y + 4, y, 42, row, gap);
    mfc_tool::ui::PlaceLabelAndControl(fade_title_, fade_edit_, left + 100, y + 4, y, 48, row, gap);
    mfc_tool::ui::PlaceLabelAndControl(spacing_title_, spacing_edit_, left + 210, y + 4, y, 46, row, gap);
    mfc_tool::ui::PlaceLabelAndControl(repeat_title_, repeat_edit_, left + 350, y + 4, y, 46, row, gap);

    y += row + gap + 8;
    mfc_tool::ui::SafeMoveWindow(preview_title_, right_x, margin + 10, 160, 18);
    mfc_tool::ui::SafeMoveWindow(preview_, right_x, margin + 32, right_w, (std::max)(180, cy / 2 - 44));
    mfc_tool::ui::SafeMoveWindow(code_title_, right_x, margin + 40 + (std::max)(180, cy / 2 - 44), 220, 18);
    mfc_tool::ui::SafeMoveWindow(code_edit_, right_x, margin + 62 + (std::max)(180, cy / 2 - 44),
                                 right_w, cy - (margin + 70 + (std::max)(180, cy / 2 - 44)) - margin);
    mfc_tool::ui::SafeMoveWindow(play_btn_, bottom_button_x, bottom_button_y, third_button_w, row);
    mfc_tool::ui::SafeMoveWindow(stop_btn_, bottom_button_x + third_button_w + gap,
                                 bottom_button_y, third_button_w, row);
    mfc_tool::ui::SafeMoveWindow(reset_pattern_btn_, bottom_button_x + (third_button_w + gap) * 2,
                                 bottom_button_y, bottom_button_w - third_button_w * 2 - gap * 2, row);
    mfc_tool::ui::SafeMoveWindow(previous_frame_btn_, bottom_button_x, playback_y, 84, row);
    mfc_tool::ui::SafeMoveWindow(next_frame_btn_, bottom_button_x + 90, playback_y, 70, row);
    mfc_tool::ui::SafeMoveWindow(playback_status_, bottom_button_x + 168, playback_y + 5, 100, 18);
    mfc_tool::ui::SafeMoveWindow(playback_slider_, bottom_button_x + 270, playback_y, 140, row);
    mfc_tool::ui::SafeMoveWindow(playback_speed_title_, bottom_button_x + 416, playback_y + 5, 48, 18);
    mfc_tool::ui::SafeMoveWindow(playback_speed_combo_, bottom_button_x + 466, playback_y, 66, 140);
    x = left;
    y = code_group_y + 22;
    x = mfc_tool::ui::PlaceLabelAndControl(symbol_title_, symbol_edit_, x, y + 4, y, 150, row, gap) + gap;
    mfc_tool::ui::SafeMoveWindow(symbol_help_btn_, x, y, 24, row);
    x += 30;
    x = mfc_tool::ui::PlaceLabelAndControl(backend_title_, backend_combo_, x, y + 4, y, 100, 150, gap) + gap;
    mfc_tool::ui::SafeMoveWindow(backend_help_btn_, x, y, 24, row);
    y += row + gap;
    mfc_tool::ui::SafeMoveWindow(copy_code_btn_, left, y, (left_group_w - 24 - gap) / 2, row);
    mfc_tool::ui::SafeMoveWindow(save_code_btn_, left + (left_group_w - 24 - gap) / 2 + gap, y,
                                 left_group_w - 24 - (left_group_w - 24 - gap) / 2 - gap, row);
}

void CRgbLedTab::LoadState(const rgb_tool::core::AppState& state) {
    const int count = rgb_tool::core::ClampInt(ParseIntOr(state.pattern.led_count, rgb_tool::core::kDefaultLedCount),
                                               rgb_tool::core::kMinLedCount, rgb_tool::core::kMaxLedCount);
    const rgb_tool::core::LedShape shape = rgb_tool::core::ParseShapeName(state.pattern.shape);
    const int start_index = rgb_tool::core::ClampInt(ParseIntOr(state.pattern.start_index, 0), 0, count - 1);
    const rgb_tool::core::LedDirection direction = rgb_tool::core::ParseDirectionName(state.pattern.direction);
    const rgb_tool::core::LedPathMode path_mode = rgb_tool::core::ParsePathModeName(state.pattern.path_mode);
    const std::wstring preset = state.pattern.preset.empty() ? L"Moving Dot" : state.pattern.preset;

    SetControlText(led_count_edit_, std::to_wstring(count));
    shape_combo_.SetCurSel(static_cast<int>(shape));
    SetControlText(symbol_edit_, state.pattern.symbol_name.empty() ? L"g_rgb_pattern" : state.pattern.symbol_name);
    SetControlText(start_edit_, std::to_wstring(start_index));
    direction_combo_.SetCurSel(static_cast<int>(direction));
    path_mode_combo_.SetCurSel(static_cast<int>(path_mode));
    backend_combo_.SetCurSel(static_cast<int>(rgb_tool::core::ParseBackendName(state.pattern.backend)));
    SetControlText(color2_red_edit_, state.pattern.color2_red);
    SetControlText(color2_green_edit_, state.pattern.color2_green);
    SetControlText(color2_blue_edit_, state.pattern.color2_blue);
    SetControlText(tail_edit_, state.pattern.tail_length);
    SetControlText(fade_edit_, state.pattern.trail_fade);
    SetControlText(spacing_edit_, state.pattern.chase_spacing);
    SetControlText(repeat_edit_, state.pattern.repeat_count);
    {
        const int found = preset_combo_.FindStringExact(-1, preset.c_str());
        preset_combo_.SetCurSel(found == CB_ERR ? 0 : found);
    }
    SetPattern(rgb_tool::core::MakePreset(preset, count, shape, start_index, direction, path_mode), 0);
}

void CRgbLedTab::SaveState(rgb_tool::core::AppState* state) const {
    if (state == nullptr) {
        return;
    }
    state->pattern.led_count = GetControlText(led_count_edit_);
    state->pattern.shape = rgb_tool::core::ShapePersistName(SelectedShape());
    {
        CString text;
        preset_combo_.GetWindowTextW(text);
        state->pattern.preset = text.GetString();
    }
    state->pattern.symbol_name = GetControlText(symbol_edit_);
    state->pattern.start_index = GetControlText(start_edit_);
    state->pattern.direction = rgb_tool::core::DirectionPersistName(SelectedDirection());
    state->pattern.path_mode = rgb_tool::core::PathModePersistName(SelectedPathMode());
    state->pattern.backend = rgb_tool::core::BackendPersistName(SelectedBackend());
    state->pattern.color2_red = GetControlText(color2_red_edit_);
    state->pattern.color2_green = GetControlText(color2_green_edit_);
    state->pattern.color2_blue = GetControlText(color2_blue_edit_);
    state->pattern.tail_length = GetControlText(tail_edit_);
    state->pattern.trail_fade = GetControlText(fade_edit_);
    state->pattern.chase_spacing = GetControlText(spacing_edit_);
    state->pattern.repeat_count = GetControlText(repeat_edit_);
}

bool CRgbLedTab::LoadPatternFromFile(const std::wstring& path, std::wstring* error) {
    rgb_tool::core::RgbPattern pattern;
    if (!rgb_tool::core::LoadPatternFile(path, &pattern, error)) {
        return false;
    }
    SetPattern(pattern, 0);
    if (log_) {
        log_(L"Pattern loaded: " + path);
    }
    return true;
}

bool CRgbLedTab::SavePatternToFile(const std::wstring& path, std::wstring* error) const {
    return rgb_tool::core::SavePatternFile(pattern_, path, error);
}

bool CRgbLedTab::SaveGeneratedCodeToFolder(const std::wstring& directory, std::wstring* error) const {
    const auto files = rgb_tool::core::GenerateSourceFiles(pattern_, GetControlText(symbol_edit_), SelectedBackend());
    return rgb_tool::core::SaveGeneratedSourceFiles(directory, files, error);
}

std::wstring CRgbLedTab::CurrentCode() const {
    return rgb_tool::core::GeneratePatternCode(pattern_, GetControlText(symbol_edit_), SelectedBackend());
}

bool CRgbLedTab::CopyCodeToClipboard() const {
    const std::wstring text = CurrentCode();
    const size_t bytes = (text.size() + 1u) * sizeof(wchar_t);
    HGLOBAL data = nullptr;
    wchar_t* locked = nullptr;

    if (!::OpenClipboard(GetSafeHwnd())) {
        return false;
    }
    EmptyClipboard();
    data = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (data == nullptr) {
        CloseClipboard();
        return false;
    }
    locked = static_cast<wchar_t*>(GlobalLock(data));
    if (locked == nullptr) {
        GlobalFree(data);
        CloseClipboard();
        return false;
    }
    memcpy(locked, text.c_str(), bytes);
    GlobalUnlock(data);
    SetClipboardData(CF_UNICODETEXT, data);
    CloseClipboard();
    return true;
}

void CRgbLedTab::OnCopyGeneratedCode() {
    if (!CopyCodeToClipboard()) {
        MessageBoxW(L"Clipboard copy failed.", L"Copy Code", MB_ICONERROR | MB_OK);
        return;
    }
    if (log_) log_(L"Generated code copied to clipboard.");
}

void CRgbLedTab::OnSaveGeneratedCode() {
    CFolderPickerDialog dialog(nullptr, 0, this);
    if (dialog.DoModal() == IDOK) {
        std::wstring error;
        const std::wstring path = dialog.GetPathName().GetString();
        if (!SaveGeneratedCodeToFolder(path, &error)) {
            MessageBoxW(error.c_str(), L"Save Code Error", MB_ICONERROR | MB_OK);
            return;
        }
        if (log_) log_(L"Generated MCU source files saved: " + path);
    }
}

void CRgbLedTab::SetPattern(rgb_tool::core::RgbPattern pattern, int frame_index) {
    pattern_ = std::move(pattern);
    rgb_tool::core::EnsurePatternShape(&pattern_, pattern_.led_count, pattern_.shape);
    current_frame_ = rgb_tool::core::ClampInt(frame_index, 0, static_cast<int>(pattern_.frames.size()) - 1);
    SetControlText(led_count_edit_, std::to_wstring(pattern_.led_count));
    shape_combo_.SetCurSel(static_cast<int>(pattern_.shape));
    SetControlText(start_edit_, std::to_wstring(pattern_.start_index));
    direction_combo_.SetCurSel(static_cast<int>(pattern_.direction));
    path_mode_combo_.SetCurSel(static_cast<int>(pattern_.path_mode));
    UpdateAllViews();
}

void CRgbLedTab::UpdateAllViews() {
    UpdateFrameCombo();
    UpdateLedCombo();
    UpdateSelectedFrameFields();
    UpdateSelectedLedFields();
    UpdatePreview();
    UpdateCodeView();
    UpdatePlayState();
    UpdatePlaybackControls();
    UpdateAppliedStatus();
}

void CRgbLedTab::UpdateCodeView() {
    if (::IsWindow(code_edit_.GetSafeHwnd())) {
        code_edit_.SetWindowTextW(CurrentCode().c_str());
    }
}

void CRgbLedTab::UpdatePreview() {
    preview_.SetPattern(&pattern_, current_frame_);
}

void CRgbLedTab::UpdateFrameCombo() {
    int i = 0;
    if (!::IsWindow(frame_combo_.GetSafeHwnd())) {
        return;
    }
    frame_combo_.ResetContent();
    for (i = 0; i < static_cast<int>(pattern_.frames.size()); ++i) {
        CString s;
        s.Format(L"%d  (%u ms)", i, static_cast<unsigned int>(pattern_.frames[static_cast<size_t>(i)].duration_ms));
        frame_combo_.AddString(s);
    }
    current_frame_ = rgb_tool::core::ClampInt(current_frame_, 0, static_cast<int>(pattern_.frames.size()) - 1);
    frame_combo_.SetCurSel(current_frame_);
}

void CRgbLedTab::UpdateLedCombo() {
    int i = 0;
    int old_sel = SelectedLedIndex();
    if (!::IsWindow(led_combo_.GetSafeHwnd())) {
        return;
    }
    led_combo_.ResetContent();
    for (i = 0; i < pattern_.led_count; ++i) {
        CString s;
        s.Format(L"%d", i);
        led_combo_.AddString(s);
    }
    if (old_sel < 0 || old_sel >= pattern_.led_count) {
        old_sel = 0;
    }
    led_combo_.SetCurSel(old_sel);
}

void CRgbLedTab::UpdateSelectedFrameFields() {
    const rgb_tool::core::PatternFrame* frame = CurrentFrame();
    if (frame == nullptr) {
        return;
    }
    updating_ui_ = true;
    SetControlText(duration_edit_, std::to_wstring(static_cast<unsigned int>(frame->duration_ms)));
    updating_ui_ = false;
}

void CRgbLedTab::UpdateSelectedLedFields() {
    const rgb_tool::core::PatternFrame* frame = CurrentFrame();
    const int led = SelectedLedIndex();
    if (frame == nullptr || led < 0 || led >= static_cast<int>(frame->leds.size())) {
        return;
    }
    const rgb_tool::core::RgbColor& color = frame->leds[static_cast<size_t>(led)];
    updating_ui_ = true;
    SetControlText(red_edit_, std::to_wstring(static_cast<unsigned int>(color.r)));
    SetControlText(green_edit_, std::to_wstring(static_cast<unsigned int>(color.g)));
    SetControlText(blue_edit_, std::to_wstring(static_cast<unsigned int>(color.b)));
    updating_ui_ = false;
}

void CRgbLedTab::UpdatePlayState() {
    mfc_tool::ui::SafeEnableWindow(play_btn_, playing_ ? FALSE : TRUE);
    mfc_tool::ui::SafeEnableWindow(stop_btn_, playing_ ? TRUE : FALSE);
}

void CRgbLedTab::UpdatePlaybackControls() {
    CString status;
    const int frame_count = static_cast<int>(pattern_.frames.size());
    const int maximum = (std::max)(0, frame_count - 1);
    status.Format(L"Frame %d / %d", frame_count == 0 ? 0 : current_frame_ + 1, frame_count);
    SetControlText(playback_status_, status.GetString());
    if (::IsWindow(playback_slider_.GetSafeHwnd())) {
        playback_slider_.SetRange(0, maximum, TRUE);
        playback_slider_.SetPos((std::min)(current_frame_, maximum));
    }
}

void CRgbLedTab::UpdateAppliedStatus() {
    CString layout;
    layout.Format(L"Current layout: %s / %d LEDs",
                  rgb_tool::core::ShapeDisplayName(pattern_.shape).c_str(), pattern_.led_count);
    SetControlText(current_layout_value_, layout.GetString());
    SetControlText(current_preset_value_, L"Current preset: " + pattern_.name);
}

unsigned int CRgbLedTab::PreviewIntervalMs() const {
    static const unsigned int numerators[] = {4u, 2u, 1u, 1u, 1u};
    static const unsigned int denominators[] = {1u, 1u, 1u, 2u, 4u};
    const int selection = rgb_tool::core::ClampInt(playback_speed_combo_.GetCurSel(), 0, 4);
    const unsigned int duration = pattern_.frames.empty() ? 20u :
        static_cast<unsigned int>(pattern_.frames[static_cast<size_t>(current_frame_)].duration_ms);
    return (std::max)(20u, (duration * numerators[selection]) / denominators[selection]);
}

void CRgbLedTab::ApplyCurrentLedCountAndShape() {
    const int count = rgb_tool::core::ClampInt(ParseEditInt(led_count_edit_, pattern_.led_count),
                                               rgb_tool::core::kMinLedCount, rgb_tool::core::kMaxLedCount);
    const rgb_tool::core::LedShape shape = SelectedShape();
    rgb_tool::core::EnsurePatternShape(&pattern_, count, shape);
    rgb_tool::core::ApplyPatternPathSettings(&pattern_, SelectedStartIndex(), SelectedDirection(), SelectedPathMode());
    current_frame_ = rgb_tool::core::ClampInt(current_frame_, 0, static_cast<int>(pattern_.frames.size()) - 1);
    SetControlText(led_count_edit_, std::to_wstring(pattern_.led_count));
    SetControlText(start_edit_, std::to_wstring(pattern_.start_index));
}

int CRgbLedTab::SelectedFrameIndex() const {
    const int sel = frame_combo_.GetCurSel();
    if (sel == CB_ERR) {
        return current_frame_;
    }
    return sel;
}

int CRgbLedTab::SelectedLedIndex() const {
    const int sel = led_combo_.GetCurSel();
    if (sel == CB_ERR) {
        return 0;
    }
    return sel;
}

int CRgbLedTab::ParseEditInt(const CEdit& edit, int fallback) const {
    return ParseIntOr(GetControlText(edit), fallback);
}

std::wstring CRgbLedTab::GetControlText(const CWnd& wnd) const {
    CString s;
    const_cast<CWnd&>(wnd).GetWindowTextW(s);
    return s.GetString();
}

void CRgbLedTab::SetControlText(CWnd& wnd, const std::wstring& text) {
    if (::IsWindow(wnd.GetSafeHwnd())) {
        wnd.SetWindowTextW(text.c_str());
    }
}

rgb_tool::core::LedShape CRgbLedTab::SelectedShape() const {
    const int sel = shape_combo_.GetCurSel();
    switch (sel) {
    case 1: return rgb_tool::core::LedShape::Ring;
    case 2: return rgb_tool::core::LedShape::Square;
    case 3: return rgb_tool::core::LedShape::UShape;
    case 0:
    default: return rgb_tool::core::LedShape::Line;
    }
}

int CRgbLedTab::SelectedStartIndex() const {
    const int fallback = pattern_.start_index;
    return rgb_tool::core::ClampInt(ParseEditInt(start_edit_, fallback), 0, (std::max)(0, pattern_.led_count - 1));
}

rgb_tool::core::LedDirection CRgbLedTab::SelectedDirection() const {
    const int sel = direction_combo_.GetCurSel();
    return sel == 1 ? rgb_tool::core::LedDirection::Reverse : rgb_tool::core::LedDirection::Forward;
}

rgb_tool::core::LedPathMode CRgbLedTab::SelectedPathMode() const {
    const int sel = path_mode_combo_.GetCurSel();
    return sel == 1 ? rgb_tool::core::LedPathMode::Bounce : rgb_tool::core::LedPathMode::Loop;
}

rgb_tool::core::LedOutputBackend CRgbLedTab::SelectedBackend() const {
    const int sel = backend_combo_.GetCurSel();
    switch (sel) {
    case 1: return rgb_tool::core::LedOutputBackend::Spi;
    case 2: return rgb_tool::core::LedOutputBackend::SpiPdma;
    case 3: return rgb_tool::core::LedOutputBackend::Gpio;
    case 0:
    default: return rgb_tool::core::LedOutputBackend::Llsi;
    }
}

rgb_tool::core::RgbColor CRgbLedTab::ParseColorEdits() const {
    rgb_tool::core::RgbColor color;
    color.r = static_cast<std::uint8_t>(rgb_tool::core::ClampInt(ParseEditInt(red_edit_, 0), 0, 255));
    color.g = static_cast<std::uint8_t>(rgb_tool::core::ClampInt(ParseEditInt(green_edit_, 0), 0, 255));
    color.b = static_cast<std::uint8_t>(rgb_tool::core::ClampInt(ParseEditInt(blue_edit_, 0), 0, 255));
    return color;
}

rgb_tool::core::RgbColor CRgbLedTab::ParseSecondColorEdits() const {
    rgb_tool::core::RgbColor color;
    color.r = static_cast<std::uint8_t>(rgb_tool::core::ClampInt(ParseEditInt(color2_red_edit_, 0), 0, 255));
    color.g = static_cast<std::uint8_t>(rgb_tool::core::ClampInt(ParseEditInt(color2_green_edit_, 0), 0, 255));
    color.b = static_cast<std::uint8_t>(rgb_tool::core::ClampInt(ParseEditInt(color2_blue_edit_, 0), 0, 255));
    return color;
}

rgb_tool::core::PatternFrame* CRgbLedTab::CurrentFrame() {
    if (pattern_.frames.empty()) {
        return nullptr;
    }
    current_frame_ = rgb_tool::core::ClampInt(current_frame_, 0, static_cast<int>(pattern_.frames.size()) - 1);
    return &pattern_.frames[static_cast<size_t>(current_frame_)];
}

const rgb_tool::core::PatternFrame* CRgbLedTab::CurrentFrame() const {
    if (pattern_.frames.empty()) {
        return nullptr;
    }
    const int frame = rgb_tool::core::ClampInt(current_frame_, 0, static_cast<int>(pattern_.frames.size()) - 1);
    return &pattern_.frames[static_cast<size_t>(frame)];
}

std::wstring CRgbLedTab::AnsiToWide(const char* text) {
    if (text == nullptr) {
        return L"";
    }
    const int n = MultiByteToWideChar(CP_ACP, 0, text, -1, nullptr, 0);
    if (n <= 0) {
        return L"";
    }
    std::wstring out(static_cast<size_t>(n - 1), L'\0');
    MultiByteToWideChar(CP_ACP, 0, text, -1, out.data(), n);
    return out;
}

void CRgbLedTab::OnTimer(UINT_PTR nIDEvent) {
    if (nIDEvent != kPreviewTimerId || pattern_.frames.empty()) {
        CWnd::OnTimer(nIDEvent);
        return;
    }
    current_frame_++;
    if (current_frame_ >= static_cast<int>(pattern_.frames.size())) {
        current_frame_ = 0;
    }
    frame_combo_.SetCurSel(current_frame_);
    UpdateSelectedFrameFields();
    UpdateSelectedLedFields();
    UpdatePreview();
    UpdatePlaybackControls();
    KillTimer(kPreviewTimerId);
    SetTimer(kPreviewTimerId, PreviewIntervalMs(), nullptr);
}

void CRgbLedTab::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) {
    (void)nSBCode;
    (void)nPos;
    if (pScrollBar != nullptr && pScrollBar->GetSafeHwnd() == playback_slider_.GetSafeHwnd() &&
        !pattern_.frames.empty()) {
        current_frame_ = rgb_tool::core::ClampInt(playback_slider_.GetPos(), 0,
                                                  static_cast<int>(pattern_.frames.size()) - 1);
        frame_combo_.SetCurSel(current_frame_);
        UpdateSelectedFrameFields();
        UpdateSelectedLedFields();
        UpdatePreview();
        UpdatePlaybackControls();
    }
    CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CRgbLedTab::OnApplyLayout() {
    ApplyCurrentLedCountAndShape();
    UpdateAllViews();
    UpdateAppliedStatus();
    if (log_) {
        log_(L"Layout applied: " + std::to_wstring(pattern_.led_count) + L" LEDs, " +
             rgb_tool::core::ShapeDisplayName(pattern_.shape));
    }
}

void CRgbLedTab::OnLoadPreset() {
    CString preset_text;
    preset_combo_.GetWindowTextW(preset_text);
    ApplyCurrentLedCountAndShape();
    SetPattern(rgb_tool::core::MakePreset(
        preset_text.GetString(),
        pattern_.led_count,
        pattern_.shape,
        pattern_.start_index,
        pattern_.direction,
        pattern_.path_mode,
        ParseColorEdits(),
        ParseSecondColorEdits(),
        ParseEditInt(tail_edit_, 3),
        ParseEditInt(fade_edit_, 128),
        ParseEditInt(spacing_edit_, 3),
        ParseEditInt(repeat_edit_, 1)
    ), 0);
    UpdateAppliedStatus();
    if (log_) {
        log_(L"Preset loaded: " + std::wstring(preset_text.GetString()));
    }
}

void CRgbLedTab::OnChoosePatternColor() {
    const rgb_tool::core::RgbColor current = ParseColorEdits();
    CColorDialog dialog(RGB(current.r, current.g, current.b), CC_FULLOPEN, this);
    if (dialog.DoModal() != IDOK) {
        return;
    }
    const COLORREF color = dialog.GetColor();
    updating_ui_ = true;
    SetControlText(red_edit_, std::to_wstring(static_cast<unsigned int>(GetRValue(color))));
    SetControlText(green_edit_, std::to_wstring(static_cast<unsigned int>(GetGValue(color))));
    SetControlText(blue_edit_, std::to_wstring(static_cast<unsigned int>(GetBValue(color))));
    updating_ui_ = false;
}

void CRgbLedTab::OnChooseSecondColor() {
    const rgb_tool::core::RgbColor current = ParseSecondColorEdits();
    CColorDialog dialog(RGB(current.r, current.g, current.b), CC_FULLOPEN, this);
    if (dialog.DoModal() != IDOK) return;
    const COLORREF color = dialog.GetColor();
    SetControlText(color2_red_edit_, std::to_wstring(static_cast<unsigned int>(GetRValue(color))));
    SetControlText(color2_green_edit_, std::to_wstring(static_cast<unsigned int>(GetGValue(color))));
    SetControlText(color2_blue_edit_, std::to_wstring(static_cast<unsigned int>(GetBValue(color))));
}

void CRgbLedTab::OnPathOptionChanged() {
    if (updating_ui_) {
        return;
    }
    rgb_tool::core::ApplyPatternPathSettings(&pattern_, SelectedStartIndex(), SelectedDirection(), SelectedPathMode());
    updating_ui_ = true;
    SetControlText(start_edit_, std::to_wstring(pattern_.start_index));
    updating_ui_ = false;
    UpdateSelectedLedFields();
    UpdatePreview();
    UpdateCodeView();
}

void CRgbLedTab::OnFrameSelChanged() {
    current_frame_ = SelectedFrameIndex();
    UpdateSelectedFrameFields();
    UpdateSelectedLedFields();
    UpdatePreview();
    UpdateCodeView();
    UpdatePlaybackControls();
}

void CRgbLedTab::OnAddFrame() {
    rgb_tool::core::PatternFrame frame;
    frame.duration_ms = rgb_tool::core::kDefaultFrameDurationMs;
    frame.leds.assign(static_cast<size_t>(pattern_.led_count), rgb_tool::core::RgbColor{});
    current_frame_++;
    if (current_frame_ < 0 || current_frame_ > static_cast<int>(pattern_.frames.size())) {
        current_frame_ = static_cast<int>(pattern_.frames.size());
    }
    pattern_.frames.insert(pattern_.frames.begin() + current_frame_, frame);
    UpdateAllViews();
}

void CRgbLedTab::OnDuplicateFrame() {
    const rgb_tool::core::PatternFrame* frame = CurrentFrame();
    if (frame == nullptr) {
        return;
    }
    current_frame_++;
    pattern_.frames.insert(pattern_.frames.begin() + current_frame_, *frame);
    UpdateAllViews();
}

void CRgbLedTab::OnDeleteFrame() {
    if (pattern_.frames.size() <= 1u) {
        return;
    }
    pattern_.frames.erase(pattern_.frames.begin() + current_frame_);
    current_frame_ = rgb_tool::core::ClampInt(current_frame_, 0, static_cast<int>(pattern_.frames.size()) - 1);
    UpdateAllViews();
}

void CRgbLedTab::OnDurationChanged() {
    rgb_tool::core::PatternFrame* frame = CurrentFrame();
    std::uint16_t duration_ms = 0u;
    if (updating_ui_) {
        return;
    }
    if (frame == nullptr || !::IsWindow(duration_edit_.GetSafeHwnd())) {
        return;
    }
    duration_ms = static_cast<std::uint16_t>(
        rgb_tool::core::ClampInt(ParseEditInt(duration_edit_, frame->duration_ms), 1, 60000));
    for (rgb_tool::core::PatternFrame& pattern_frame : pattern_.frames) {
        pattern_frame.duration_ms = duration_ms;
    }
    UpdateFrameCombo();
    UpdateCodeView();
}

void CRgbLedTab::OnLedSelChanged() {
    UpdateSelectedLedFields();
}

void CRgbLedTab::OnApplyLed() {
    rgb_tool::core::PatternFrame* frame = CurrentFrame();
    const int led = SelectedLedIndex();
    if (frame == nullptr || led < 0 || led >= static_cast<int>(frame->leds.size())) {
        return;
    }
    frame->leds[static_cast<size_t>(led)] = ParseColorEdits();
    UpdatePreview();
    UpdateCodeView();
}

void CRgbLedTab::OnFillAll() {
    rgb_tool::core::PatternFrame* frame = CurrentFrame();
    if (frame == nullptr) {
        return;
    }
    std::fill(frame->leds.begin(), frame->leds.end(), ParseColorEdits());
    UpdatePreview();
    UpdateCodeView();
}

void CRgbLedTab::OnClearFrame() {
    rgb_tool::core::PatternFrame* frame = CurrentFrame();
    if (frame == nullptr) {
        return;
    }
    std::fill(frame->leds.begin(), frame->leds.end(), rgb_tool::core::RgbColor{});
    UpdateSelectedLedFields();
    UpdatePreview();
    UpdateCodeView();
}

void CRgbLedTab::OnGradient() {
    unsigned int pattern_peak = 0u;
    size_t frame_index = 0u;
    int i = 0;
    if (pattern_.frames.empty()) {
        return;
    }

    for (frame_index = 0u; frame_index < pattern_.frames.size(); ++frame_index) {
        const rgb_tool::core::PatternFrame& frame = pattern_.frames[frame_index];
        for (i = 0; i < static_cast<int>(frame.leds.size()); ++i) {
            const rgb_tool::core::RgbColor& color = frame.leds[static_cast<size_t>(i)];
            const unsigned int led_peak = (std::max)(
                static_cast<unsigned int>(color.r),
                (std::max)(static_cast<unsigned int>(color.g), static_cast<unsigned int>(color.b)));
            pattern_peak = (std::max)(pattern_peak, led_peak);
        }
    }

    for (frame_index = 0u; frame_index < pattern_.frames.size(); ++frame_index) {
        rgb_tool::core::PatternFrame& frame = pattern_.frames[frame_index];
        for (i = 0; i < static_cast<int>(frame.leds.size()); ++i) {
            const rgb_tool::core::RgbColor old_color = frame.leds[static_cast<size_t>(i)];
            unsigned int level = (std::max)(
                static_cast<unsigned int>(old_color.r),
                (std::max)(static_cast<unsigned int>(old_color.g), static_cast<unsigned int>(old_color.b)));
            rgb_tool::core::RgbColor rainbow = rgb_tool::core::ColorWheel(
                (i * 256) / (std::max)(1, static_cast<int>(frame.leds.size())));

            if (pattern_peak == 0u) {
                level = 255u;
            } else {
                level = (level * 255u) / pattern_peak;
            }
            rainbow.r = static_cast<std::uint8_t>((static_cast<unsigned int>(rainbow.r) * level) / 255u);
            rainbow.g = static_cast<std::uint8_t>((static_cast<unsigned int>(rainbow.g) * level) / 255u);
            rainbow.b = static_cast<std::uint8_t>((static_cast<unsigned int>(rainbow.b) * level) / 255u);
            frame.leds[static_cast<size_t>(i)] = rainbow;
        }
    }
    UpdateSelectedLedFields();
    UpdatePreview();
    UpdateCodeView();
}

void CRgbLedTab::OnShiftLeft() {
    rgb_tool::core::PatternFrame* frame = CurrentFrame();
    if (frame == nullptr || frame->leds.empty()) {
        return;
    }
    std::rotate(frame->leds.begin(), frame->leds.begin() + 1, frame->leds.end());
    UpdateSelectedLedFields();
    UpdatePreview();
    UpdateCodeView();
}

void CRgbLedTab::OnShiftRight() {
    rgb_tool::core::PatternFrame* frame = CurrentFrame();
    if (frame == nullptr || frame->leds.empty()) {
        return;
    }
    std::rotate(frame->leds.rbegin(), frame->leds.rbegin() + 1, frame->leds.rend());
    UpdateSelectedLedFields();
    UpdatePreview();
    UpdateCodeView();
}

void CRgbLedTab::OnBackendSelChanged() {
    UpdateCodeView();
}

void CRgbLedTab::OnHelpSymbol() {
    MessageBoxW(
        L"C Symbol is used as the base name for generated C data and functions.\n\n"
        L"Example: g_rgb_pattern produces g_rgb_pattern_play_once(), "
        L"g_rgb_pattern_step(), and related frame tables.",
        L"C Symbol",
        MB_ICONINFORMATION | MB_OK
    );
}

void CRgbLedTab::OnHelpOutput() {
    MessageBoxW(
        L"Output selects the generated MCU driver backend.\n\n"
        L"LLSI uses LLSI_PDMA_Transfer().\n"
        L"SPI sends expanded WS2812-style bytes through a SPI write hook.\n"
        L"SPI + PDMA builds an expanded SPI buffer and calls a transfer hook.\n"
        L"GPIO generates bit-bang timing code in the driver.",
        L"Output",
        MB_ICONINFORMATION | MB_OK
    );
}

void CRgbLedTab::OnHelpPath() {
    MessageBoxW(
        L"Start is the first physical LED index for the pattern path.\n"
        L"Dir chooses whether the path walks toward higher or lower LED indexes.\n"
        L"Changing Start or Dir remaps the current frames so preview and generated code stay aligned.\n"
        L"Motion controls endpoint behavior when Load Preset creates frames: Loop wraps around; Bounce reverses at LED 0 and the last LED.\n\n"
        L"Example: 24 LEDs, Start 12, Reverse, Bounce produces a moving-dot path like "
        L"12, 11, ... 0, 1, ... 23, 22, ...",
        L"Path Controls",
        MB_ICONINFORMATION | MB_OK
    );
}

void CRgbLedTab::OnPlay() {
    if (playing_ || pattern_.frames.empty()) {
        return;
    }
    playing_ = true;
    UpdatePlayState();
    SetTimer(kPreviewTimerId, PreviewIntervalMs(), nullptr);
}

void CRgbLedTab::OnPreviousFrame() {
    if (pattern_.frames.empty()) {
        return;
    }
    current_frame_ = (current_frame_ == 0) ? static_cast<int>(pattern_.frames.size()) - 1 : current_frame_ - 1;
    frame_combo_.SetCurSel(current_frame_);
    UpdateSelectedFrameFields();
    UpdateSelectedLedFields();
    UpdatePreview();
    UpdatePlaybackControls();
}

void CRgbLedTab::OnNextFrame() {
    if (pattern_.frames.empty()) {
        return;
    }
    current_frame_ = (current_frame_ + 1) % static_cast<int>(pattern_.frames.size());
    frame_combo_.SetCurSel(current_frame_);
    UpdateSelectedFrameFields();
    UpdateSelectedLedFields();
    UpdatePreview();
    UpdatePlaybackControls();
}

void CRgbLedTab::OnStop() {
    if (!playing_) {
        return;
    }
    KillTimer(kPreviewTimerId);
    playing_ = false;
    UpdatePlayState();
}

void CRgbLedTab::OnResetPattern() {
    const rgb_tool::core::AppState defaults = rgb_tool::core::AppState::Default();
    KillTimer(kPreviewTimerId);
    playing_ = false;
    if (::IsWindow(led_combo_.GetSafeHwnd())) led_combo_.SetCurSel(0);
    LoadState(defaults);
    if (::IsWindow(led_combo_.GetSafeHwnd())) led_combo_.SetCurSel(0);
    UpdateSelectedLedFields();
    playback_speed_combo_.SetCurSel(2);
    UpdatePlayState();
    UpdatePlaybackControls();
    if (log_) log_(L"Pattern / Code settings reset to defaults.");
}
