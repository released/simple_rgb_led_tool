#include "app.h"

#include "main_frame.h"

BEGIN_MESSAGE_MAP(CRgbLedToolApp, CWinApp)
END_MESSAGE_MAP()

BOOL CRgbLedToolApp::InitInstance() {
    CWinApp::InitInstance();

    auto* frame = new CMainFrame();
    if (!frame->Create(nullptr, L"Simple RGB LED Tool", WS_OVERLAPPEDWINDOW, CRect(0, 0, 1360, 820))) {
        delete frame;
        return FALSE;
    }

    m_pMainWnd = frame;
    frame->ShowWindow(SW_SHOWMAXIMIZED);
    frame->UpdateWindow();
    return TRUE;
}
