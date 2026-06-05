#include "precompiled_header.h"

#include "render_resources.h"

void RenderResources::Init(HWND hwnd, UINT swapchainWidth, UINT swapchainHeight, bool hdr)
{
    m_HWND = hwnd;

    DXGI_SWAP_CHAIN_DESC swapchainDesc = {
        .BufferDesc =
            {
                .Width = swapchainWidth,
                .Height = swapchainHeight,
                .RefreshRate = {.Numerator = 60, .Denominator = 1},
                .Format = hdr ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM,
            },
        .SampleDesc = {.Count = 1, .Quality = 0},
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = 2,
        .OutputWindow = m_HWND,
        .Windowed = TRUE,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = 0,
    };

    UINT createDeviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ComPtr<IDXGISwapChain> swapchain;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, NULL, 0, D3D11_SDK_VERSION,
        &swapchainDesc, swapchain.ReleaseAndGetAddressOf(), m_Device.ReleaseAndGetAddressOf(), NULL,
        m_DeviceContext.ReleaseAndGetAddressOf());
    CHECK_HR(hr, m_HWND, L"D3D11CreateDeviceAndSwapChain call failed");

    hr = swapchain.As(&m_Swapchain3);
    CHECK_HR(hr, m_HWND, L"Failed to query IDXGISwapchain3");

    hr = m_Swapchain3->SetColorSpace1(hdr ? DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709
                                          : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
    CHECK_HR(hr, m_HWND, L"IDXGISwapChain3::SetColorSpace1 call failed");
    UpdateBackbuffer(hdr);
}

void RenderResources::ResizeSwapchain(UINT width, UINT height, bool hdr)
{
    m_SwapchainBackbufferRTV.Reset();
    m_SwapchainBackbuffer.Reset();

    DXGI_MODE_DESC modeDesc = {
        .Width = width,
        .Height = height,
        .RefreshRate = {},
        .Format = hdr ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM,
        .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
        .Scaling = DXGI_MODE_SCALING_UNSPECIFIED,
    };
    HRESULT hr = m_Swapchain3->ResizeTarget(&modeDesc);
    CHECK_HR(hr, m_HWND, L"IDXGISwapChain::ResizeTarget call failed");

    hr = m_Swapchain3->ResizeBuffers(2, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT, 0);
    CHECK_HR(hr, m_HWND, L"IDXGISwapChain::ResizeBuffers call failed");

    hr = m_Swapchain3->SetColorSpace1(hdr ? DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709
                                          : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);

    CHECK_HR(hr, m_HWND, L"IDXGISwapChain3::SetColorSpace1 call failed");

    UpdateBackbuffer(hdr);
}

ID3D11Device* RenderResources::GetDevice()
{
    return m_Device.Get();
}

ID3D11DeviceContext* RenderResources::GetDeviceContext()
{
    return m_DeviceContext.Get();
}

IDXGISwapChain3* RenderResources::GetSwapchain()
{
    return m_Swapchain3.Get();
}

ID3D11Texture2D* RenderResources::GetSwapchainBackbuffer()
{
    return m_SwapchainBackbuffer.Get();
}

ID3D11RenderTargetView* RenderResources::GetSwapchainBackbufferRTV()
{
    return m_SwapchainBackbufferRTV.Get();
}

void RenderResources::Release()
{
    m_HWND = nullptr;
    m_SwapchainBackbufferRTV.Reset();
    m_SwapchainBackbuffer.Reset();
    m_Swapchain3.Reset();
    m_DeviceContext.Reset();
    m_Device.Reset();
}

void RenderResources::UpdateBackbuffer(bool hdr)
{
    HRESULT hr = m_Swapchain3->GetBuffer(0, IID_ID3D11Texture2D,
                                         (void**)m_SwapchainBackbuffer.ReleaseAndGetAddressOf());
    CHECK_HR(hr, m_HWND, L"IDXGISwapChain::GetBuffer call failed");

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {.Format = hdr ? DXGI_FORMAT_R16G16B16A16_FLOAT
                                                           : DXGI_FORMAT_R8G8B8A8_UNORM,
                                             .ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D,
                                             .Texture2D = {.MipSlice = 0}};

    hr = m_Device->CreateRenderTargetView(m_SwapchainBackbuffer.Get(), &rtvDesc,
                                          m_SwapchainBackbufferRTV.ReleaseAndGetAddressOf());
    CHECK_HR(hr, m_HWND, L"ID3D11Device::CreateRenderTargetView call failed");
}
