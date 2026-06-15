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

#pragma once

class BaseWindow
{
public:
    BaseWindow() = default;
    ~BaseWindow() = default;

    void Init(const wchar_t* className, const wchar_t* windowTitle, bool borderless,
              DWORD extendedStyle, UINT clientWidth, UINT clientHeight);
    void Show(bool activate);

    void MessageLoop();
    HWND GetHWND();

protected:
    virtual void InitDerived(HWND hwnd, UINT clientWidth, UINT clientHeight, UINT windowWidth,
                             UINT windowHeight) = 0;
    virtual void ReleaseDerived() = 0;

    virtual LRESULT WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) = 0;
    static LRESULT WindowProcStatic(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    std::pair<UINT, UINT> CalculateWindowSize(DWORD windowStyle, UINT clientWidth,
                                              UINT clientHeight);
    DWORD CalculateWindowStyle(bool borderless);

    HWND m_HWND = NULL;
    const wchar_t* m_ClassName = nullptr;
};
