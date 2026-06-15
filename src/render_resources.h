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
