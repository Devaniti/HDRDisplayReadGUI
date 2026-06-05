#pragma once

using Microsoft::WRL::ComPtr;

#ifdef _DEBUG
#define HDRGUI_ASSERT(isSuccess) \
    if (!(isSuccess))            \
        __debugbreak();
#else
#define HDRGUI_ASSERT(isSuccess)
#endif

#define CHECK_RESULT(isSuccess, hwnd, errorMessage)                                         \
    if (!(isSuccess))                                                                       \
    {                                                                                       \
        HDRGUI_ASSERT(0);                                                                   \
        ::MessageBoxW(hwnd, errorMessage, L"Error", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL); \
        ::ExitProcess(1);                                                                   \
    }

#define CHECK_HR(hr, hwnd, errorMessage) CHECK_RESULT(SUCCEEDED(hr), hwnd, errorMessage)

#define WM_USER_GUI_WINDOW_PICK_SPOTREAD_PATH (WM_USER + 0x0001)
#define WM_USER_GUI_WINDOW_OPEN_ARGYLLCMS_DOWNLOAD_PAGE (WM_USER + 0x0002)
#define WM_USER_GUI_WINDOW_SHOW_OPEN_SOURCE_LICENSES (WM_USER + 0x0003)
#define WM_USER_GUI_WINDOW_PICK_PATCHES_FILE (WM_USER + 0x0004)
#define WM_USER_GUI_WINDOW_SAVE_RESULT (WM_USER + 0x0005)
#define WM_USER_GUI_WINDOW_MOVE_WINDOW_TO_NEW_DISPLAY (WM_USER + 0x0006)
#define WM_USER_GUI_WINDOW_OPEN_DISPLAY_SETTINGS (WM_USER + 0x0007)
#define WM_USER_GUI_WINDOW_NEW_PATCH_PRESENTED (WM_USER + 0x0008)
#define WM_USER_SIGNAL_GENERATOR_NEW_PATCH (WM_USER + 0x0009)
#define WM_USER_SIGNAL_GENERATOR_CLOSE (WM_USER + 0x0010)

int intPow(int value, int power);

std::string wStringToString(const std::wstring& in);
std::wstring stringToWString(const std::string& in);
