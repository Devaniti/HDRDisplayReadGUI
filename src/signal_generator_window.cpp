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

#include "signal_generator_window.h"

#include "embeds.h"

void SignalGeneratorWindow::OpenAsync(HWND parentHWND, UINT posX, UINT posY, UINT width,
                                      UINT height)
{
    m_ParentHWND = parentHWND;
    m_PosX = posX;
    m_PosY = posY;
    m_Width = width;
    m_Height = height;
    m_WindowThread = std::thread(&SignalGeneratorWindow::ThreadEntrypoint, this);
}

void SignalGeneratorWindow::Stop()
{
    if (GetHWND() == NULL)
    {
        return;
    }

    ::PostMessage(GetHWND(), WM_USER_SIGNAL_GENERATOR_CLOSE, 0, 0);
    m_WindowThread.join();

    m_Patch = DefaultMeasurementPatch;
    m_WindowSize = 0.0f;
    m_SpotColor = {};
}

void SignalGeneratorWindow::DisplayPatch(const MeasurementPatch& patch)
{
    m_Patch = patch;
    ::PostMessage(GetHWND(), WM_USER_SIGNAL_GENERATOR_NEW_PATCH, 0, 0);
}

void SignalGeneratorWindow::InitDerived(HWND hwnd, UINT clientWidth, UINT clientHeight,
                                        UINT windowWidth, UINT windowHeight)
{
    HDRGUI_ASSERT(clientWidth == windowWidth);
    HDRGUI_ASSERT(clientWidth == m_Width);
    HDRGUI_ASSERT(clientHeight == windowHeight);
    HDRGUI_ASSERT(clientHeight == m_Height);

    ::MoveWindow(hwnd, m_PosX, m_PosY, m_Width, m_Height, FALSE);

    InitRenderResources();
}

void SignalGeneratorWindow::ReleaseDerived()
{
    m_RenderResources.Release();
}

LRESULT SignalGeneratorWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static bool isPainting = false;
    // Ensure that we don't have recursive message handling during painting
    HDRGUI_ASSERT(!isPainting);

    switch (message)
    {
    case WM_MOVE: {
        ::MoveWindow(hwnd, m_PosX, m_PosY, m_Width, m_Height, FALSE);
        return 0;
    }
    case WM_ACTIVATE: {
        ::SetForegroundWindow(m_ParentHWND);
        return 0;
    }
    case WM_MOUSEACTIVATE: {
        return MA_NOACTIVATEANDEAT;
    }
    case WM_PAINT: {
        isPainting = true;
        Render();
        ::ValidateRect(hwnd, NULL);
        isPainting = false;
        return 0;
    }
    case WM_CLOSE: {
        return 0;
    }
    case WM_GETMINMAXINFO: {
        LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
        lpMMI->ptMinTrackSize.x = m_Width;
        lpMMI->ptMinTrackSize.y = m_Height;
        lpMMI->ptMaxTrackSize.x = m_Width;
        lpMMI->ptMaxTrackSize.y = m_Height;
        return 0;
    }
    case WM_SIZING: {
        RECT* rect = (RECT*)lParam;
        rect->right = rect->left + m_Width;
        rect->bottom = rect->top + m_Height;
        return TRUE;
    }
    case WM_NCDESTROY:
        ::PostQuitMessage(0);
        return 0;
    case WM_USER_SIGNAL_GENERATOR_NEW_PATCH: {
        ConvertPatch();
        Render();
        ::PostMessage(m_ParentHWND, WM_USER_GUI_WINDOW_NEW_PATCH_PRESENTED, 0, 0);
        return 0;
    }
    case WM_USER_SIGNAL_GENERATOR_CLOSE: {
        ::DestroyWindow(hwnd);
        return 0;
    }
    default:
        return ::DefWindowProcW(hwnd, message, wParam, lParam);
    }

    // All cases should return a value, so that we never reach this point
    HDRGUI_ASSERT(0);
}

void SignalGeneratorWindow::ThreadEntrypoint()
{
    Init(L"HDRDisplayReadGUISignalGeneratorClass", L"HDR Display Read GUI Signal Generator", true,
         WS_EX_TOPMOST | WS_EX_NOACTIVATE, m_Width, m_Height);
    Show(false);
    ::BringWindowToTop(m_ParentHWND);
    MessageLoop();
}

