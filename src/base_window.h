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
