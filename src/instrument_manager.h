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
