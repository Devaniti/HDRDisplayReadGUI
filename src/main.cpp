#include "precompiled_header.h"

#include "gui_window.h"

void InitializeWin32()
{
    HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    CHECK_HR(hr, NULL, L"CoInitializeEx call failed");
}

void CleanupWin32()
{
    ::CoUninitialize();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    InitializeWin32();
    {
        GUIWindow window;
        window.Init(L"HDRMeasurementGUIGUIClass", L"HDR Measurement GUI", false, WS_EX_TOPMOST, 500,
                    400);
        window.Show(true);
        window.MessageLoop();
    }
    CleanupWin32();
}
