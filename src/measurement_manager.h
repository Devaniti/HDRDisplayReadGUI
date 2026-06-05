#pragma once

#include "instrument_manager.h"
#include "shaders/cpp_shared.hlsli"

struct MeasurementPatch
{
    float WindowSize;
    uint16_t HDR10R;
    uint16_t HDR10G;
    uint16_t HDR10B;
};

constexpr MeasurementPatch DefaultMeasurementPatch = {
    .WindowSize = 0.01f, .HDR10R = 520, .HDR10G = 520, .HDR10B = 520};

class MeasurementManager
{
public:
    void Initialize(std::vector<MeasurementPatch>&& patchesToMeasure);
    void RestartAllMeasurements();
    void RestartLastMeasurement();

    void Release();
    void Update();
    bool IsRunning();
    bool IsError();
    std::string GetStatusString();
    const std::vector<MeasurementPatch>& GetPatches();
    const std::vector<XYZValue>& GetMeasurements();
    bool NeedDisplayNextPatch();
    MeasurementPatch GetPatchToDisplay();
    void SignalPresent();

private:
    static constexpr int ms_MaxRetryCount = 3;

    std::vector<MeasurementPatch> m_Patches;
    std::vector<XYZValue> m_Measurements;
    bool m_IsRunning = false;
    bool m_IsMeasuring = false;
    bool m_NeedDisplayNextPatch = false;
    bool m_IsError = false;
    int m_RetryCount = 0;
};
