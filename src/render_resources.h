#pragma once

class RenderResources
{
public:
    RenderResources() = default;
    ~RenderResources() = default;

    void Init(HWND hwnd, UINT swapchainWidth, UINT swapchainHeight, bool hdr);
    void ResizeSwapchain(UINT width, UINT height, bool hdr);

    ID3D11Device* GetDevice();
    ID3D11DeviceContext* GetDeviceContext();
    IDXGISwapChain3* GetSwapchain();
    ID3D11Texture2D* GetSwapchainBackbuffer();
    ID3D11RenderTargetView* GetSwapchainBackbufferRTV();

    void Release();

private:
    void UpdateBackbuffer(bool hdr);

    HWND m_HWND;
    ComPtr<ID3D11Device> m_Device;
    ComPtr<ID3D11DeviceContext> m_DeviceContext;
    ComPtr<IDXGISwapChain3> m_Swapchain3;
    ComPtr<ID3D11Texture2D> m_SwapchainBackbuffer;
    ComPtr<ID3D11RenderTargetView> m_SwapchainBackbufferRTV;
};
