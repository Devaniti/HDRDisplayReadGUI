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

#include "instrument_manager.h"

const std::chrono::milliseconds InstrumentManager::ms_MeasurementDelay =
    std::chrono::milliseconds(200);

InstrumentManager& InstrumentManager::GetInstance()
{
    static InstrumentManager instance;
    return instance;
}

void InstrumentManager::SetSpotreadPath(std::filesystem::path&& path)
{
    m_SpotreadPath = std::move(path);
}

void InstrumentManager::Terminate()
{
    if (!m_IsStarted)
    {
        return;
    }

    m_SpotreadProcess.Terminate();
    m_SpotreadProcess.Release();
    m_OutputBuffer.clear();
    m_Status = InstrumentStatus::NotRunning;
    m_UserMessage.clear();
    m_IsReadyForMeasurement = false;
    m_LastMeasurement = {};
    m_IsLastMeasurementFailed = false;
    m_OutputUpdatedLastIteration = false;
    m_IsStarted = false;
}

void InstrumentManager::StartSpotread()
{
    if (m_IsStarted)
    {
        return;
    }

    std::wstring commandline = m_SpotreadPath.wstring() + L" -e -H";
    m_SpotreadProcess.Start(commandline, {{"ARGYLL_NOT_INTERACTIVE", "1"}});
    m_Status = InstrumentStatus::StartingUp;
    m_IsReadyForMeasurement = false;
    m_LogFile.open(GetLogFilePath());
    m_IsLastMeasurementFailed = false;
    m_OutputUpdatedLastIteration = false;
    m_IsStarted = true;
}

void InstrumentManager::Update()
{
    if (m_Status == InstrumentStatus::NotRunning)
    {
        return;
    }

    bool isOutputBufferUpdated = false;
    while (true)
    {
        char buffer[512];
        int readBytes = m_SpotreadProcess.Read(buffer, sizeof(buffer));
        if (readBytes > 0)
        {
            m_OutputBuffer.insert(m_OutputBuffer.size(), buffer, (size_t)readBytes);
            m_LogFile.write(buffer, readBytes);
            isOutputBufferUpdated = true;
        }
        else
        {
            break;
        }
    }

    if (!m_SpotreadProcess.IsRunning())
    {
        m_Status = InstrumentStatus::NotRunning;
        m_LogFile.flush();
        ProcessBuffer();
        m_OutputUpdatedLastIteration = false;
        return;
    }

    if (!isOutputBufferUpdated && m_OutputUpdatedLastIteration)
    {
        m_LogFile.flush();
        ProcessBuffer();
    }

    m_OutputUpdatedLastIteration = isOutputBufferUpdated;
}

InstrumentStatus InstrumentManager::GetStatus()
{
    return m_Status;
}

const std::string& InstrumentManager::GetUserMessage()
{
    return m_UserMessage;
}

void InstrumentManager::PushUserInput()
{
    if (m_Status == InstrumentStatus::NeedsUserInput)
    {
        m_SpotreadProcess.Write("\n", 1);
        m_Status = InstrumentStatus::StartingUp;
    }

    if (m_Status == InstrumentStatus::ReadyForMeasurement)
    {
        m_SpotreadProcess.Write("\n", 1);
        m_Status = InstrumentStatus::Measuring;
    }
}

void InstrumentManager::Measure()
{
    m_IsLastMeasurementFailed = false;
    PushUserInput();
}

bool InstrumentManager::IsLastMeasurementFailed()
{
    return m_IsLastMeasurementFailed;
}

void InstrumentManager::ClearMeasurementFailFlag()
{
    m_IsLastMeasurementFailed = false;
}

XYZValue InstrumentManager::GetLastMeasurement()
{
    return m_LastMeasurement;
}

void InstrumentManager::ProcessBuffer()
{
    if (m_Status == InstrumentStatus::NotRunning)
    {
        const char dianosticMessage[] = "Diagnostic: ";
        size_t diagnosticsOffset = m_OutputBuffer.find(dianosticMessage);
        if (diagnosticsOffset != std::string::npos)
        {
            size_t messageBegin = diagnosticsOffset + sizeof(dianosticMessage) - 1;
            size_t messageEnd = m_OutputBuffer.find('\r', messageBegin);
            m_UserMessage = m_OutputBuffer.substr(messageBegin, messageEnd - messageBegin);
            m_OutputBuffer.clear();
            return;
        }

        m_UserMessage = std::move(m_OutputBuffer);
        m_OutputBuffer.clear();
        return;
    }

    const char calibrationMessage[] = "any key to continue,";
    size_t abortOffset = m_OutputBuffer.find(calibrationMessage);
    if (abortOffset != std::string::npos)
    {
        const char messageStartPattern[] = "\r\n\r\n";
        size_t messageBegin = m_OutputBuffer.rfind(messageStartPattern, abortOffset);
        messageBegin += sizeof(messageStartPattern) - 1;
        const char messageEndPattern[] = ",\r\n";
        size_t messageEnd = m_OutputBuffer.rfind(messageEndPattern, abortOffset);
        m_UserMessage = m_OutputBuffer.substr(messageBegin, messageEnd - messageBegin);
        m_UserMessage += " and press Proceed";
        m_Status = InstrumentStatus::NeedsUserInput;
        m_OutputBuffer.clear();
        return;
    }

    const char readyForMeasurementMessage[] = "any other key to take a reading:";
    size_t readyForMeasurementOffset = m_OutputBuffer.find(readyForMeasurementMessage);
    if (readyForMeasurementOffset != std::string::npos)
    {
        if (m_Status == InstrumentStatus::Measuring)
        {
            const char resultMessageBegin[] = " Result is XYZ: ";
            size_t resultBegin = m_OutputBuffer.find(resultMessageBegin);
            const char resultMessageEnd[] = ", D50 Lab:";
            size_t resultEnd = m_OutputBuffer.find(resultMessageBegin);
            if (resultBegin == std::string::npos || resultEnd == std::string::npos)
            {
                m_UserMessage =
                    "Failed to parse Spotread output. Can't find result after measurement.";
                Terminate();
                return;
            }

            resultBegin += sizeof(resultMessageBegin) - 1;
            std::string resultString = m_OutputBuffer.substr(resultBegin, resultEnd - resultBegin);
            XYZValue result{};

            int parseResult =
                std::sscanf(resultString.c_str(), "%f %f %f", &result.X, &result.Y, &result.Z);
            if (parseResult != 3)
            {
                m_UserMessage =
                    "Failed to parse Spotread output. Can't parse result after measurement.";
                Terminate();
                return;
            }

            m_LastMeasurement = result;
        }

        m_Status = InstrumentStatus::ReadyForMeasurement;
        m_OutputBuffer.clear();
        return;
    }

    const char failedMessage[] = "Spot read failed";
    size_t failedOffset = m_OutputBuffer.find(failedMessage);
    if (failedOffset != std::string::npos)
    {
        m_IsLastMeasurementFailed = true;
        // Need enter press to retry measurements
        m_SpotreadProcess.Write("\n", 1);
        m_Status = InstrumentStatus::StartingUp;
        m_OutputBuffer.clear();
        return;
    }
}

std::filesystem::path InstrumentManager::GetLogFilePath()
{
    auto currentTime =
        std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now());
    std::string timeString = std::format("{:%Y_%m_%d_%H_%M_%S}", currentTime);
    return std::filesystem::temp_directory_path() / ("HDRDisplayReadGUI_" + timeString + ".log");
}
