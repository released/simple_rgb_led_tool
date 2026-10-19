#pragma once

#include <afxcmn.h>
#include <afxwin.h>
#include <functional>
#include <string>

#include "../core/app_state.h"
#include "../services/hid_device.h"

class CControlTab : public CWnd {
public:
    BOOL Create(CWnd* parent, const RECT& rect, UINT id);
    void Bind(std::function<void(const std::wstring&)> logger);
    void LoadState(const rgb_tool::core::AppState& state);
    void SaveState(rgb_tool::core::AppState* state) const;
    void Disconnect();

protected:
    afx_msg int OnCreate(LPCREATESTRUCT create_struct);
    afx_msg void OnSize(UINT type, int cx, int cy);
    afx_msg void OnConnect();
    afx_msg void OnDisconnect();
    afx_msg void OnSendConfig();
    afx_msg void OnApplyEffect();
    afx_msg void OnStop();
    afx_msg void OnEffectChanged();
    afx_msg void OnChooseColor();
    afx_msg void OnResetValues();
    afx_msg void OnNumericEditCommit();
    afx_msg void OnHScroll(UINT code, UINT position, CScrollBar* scroll_bar);
    DECLARE_MESSAGE_MAP()

private:
    void LayoutControls(int cx, int cy);
    int ReadInt(const CEdit& edit, int fallback, int minimum, int maximum) const;
    int NormalizeEdit(CEdit& edit, CSliderCtrl& slider, int fallback, int minimum, int maximum);
    void SetEditValue(CEdit& edit, int value);
    void SetColorValues(int red, int green, int blue);
    void UpdateColorButtonText();
    void UpdateEffectUi();
    void UpdateConnectionUi();
    bool Send(rgb_tool::hid::Command command, const std::uint8_t* payload, std::uint8_t length, const wchar_t* label);

    std::function<void(const std::wstring&)> log_;
    rgb_tool::hid::HidDevice device_;
    CFont font_;
    CButton connection_group_;
    CStatic status_title_;
    CStatic status_value_;
    CButton connect_btn_;
    CButton disconnect_btn_;
    CButton config_group_;
    CStatic led_count_title_;
    CEdit led_count_edit_;
    CStatic order_title_;
    CComboBox order_combo_;
    CStatic brightness_title_;
    CEdit brightness_edit_;
    CButton send_config_btn_;
    CButton effect_group_;
    CStatic effect_title_;
    CComboBox effect_combo_;
    CStatic effect_help_;
    CButton choose_color_btn_;
    CButton reset_values_btn_;
    CStatic red_title_;
    CEdit red_edit_;
    CSliderCtrl red_slider_;
    CStatic green_title_;
    CEdit green_edit_;
    CSliderCtrl green_slider_;
    CStatic blue_title_;
    CEdit blue_edit_;
    CSliderCtrl blue_slider_;
    CStatic speed_title_;
    CEdit speed_edit_;
    CSliderCtrl speed_slider_;
    CStatic direction_title_;
    CComboBox direction_combo_;
    CButton apply_btn_;
    CButton stop_btn_;
    bool updating_ui_ = false;
};
