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
        window.Init(L"HDRDisplayReadGUIGUIClass", L"HDR Display Read GUI", false, WS_EX_TOPMOST, 500,
                    400);
        window.Show(true);
        window.MessageLoop();
    }
    CleanupWin32();
}