void SignalGeneratorWindow::InitRenderResources()
{
    m_RenderResources.Init(GetHWND(), m_Width, m_Height, true);

    ID3D11Device* device = m_RenderResources.GetDevice();

    D3D11_RASTERIZER_DESC rasterizerDesc = {
        .FillMode = D3D11_FILL_SOLID,
        .CullMode = D3D11_CULL_NONE,
        .FrontCounterClockwise = FALSE,
        .DepthBias = 0,
        .DepthBiasClamp = 0.0f,
        .SlopeScaledDepthBias = 0.0f,
        .DepthClipEnable = FALSE,
        .ScissorEnable = FALSE,
        .MultisampleEnable = FALSE,
        .AntialiasedLineEnable = FALSE,
    };

    HRESULT hr =
        device->CreateRasterizerState(&rasterizerDesc, RasterizerState.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        MessageBoxW(GetHWND(), L"Failed to create D3D11 rasterizer state", L"Error",
                    MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        ExitProcess(1);
        return;
    }

    D3D11_BLEND_DESC blendDesc = {
        .AlphaToCoverageEnable = FALSE,
        .IndependentBlendEnable = FALSE,
        .RenderTarget = {{.BlendEnable = FALSE,
                          .SrcBlend = D3D11_BLEND_ONE,
                          .DestBlend = D3D11_BLEND_ZERO,
                          .BlendOp = D3D11_BLEND_OP_ADD,
                          .SrcBlendAlpha = D3D11_BLEND_ONE,
                          .DestBlendAlpha = D3D11_BLEND_ZERO,
                          .BlendOpAlpha = D3D11_BLEND_OP_ADD,
                          .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL}},
    };

    hr = device->CreateBlendState(&blendDesc, BlendState.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        MessageBoxW(GetHWND(), L"Failed to create D3D11 blend state", L"Error",
                    MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        ExitProcess(1);
        return;
    }

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {
        .DepthEnable = FALSE,
        .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO,
        .DepthFunc = D3D11_COMPARISON_ALWAYS,
        .StencilEnable = FALSE,
        .StencilReadMask = 0,
        .StencilWriteMask = 0,
        .FrontFace = {},
        .BackFace = {},
    };

    hr = device->CreateDepthStencilState(&depthStencilDesc,
                                         DepthStencilState.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        MessageBoxW(GetHWND(), L"Failed to create D3D11 depth stencil state", L"Error",
                    MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        ExitProcess(1);
        return;
    }

    MemoryBlock tonemapVSBytecode = GetSignalGeneratorVSBytecode();
    hr = device->CreateVertexShader(tonemapVSBytecode.data, tonemapVSBytecode.size, NULL,
                                    SignalGeneratorVS.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        MessageBoxW(GetHWND(), L"Failed to create D3D11 vertex shader", L"Error",
                    MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        ExitProcess(1);
        return;
    }

    MemoryBlock tonemapPSBytecode = GetSignalGeneratorPSBytecode();
    hr = device->CreatePixelShader(tonemapPSBytecode.data, tonemapPSBytecode.size, NULL,
                                   SignalGeneratorPS.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        MessageBoxW(GetHWND(), L"Failed to create D3D11 pixel shader", L"Error",
                    MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        ExitProcess(1);
        return;
    }

    D3D11_BUFFER_DESC bufferDesc = {
        .ByteWidth = sizeof(ShaderTypes::SignalGeneratorParametersStruct),
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
        .CPUAccessFlags = 0,
        .MiscFlags = 0,
        .StructureByteStride = 0,
    };

    hr = device->CreateBuffer(&bufferDesc, NULL, ConstantBuffer.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        MessageBoxW(GetHWND(), L"Failed to create D3D11 constant buffer", L"Error",
                    MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        ExitProcess(1);
        return;
    }

    ConvertPatch();
    Render();
}

void SignalGeneratorWindow::Render()
{
    UpdateConstantBuffer();

    ID3D11DeviceContext* deviceContext = m_RenderResources.GetDeviceContext();

    ID3D11RenderTargetView* nullRTVToSet[] = {nullptr};
    ID3D11RenderTargetView* swapchainRTVToSet[] = {m_RenderResources.GetSwapchainBackbufferRTV()};

    ID3D11Buffer* nullCBVToSet[] = {nullptr};
    ID3D11Buffer* cbvToSet[] = {ConstantBuffer.Get()};

    deviceContext->RSSetState(RasterizerState.Get());
    deviceContext->OMSetBlendState(BlendState.Get(), NULL, 0xffffffff);
    deviceContext->OMSetDepthStencilState(DepthStencilState.Get(), 0);

    D3D11_VIEWPORT viewportDesc = {
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = static_cast<float>(m_Width),
        .Height = static_cast<float>(m_Height),
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
    };
    deviceContext->RSSetViewports(1, &viewportDesc);

    deviceContext->VSSetShader(SignalGeneratorVS.Get(), NULL, 0);
    deviceContext->PSSetShader(SignalGeneratorPS.Get(), NULL, 0);

    deviceContext->PSSetConstantBuffers(0, 1, cbvToSet);

    deviceContext->OMSetRenderTargets(1, swapchainRTVToSet, NULL);

    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    deviceContext->Draw(6, 0);

    deviceContext->PSSetConstantBuffers(0, 1, nullCBVToSet);
    deviceContext->OMSetRenderTargets(1, nullRTVToSet, NULL);

    HRESULT hr = m_RenderResources.GetSwapchain()->Present(1, 0);
    CHECK_HR(hr, GetHWND(), L"IDXGISwapchain::Present call failed");
}

void SignalGeneratorWindow::UpdateConstantBuffer()
{
    ID3D11DeviceContext* deviceContext = m_RenderResources.GetDeviceContext();

    ShaderTypes::PatchBBoxType bbox = CalculatePatchBBox();

    ShaderTypes::SignalGeneratorParametersStruct conversionParameters = {
        .PatchBBox = bbox,
        .SpotColor = {.r = m_SpotColor.R, .g = m_SpotColor.G, .b = m_SpotColor.B}};

    deviceContext->UpdateSubresource(ConstantBuffer.Get(), 0, NULL, &conversionParameters, 0, 0);
}

ShaderTypes::PatchBBoxType SignalGeneratorWindow::CalculatePatchBBox()
{
    float pixelCount = float(m_Width) * m_Height * m_WindowSize;
    float squareSize = std::sqrt(pixelCount);

    uint32_t spotWidth = 0;
    uint32_t spotHeight = 0;

    if (squareSize < m_Width && squareSize < m_Height)
    {
        uint32_t squareSizeUint = uint32_t(squareSize);
        spotHeight = squareSizeUint;
        spotWidth = uint32_t(pixelCount / spotHeight);
    }
    else
    {
        if (m_Width > m_Height)
        {
            spotHeight = m_Height;
            spotWidth = uint32_t(pixelCount / spotHeight);
        }
        else
        {
            spotWidth = m_Width;
            spotHeight = uint32_t(pixelCount / spotWidth);
        }
    }

    uint32_t left = m_Width / 2 - spotWidth / 2;
    uint32_t right = left + spotWidth;
    uint32_t top = m_Height / 2 - spotHeight / 2;
    uint32_t bottom = top + spotHeight;

    ShaderTypes::PatchBBoxType result = {};
    result.left = left;
    result.right = right;
    result.top = top;
    result.bottom = bottom;

    return result;
}

CCCSColor SignalGeneratorWindow::HDR10ToCCCS(const MeasurementPatch& input)
{
    float rLinearRec2020 = PQEOTF(input.HDR10R);
    float gLinearRec2020 = PQEOTF(input.HDR10G);
    float bLinearRec2020 = PQEOTF(input.HDR10B);

    float rLinearRec709 =
        1.66049159f * rLinearRec2020 - 0.587641180 * gLinearRec2020 - 0.0728499144 * bLinearRec2020;
    float gLinearRec709 = -0.124550551f * rLinearRec2020 + 1.13290012f * gLinearRec2020 -
                          0.00834939629 * bLinearRec2020;
    float bLinearRec709 = -0.0181507692f * rLinearRec2020 - 0.100578845 * gLinearRec2020 +
                          1.11872947 * bLinearRec2020;

    return CCCSColor{
        .R = rLinearRec709 / 80.0f,
        .G = gLinearRec709 / 80.0f,
        .B = bLinearRec709 / 80.0f,
    };
}

float SignalGeneratorWindow::PQEOTF(uint16_t input)
{
    float inputFloat = input / 1023.0f;

    constexpr float m1 = 2610.0f / 16384.0f;
    constexpr float m2 = (2523.0f / 4096.0f) * 128.0f;
    constexpr float c1 = 3424.0f / 4096.0f;
    constexpr float c2 = (2413.0f / 4096.0f) * 32.0f;
    constexpr float c3 = (2392.0f / 4096.0f) * 32.0f;

    float p = std::pow(inputFloat, 1 / m2);
    float numerator = std::max(p - c1, 0.0f);
    float denominator = c2 - c3 * p;

    return std::pow(numerator / denominator, 1 / m1) * 10000.0f;
}

void SignalGeneratorWindow::ConvertPatch()
{
    m_WindowSize = m_Patch.WindowSize;
    m_SpotColor = HDR10ToCCCS(m_Patch);
}
