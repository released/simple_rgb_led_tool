#pragma once

#include <afxcmn.h>
#include <afxwin.h>

#include <string>

#include "config/ini_manager.h"
#include "core/app_state.h"
#include "log/logger.h"
#include "ui/rgb_led_tab.h"
#include "ui/control_tab.h"

class CMainFrame : public CFrameWnd {
public:
    CMainFrame();

protected:
    BOOL PreCreateWindow(CREATESTRUCT& cs) override;
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnBnClickedSaveIni();
    afx_msg void OnBnClickedLoadIni();
    afx_msg void OnBnClickedResetIni();
    afx_msg void OnBnClickedLoadPattern();
    afx_msg void OnBnClickedSavePattern();
    afx_msg void OnBnClickedCopyCode();
    afx_msg void OnBnClickedSaveCode();
    afx_msg void OnBnClickedSaveLog();
    afx_msg void OnBnClickedSaveLogCheck();
    afx_msg void OnBnClickedClearLog();
    afx_msg void OnClose();
    afx_msg void OnTabChanged(NMHDR* header, LRESULT* result);

    DECLARE_MESSAGE_MAP()

private:
    void LayoutControls(int cx, int cy);
    void UpdateTabVisibility();
    void AppendLog(const std::wstring& text);
    void TrimVisibleLogIfNeeded();
    void UpdateIniPathUi();
    void LoadIni();
    void SaveIni();
    void SaveIniTo(const std::wstring& path);
    bool TryLoadIniFrom(const std::wstring& path);
    void ResetIniToDefault();
    void SaveLogToFile(const std::wstring& path);
    std::wstring GetControlText(const CWnd& w) const;
    void SetControlText(CWnd& w, const std::wstring& text);
    static void ShowErrorBox(const std::wstring& title, const std::wstring& message);

private:
    CFont ui_font_;

    CButton save_ini_btn_;
    CButton load_ini_btn_;
    CButton reset_ini_btn_;
    CButton load_pattern_btn_;
    CButton save_pattern_btn_;
    CButton copy_code_btn_;
    CButton save_code_btn_;
    CStatic ini_path_title_;
    CStatic ini_path_value_;
    CStatic build_info_title_;
    CStatic build_info_value_;

    CTabCtrl tab_ctrl_;
    CControlTab control_tab_;
    CRgbLedTab rgb_tab_;
    CButton save_log_check_;
    CButton save_log_btn_;
    CButton clear_log_btn_;
    CEdit log_edit_;

    mfc_tool::config::IniManager ini_;
    rgb_tool::core::AppState state_;
    mfc_tool::log::Logger logger_;
};
