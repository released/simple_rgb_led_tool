#pragma once

#include <afxcmn.h>
#include <afxwin.h>

#include <functional>
#include <string>

#include "../core/app_state.h"
#include "../core/rgb_pattern.h"

class CRgbPreviewWnd : public CWnd {
public:
    BOOL Create(CWnd* parent, const RECT& rect, UINT id);
    void SetPattern(const rgb_tool::core::RgbPattern* pattern, int frame_index);

protected:
    afx_msg void OnPaint();
    DECLARE_MESSAGE_MAP()

private:
    const rgb_tool::core::RgbPattern* pattern_ = nullptr;
    int frame_index_ = 0;
};

class CRgbLedTab : public CWnd {
public:
    BOOL Create(CWnd* parent, const RECT& rect, UINT id);

    void Bind(std::function<void(const std::wstring&)> logger);
    void LoadState(const rgb_tool::core::AppState& state);
    void SaveState(rgb_tool::core::AppState* state) const;

    bool LoadPatternFromFile(const std::wstring& path, std::wstring* error);
    bool SavePatternToFile(const std::wstring& path, std::wstring* error) const;
    bool SaveGeneratedCodeToFolder(const std::wstring& directory, std::wstring* error) const;
    std::wstring CurrentCode() const;
    bool CopyCodeToClipboard() const;

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnApplyLayout();
    afx_msg void OnLoadPreset();
    afx_msg void OnChoosePatternColor();
    afx_msg void OnChooseSecondColor();
    afx_msg void OnPathOptionChanged();
    afx_msg void OnFrameSelChanged();
    afx_msg void OnAddFrame();
    afx_msg void OnDuplicateFrame();
    afx_msg void OnDeleteFrame();
    afx_msg void OnDurationChanged();
    afx_msg void OnLedSelChanged();
    afx_msg void OnApplyLed();
    afx_msg void OnFillAll();
    afx_msg void OnClearFrame();
    afx_msg void OnGradient();
    afx_msg void OnShiftLeft();
    afx_msg void OnShiftRight();
    afx_msg void OnBackendSelChanged();
    afx_msg void OnHelpSymbol();
    afx_msg void OnHelpOutput();
    afx_msg void OnHelpPath();
    afx_msg void OnPlay();
    afx_msg void OnStop();
    afx_msg void OnResetPattern();
    afx_msg void OnCopyGeneratedCode();
    afx_msg void OnSaveGeneratedCode();
    afx_msg void OnPreviousFrame();
    afx_msg void OnNextFrame();
    DECLARE_MESSAGE_MAP()

private:
    void CreateControls();
    void LayoutControls(int cx, int cy);
    void SetPattern(rgb_tool::core::RgbPattern pattern, int frame_index);
    void UpdateAllViews();
    void UpdateCodeView();
    void UpdatePreview();
    void UpdateFrameCombo();
    void UpdateLedCombo();
    void UpdateSelectedFrameFields();
    void UpdateSelectedLedFields();
    void UpdatePlayState();
    void UpdatePlaybackControls();
    void UpdateAppliedStatus();
    unsigned int PreviewIntervalMs() const;
    void ApplyCurrentLedCountAndShape();
    int SelectedFrameIndex() const;
    int SelectedLedIndex() const;
    int ParseEditInt(const CEdit& edit, int fallback) const;
    std::wstring GetControlText(const CWnd& wnd) const;
    void SetControlText(CWnd& wnd, const std::wstring& text);
    rgb_tool::core::LedShape SelectedShape() const;
    int SelectedStartIndex() const;
    rgb_tool::core::LedDirection SelectedDirection() const;
    rgb_tool::core::LedPathMode SelectedPathMode() const;
    rgb_tool::core::LedOutputBackend SelectedBackend() const;
    rgb_tool::core::RgbColor ParseColorEdits() const;
    rgb_tool::core::RgbColor ParseSecondColorEdits() const;
    rgb_tool::core::PatternFrame* CurrentFrame();
    const rgb_tool::core::PatternFrame* CurrentFrame() const;
    static std::wstring AnsiToWide(const char* text);

private:
    std::function<void(const std::wstring&)> log_;
    rgb_tool::core::RgbPattern pattern_;
    int current_frame_ = 0;
    bool playing_ = false;
    bool updating_ui_ = false;

    CFont ui_font_;
    CFont small_font_;

    CButton group_;
    CButton code_group_;
    CStatic led_count_title_;
    CEdit led_count_edit_;
    CStatic shape_title_;
    CComboBox shape_combo_;
    CButton apply_layout_btn_;
    CStatic current_layout_value_;

    CStatic preset_title_;
    CComboBox preset_combo_;
    CButton load_preset_btn_;
    CStatic current_preset_value_;
    CStatic symbol_title_;
    CEdit symbol_edit_;
    CButton symbol_help_btn_;
    CStatic backend_title_;
    CComboBox backend_combo_;
    CButton backend_help_btn_;
    CButton copy_code_btn_;
    CButton save_code_btn_;
    CStatic start_title_;
    CEdit start_edit_;
    CStatic direction_title_;
    CComboBox direction_combo_;
    CStatic path_mode_title_;
    CComboBox path_mode_combo_;
    CButton path_help_btn_;

    CStatic frame_title_;
    CComboBox frame_combo_;
    CButton add_frame_btn_;
    CButton duplicate_frame_btn_;
    CButton delete_frame_btn_;
    CStatic duration_title_;
    CEdit duration_edit_;
    CButton play_btn_;
    CButton stop_btn_;
    CButton reset_pattern_btn_;
    CButton previous_frame_btn_;
    CButton next_frame_btn_;
    CStatic playback_status_;
    CSliderCtrl playback_slider_;
    CStatic playback_speed_title_;
    CComboBox playback_speed_combo_;

    CStatic led_title_;
    CComboBox led_combo_;
    CStatic red_title_;
    CEdit red_edit_;
    CStatic green_title_;
    CEdit green_edit_;
    CStatic blue_title_;
    CEdit blue_edit_;
    CButton choose_color_btn_;
    CStatic color2_title_;
    CEdit color2_red_edit_;
    CEdit color2_green_edit_;
    CEdit color2_blue_edit_;
    CButton choose_color2_btn_;
    CStatic tail_title_;
    CEdit tail_edit_;
    CStatic fade_title_;
    CEdit fade_edit_;
    CStatic spacing_title_;
    CEdit spacing_edit_;
    CStatic repeat_title_;
    CEdit repeat_edit_;
    CButton apply_led_btn_;
    CButton fill_all_btn_;
    CButton clear_frame_btn_;
    CButton gradient_btn_;
    CButton shift_left_btn_;
    CButton shift_right_btn_;

    CStatic preview_title_;
    CRgbPreviewWnd preview_;
    CStatic code_title_;
    CEdit code_edit_;
};
