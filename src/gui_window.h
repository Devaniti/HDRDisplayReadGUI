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
#pragma once

#include "base_window.h"
#include "measurement_manager.h"
#include "render_resources.h"
#include "signal_generator_window.h"

class GUIWindow : public BaseWindow
{
public:
    GUIWindow() = default;
    ~GUIWindow() = default;

protected:
    void InitDerived(HWND hwnd, UINT clientWidth, UINT clientHeight, UINT windowWidth,
                     UINT windowHeight) final;
    void ReleaseDerived() final;
    LRESULT WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) final;

private:
    enum class MeasurementState
    {
        WaitingForSpotreadPath,
        PatchesSelection,
        DisplaySelection,
        ReadyToStartInstrument,
        InstrumentStarting,
        ReadyForMeasurement,
        CheckingInstrument,
        Measuring,
        Finished,
    };

    void RenderFrame();
    void RenderImGui();

    void HandlePickSpotreadPath();
    void HandlePickPatchesFile();
    void HandleSaveResults();

    void TransitionToAnotherState(MeasurementState newState);
    void OnEnterState(MeasurementState state);

    void MoveToDisplay(int displayIndex);

    std::vector<MeasurementPatch> GetInstrumentCheckPatches();
    bool IsInstrumentCheckMeasurementsGood();
    bool SaveMeasurements(std::filesystem::path outputPath);

    void SaveSpotreadPath(const std::filesystem::path& spotreadPath);
    std::filesystem::path LoadSpotreadPath();

    void SetSleepOverride(bool prevent);
    void SetDisplayIndexFromWindowPos();

    UINT m_PosX;
    UINT m_PosY;
    UINT m_ClientWidth;
    UINT m_ClientHeight;
    UINT m_WindowWidth;
    UINT m_WindowHeight;

    RenderResources m_RenderResources;

    int m_DisplayIndex = 0;
    int m_SelectedDisplay = 0;
    MeasurementState m_CurrentState = MeasurementState::WaitingForSpotreadPath;
    std::string m_LastError;
    bool m_MeasurementsStarted = false;
    std::filesystem::path m_CachedSpotreadPath;
    std::string m_CachedSpotreadButtonLabel;

    MeasurementManager m_MeasurementManagerInstrumentCheck;
    MeasurementManager m_MeasurementManagerActualMeasurements;
    SignalGeneratorWindow m_SignalGenerator;
};
