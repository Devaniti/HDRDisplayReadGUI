#pragma once

#include <subprocess.h>

#include "shaders/cpp_shared.hlsli"

struct XYZValue
{
    float X;
    float Y;
    float Z;
};

enum class InstrumentStatus
{
    NotRunning,
    StartingUp,
    NeedsUserInput,
    ReadyForMeasurement,
    Measuring,
    Error
};

class InstrumentManager
{
public:
    static InstrumentManager& GetInstance();

    void SetSpotreadPath(std::filesystem::path&& path);
    void Terminate();

    void StartSpotread();
    void Update();
    InstrumentStatus GetStatus();

    const std::string& GetUserMessage();
    void PushUserInput();

    void Measure();
    bool IsLastMeasurementFailed();
    void ClearMeasurementFailFlag();
    XYZValue GetLastMeasurement();

private:
    void ProcessBuffer();
    std::filesystem::path GetLogFilePath();

    static const std::chrono::milliseconds ms_MeasurementDelay;

    std::filesystem::path m_SpotreadPath;

    bool m_IsStarted = false;
    Subprocess m_SpotreadProcess;
    std::ofstream m_LogFile;
    std::string m_OutputBuffer;

    InstrumentStatus m_Status = InstrumentStatus::NotRunning;

    std::string m_UserMessage;
    bool m_IsReadyForMeasurement = false;
    XYZValue m_LastMeasurement;
    bool m_IsLastMeasurementFailed = false;
    bool m_OutputUpdatedLastIteration = false;
};
