#pragma once

struct DisplayDesc
{
    RECT DesktopCoordinates;
    bool IsHDR;
};

class DisplayEnumerator
{
    DisplayEnumerator();
    ~DisplayEnumerator() = default;

public:
    static DisplayEnumerator& GetInstance();

    void Reenumerate();
    int GetDisplayCount();
    const DisplayDesc& GetDisplay(int index);

private:
    void EnumerateDisplaysImpl();

    std::vector<DisplayDesc> m_Displays;
};
