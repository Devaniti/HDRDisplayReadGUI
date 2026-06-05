#include "cpp_shared.hlsli"

#pragma warning(disable : 3571)

Texture2D<float4> ImGUITarget : register(t0);

cbuffer PerObject : register(b0)
{
    SignalGeneratorParametersStruct SignalGeneratorParameters;
};

struct VSInput
{
    uint vertexID : SV_VertexID;
};

struct PSInput
{
    float4 position : SV_Position;
};

PSInput VSMain(VSInput input)
{
    PSInput output = (PSInput)0;

    static const float2 vertexBuffer[6] =
        {
            float2(-1.0f, -1.0f),
            float2(-1.0f, 1.0f),
            float2(1.0f, -1.0f),
            float2(1.0f, -1.0f),
            float2(-1.0f, 1.0f),
            float2(1.0f, 1.0f)};

    output.position = float4(vertexBuffer[input.vertexID], 0.5, 1.0f);

    return output;
}

// Return color in CCCS color space
float4 PSMain(PSInput input) : SV_TARGET
{
    bool isInsideSpot = all(input.position.xy >= float2(SignalGeneratorParameters.PatchBBox.left, SignalGeneratorParameters.PatchBBox.top)) && 
        all(input.position.xy < float2(SignalGeneratorParameters.PatchBBox.right, SignalGeneratorParameters.PatchBBox.bottom));
    
	return isInsideSpot ? float4(SignalGeneratorParameters.SpotColor, 0.0f) : (0.0f).xxxx;
}
