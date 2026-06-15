#include "precompiled_header.h"

#include "measurement_manager.h"

void MeasurementManager::Initialize(std::vector<MeasurementPatch>&& patchesToMeasure)
{
    HDRGUI_ASSERT(!m_IsRunning);
    HDRGUI_ASSERT(!m_IsMeasuring);

    m_Patches = std::move(patchesToMeasure);
    m_Measurements.clear();
    m_Measurements.reserve(m_Patches.size());
    m_IsError = false;
    m_IsMeasuring = false;
    m_NeedDisplayNextPatch = true;
    m_IsRunning = true;
    m_RetryCount = 0;
}

void MeasurementManager::RestartAllMeasurements()
{
    m_Measurements.clear();
    m_IsRunning = true;
    m_NeedDisplayNextPatch = true;
    m_IsError = false;
    m_RetryCount = 0;
}

void MeasurementManager::RestartLastMeasurement()
{
    m_NeedDisplayNextPatch = true;
    m_RetryCount = 0;
}

void MeasurementManager::Release()
{
    m_Patches.clear();
    m_Measurements.clear();
    m_IsRunning = false;
    m_IsMeasuring = false;
    m_NeedDisplayNextPatch = false;
    m_IsError = false;
}

bool MeasurementManager::Update()
{
    InstrumentManager::GetInstance().Update();

    if (!m_IsMeasuring ||
        (InstrumentManager::GetInstance().GetStatus() == InstrumentStatus::Measuring) ||
        (InstrumentManager::GetInstance().GetStatus() == InstrumentStatus::StartingUp))
    {
        return false;
    }

    m_IsMeasuring = false;

    if (InstrumentManager::GetInstance().GetStatus() == InstrumentStatus::Error ||
        InstrumentManager::GetInstance().GetStatus() == InstrumentStatus::NotRunning)
    {
        m_IsRunning = false;
        m_IsError = true;
        return false;
    }

    if (InstrumentManager::GetInstance().IsLastMeasurementFailed())
    {
        InstrumentManager::GetInstance().ClearMeasurementFailFlag();

        ++m_RetryCount;
        if (m_RetryCount >= ms_MaxRetryCount)
        {
            m_IsRunning = false;
            m_IsError = true;
        }
        else
        {
            m_NeedDisplayNextPatch = true;
        }
        return false;
    }
    else
    {
        m_RetryCount = 0;
    }

    m_Measurements.push_back(InstrumentManager::GetInstance().GetLastMeasurement());

    if (m_Measurements.size() == m_Patches.size())
    {
        m_IsRunning = false;
    }
    else
    {
        m_NeedDisplayNextPatch = true;
    }

    return true;
}

bool MeasurementManager::IsRunning()
{
    return m_IsRunning;
}

bool MeasurementManager::IsError()
{
    return m_IsError;
}

std::string MeasurementManager::GetStatusString()
{
    std::string result =
        std::format("Measuring patch {} of {}", m_Measurements.size() + 1, m_Patches.size());

    if (!m_Measurements.empty())
    {
        const XYZValue& lastMeasurement = m_Measurements.back();
        result += std::format("\nLast measurement:\nX={:11.5f}\nY={:11.5f}\nZ={:11.5f}",
                              lastMeasurement.X, lastMeasurement.Y, lastMeasurement.Z);
    }

    if (m_RetryCount > 0)
    {
        result += std::format("\nLast measurement failed\nRetry {} out of {}", m_RetryCount,
                              ms_MaxRetryCount);
    }

    return result;
}

const std::vector<MeasurementPatch>& MeasurementManager::GetPatches()
{
    return m_Patches;
}

const std::vector<XYZValue>& MeasurementManager::GetMeasurements()
{
    return m_Measurements;
}

bool MeasurementManager::NeedDisplayNextPatch()
{
    bool res = m_NeedDisplayNextPatch;
    m_NeedDisplayNextPatch = false;
    return res;
}

MeasurementPatch MeasurementManager::GetPatchToDisplay()
{
    HDRGUI_ASSERT(m_IsRunning);
    HDRGUI_ASSERT(m_Measurements.size() < m_Patches.size());

    return m_Patches[m_Measurements.size()];
}

void MeasurementManager::SignalPresent()
{
    HDRGUI_ASSERT(m_IsRunning);

    if (!m_IsMeasuring)
    {
        InstrumentManager::GetInstance().Measure();
        m_IsMeasuring = true;
    }
}
