/*
    HDR Display Read GUI
    Copyright (C) 2026  Dmytro Bulatov

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    My contacts are on https://boolka.dev/
*/

#include "precompiled_header.h"

#include "base_window.h"

void BaseWindow::Init(const wchar_t* className, const wchar_t* windowTitle, bool borderless,
                      DWORD extendedStyle, UINT clientWidth, UINT clientHeight)
{
    m_ClassName = className;

    HINSTANCE instance = ::GetModuleHandle(NULL);

    WNDCLASSEXW windowClassDesc = {};
    windowClassDesc.cbSize = sizeof(windowClassDesc);
    windowClassDesc.style = CS_HREDRAW | CS_VREDRAW;
    windowClassDesc.lpfnWndProc = &BaseWindow::WindowProcStatic;
    windowClassDesc.hInstance = instance;
    windowClassDesc.hIcon = ::LoadIcon(instance, MAKEINTRESOURCE(IDI_ICON1));
    windowClassDesc.lpszClassName = m_ClassName;

    ATOM windowClass = RegisterClassExW(&windowClassDesc);

    if (windowClass == 0)
    {
        ::MessageBoxW(NULL, L"Failed to register window class", L"Error",
                      MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        ExitProcess(1);
    }

    DWORD windowStyle = CalculateWindowStyle(borderless);
    auto [windowWidth, windowHeight] = CalculateWindowSize(windowStyle, clientWidth, clientHeight);

    m_HWND = CreateWindowExW(extendedStyle, m_ClassName, windowTitle, windowStyle, CW_USEDEFAULT,
                             CW_USEDEFAULT, windowWidth, windowHeight, NULL, NULL, instance, this);

    if (m_HWND == NULL)
    {
        ::MessageBoxW(m_HWND, L"Failed to create main window", L"Error",
                      MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        ExitProcess(1);
    }

    InitDerived(m_HWND, clientWidth, clientHeight, windowWidth, windowHeight);
}

void BaseWindow::Show(bool activate)
{
    ::ShowWindow(m_HWND, activate ? SW_SHOW : SW_SHOWNA);
    ::UpdateWindow(m_HWND);
}

void BaseWindow::MessageLoop()
{
    MSG msg;
    while (::GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
    ::UnregisterClassW(m_ClassName, ::GetModuleHandle(NULL));
    ReleaseDerived();
}

HWND BaseWindow::GetHWND()
{
    return m_HWND;
}

LRESULT BaseWindow::WindowProcStatic(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // How can I make a WNDPROC or DLGPROC a member of my C++ class?
    // https://devblogs.microsoft.com/oldnewthing/20140203-00/?p=1893
    BaseWindow* thisPtr;
    if (message == WM_NCCREATE)
    {
        LPCREATESTRUCT createStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        thisPtr = static_cast<BaseWindow*>(createStruct->lpCreateParams);
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(thisPtr));
    }
    else
    {
        thisPtr = reinterpret_cast<BaseWindow*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (thisPtr)
    {
        return thisPtr->WindowProc(hwnd, message, wParam, lParam);
    }

    return ::DefWindowProc(hwnd, message, wParam, lParam);
}

std::pair<UINT, UINT> BaseWindow::CalculateWindowSize(DWORD windowStyle, UINT clientWidth,
                                                      UINT clientHeight)
{
    RECT calculatedRect;
    calculatedRect.left = 0;
    calculatedRect.top = 0;
    calculatedRect.right = clientWidth;
    calculatedRect.bottom = clientHeight;

    ::AdjustWindowRect(&calculatedRect, windowStyle, FALSE);

    return {calculatedRect.right - calculatedRect.left, calculatedRect.bottom - calculatedRect.top};
}

DWORD BaseWindow::CalculateWindowStyle(bool borderless)
{
    return borderless ? WS_POPUP : (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU);
}
