#ifndef CPP_SHADER_HLSLI
#define CPP_SHADER_HLSLI

#ifdef __cplusplus
#include <cstdint>

namespace ShaderTypes
{
    using uint = uint32_t;
    struct float3
    {
        float r;
        float g;
        float b;
    };
#endif

struct PatchBBoxType
{
    float left;
    float right;
    float top;
    float bottom;
};

struct SignalGeneratorParametersStruct
{
    PatchBBoxType PatchBBox;
    float3 SpotColor;
    uint pad;
};

#ifdef __cplusplus
    
}
#endif

#endif
