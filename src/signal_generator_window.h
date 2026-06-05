#pragma once

#include "base_window.h"
#include "measurement_manager.h"
#include "render_resources.h"
#include "shaders/cpp_shared.hlsli"

struct CCCSColor
{
    float R;
    float G;
    float B;
};

class SignalGeneratorWindow : private BaseWindow
{
public:
    SignalGeneratorWindow() = default;
    ~SignalGeneratorWindow() = default;

    void OpenAsync(HWND parentHWND, UINT posX, UINT posY, UINT width, UINT height);
    void Stop();
    void DisplayPatch(const MeasurementPatch& patch);

protected:
    void InitDerived(HWND hwnd, UINT clientWidth, UINT clientHeight, UINT windowWidth,
                     UINT windowHeight) final;
    void ReleaseDerived() final;
    LRESULT WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) final;

private:
    void ThreadEntrypoint();
    void InitRenderResources();
    void Render();
    void UpdateConstantBuffer();
    ShaderTypes::PatchBBoxType CalculatePatchBBox();
    static CCCSColor HDR10ToCCCS(const MeasurementPatch& input);
    static float PQEOTF(uint16_t input);
    void ConvertPatch();

    HWND m_ParentHWND;
    UINT m_PosX;
    UINT m_PosY;
    UINT m_Width;
    UINT m_Height;
    std::thread m_WindowThread;

    MeasurementPatch m_Patch = DefaultMeasurementPatch;
    float m_WindowSize = 0.0f;
    CCCSColor m_SpotColor = {};

    RenderResources m_RenderResources;
    ComPtr<ID3D11RasterizerState> RasterizerState;
    ComPtr<ID3D11BlendState> BlendState;
    ComPtr<ID3D11DepthStencilState> DepthStencilState;
    ComPtr<ID3D11Buffer> ConstantBuffer;
    ComPtr<ID3D11VertexShader> SignalGeneratorVS;
    ComPtr<ID3D11PixelShader> SignalGeneratorPS;
};
