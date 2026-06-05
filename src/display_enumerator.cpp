#include "precompiled_header.h"

#include "display_enumerator.h"

DisplayEnumerator::DisplayEnumerator()
{
    EnumerateDisplaysImpl();
}

DisplayEnumerator& DisplayEnumerator::GetInstance()
{
    static DisplayEnumerator instance;
    return instance;
}

void DisplayEnumerator::Reenumerate()
{
    m_Displays.clear();
    EnumerateDisplaysImpl();
}

int DisplayEnumerator::GetDisplayCount()
{
    return m_Displays.size();
}

const DisplayDesc& DisplayEnumerator::GetDisplay(int index)
{
    return m_Displays[index];
}

void DisplayEnumerator::EnumerateDisplaysImpl()
{
    bool isHDREnabled = false;

    ComPtr<IDXGIFactory1> dxgiFactory;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
    CHECK_HR(hr, NULL, L"CreateDXGIFactory1 call failed");

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT adapterIndex = 0; SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIndex, &adapter));
         ++adapterIndex)
    {
        ComPtr<IDXGIOutput> output;
        for (UINT outputIndex = 0; SUCCEEDED(adapter->EnumOutputs(outputIndex, &output));
             ++outputIndex)
        {
            ComPtr<IDXGIOutput6> output6;

            hr = output.As(&output6);
            CHECK_HR(hr, NULL, L"Failed to query IDXGIOutput6");

            DXGI_OUTPUT_DESC1 desc1;
            hr = output6->GetDesc1(&desc1);
            CHECK_HR(hr, NULL, L"IDXGIOutput6::GetDesc1 call failed");

            m_Displays.push_back(
                {.DesktopCoordinates = desc1.DesktopCoordinates,
                 .IsHDR = desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020});
        }
    }
}
