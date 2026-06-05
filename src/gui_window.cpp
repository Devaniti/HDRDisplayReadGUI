#include "precompiled_header.h"

#include "gui_window.h"

#include "display_enumerator.h"
#include "embeds.h"
#include "file_dialog.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
                                                             LPARAM lParam);

void GUIWindow::InitDerived(HWND hwnd, UINT clientWidth, UINT clientHeight, UINT windowWidth,
                            UINT windowHeight)
{
    BOOL value = TRUE;
    HRESULT hr =
        ::DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
    CHECK_HR(hr, hwnd, L"DwmSetWindowAttribute call failed");

    m_RenderResources.Init(hwnd, clientWidth, clientHeight, false);

    m_ClientWidth = clientWidth;
    m_ClientHeight = clientHeight;
    m_WindowWidth = windowWidth;
    m_WindowHeight = windowHeight;

    DisplayEnumerator::GetInstance();

    IMGUI_CHECKVERSION();

    ImGui_ImplWin32_EnableDpiAwareness();
    float scalingFactor = ImGui_ImplWin32_GetDpiScaleForMonitor(
        ::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));

    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(scalingFactor);
    style.FontScaleDpi = scalingFactor;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(m_RenderResources.GetDevice(), m_RenderResources.GetDeviceContext());

    MoveToDisplay(0);
    ::SetTimer(hwnd, 0, 16, NULL);

    m_MeasurementManagerInstrumentCheck.Initialize(GetInstrumentCheckPatches());
    m_CachedSpotreadPath = LoadSpotreadPath();
    m_CachedSpotreadButtonLabel = "Use " + wStringToString(m_CachedSpotreadPath.wstring());
}

void GUIWindow::ReleaseDerived()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    m_RenderResources.Release();
    m_SignalGenerator.Stop();
}

LRESULT GUIWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static bool isPainting = false;
    // Ensure that we don't have recursive message handling during painting
    HDRGUI_ASSERT(!isPainting);

    if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam))
    {
        return true;
    }

    switch (message)
    {
    case WM_MOVE: {
        m_PosX = LOWORD(lParam);
        m_PosY = HIWORD(lParam);
        SetDisplayIndexFromWindowPos();
        return 0;
    }
    case WM_PAINT: {
        isPainting = true;
        RenderFrame();
        ::ValidateRect(hwnd, NULL);
        isPainting = false;
        return 0;
    }
    case WM_GETMINMAXINFO: {
        LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
        lpMMI->ptMinTrackSize.x = m_WindowWidth;
        lpMMI->ptMinTrackSize.y = m_WindowHeight;
        lpMMI->ptMaxTrackSize.x = m_WindowWidth;
        lpMMI->ptMaxTrackSize.y = m_WindowHeight;
        return 0;
    }
    case WM_TIMER: {
        ::InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    case WM_SIZING: {
        RECT* rect = (RECT*)lParam;
        rect->right = rect->left + m_WindowWidth;
        rect->bottom = rect->top + m_WindowHeight;
        return TRUE;
    }
    case WM_NCDESTROY:
        ::PostQuitMessage(0);
        return 0;
    case WM_DISPLAYCHANGE:
        if (m_CurrentState == MeasurementState::CheckingInstrument ||
            m_CurrentState == MeasurementState::Measuring)
        {
            TransitionToAnotherState(MeasurementState::DisplaySelection);
            m_LastError =
                "Display configuration changed. This may have brought target display out of HDR "
                "mode, made it blink, turned it off, etc.",
            "HDR Measurement GUI Fatal Error";
            m_SignalGenerator.Stop();
            m_MeasurementManagerInstrumentCheck.RestartAllMeasurements();
            m_MeasurementManagerActualMeasurements.RestartAllMeasurements();
            InstrumentManager::GetInstance().Terminate();
        }
        DisplayEnumerator::GetInstance().Reenumerate();
        if (m_DisplayIndex >= DisplayEnumerator::GetInstance().GetDisplayCount())
        {
            m_DisplayIndex = 0;
        }
        return 0;
    case WM_USER_GUI_WINDOW_PICK_SPOTREAD_PATH:
        HandlePickSpotreadPath();
        return 0;
    case WM_USER_GUI_WINDOW_OPEN_ARGYLLCMS_DOWNLOAD_PAGE:
        ::ShellExecuteW(nullptr, L"open", L"https://www.argyllcms.com/downloadwin.html", nullptr,
                        nullptr, SW_SHOWDEFAULT);
        return 0;
    case WM_USER_GUI_WINDOW_SHOW_OPEN_SOURCE_LICENSES: {
        std::filesystem::path tempFilePath =
            std::filesystem::temp_directory_path() / "HDRMeasurementGUI_OpenSourceLicenses.txt";
        {
            std::ofstream tempFileStream(tempFilePath, std::ios::binary);
            MemoryBlock fileData = GetOpenSourceLicensesData();
            tempFileStream.write((const char*)fileData.data, fileData.size);
        }

        std::wstring tempFilePathStr = tempFilePath.wstring();
        ::ShellExecuteW(nullptr, L"open", tempFilePathStr.c_str(), nullptr, nullptr,
                        SW_SHOWDEFAULT);
        return 0;
    }
    case WM_USER_GUI_WINDOW_PICK_PATCHES_FILE:
        HandlePickPatchesFile();
        return 0;
    case WM_USER_GUI_WINDOW_SAVE_RESULT:
        HandleSaveResults();
        return 0;
    case WM_USER_GUI_WINDOW_MOVE_WINDOW_TO_NEW_DISPLAY: {
        int displayCount = DisplayEnumerator::GetInstance().GetDisplayCount();
        if (m_DisplayIndex >= displayCount)
        {
            m_DisplayIndex = 0;
        }
        MoveToDisplay(m_DisplayIndex);
        return 0;
    }
    case WM_USER_GUI_WINDOW_OPEN_DISPLAY_SETTINGS: {
        ::ShellExecuteW(nullptr, L"open", L"ms-settings:display", nullptr, nullptr, SW_SHOWDEFAULT);
        return 0;
    }
    case WM_USER_GUI_WINDOW_NEW_PATCH_PRESENTED: {
        if (m_CurrentState == MeasurementState::CheckingInstrument)
        {
            m_MeasurementManagerInstrumentCheck.SignalPresent();
        }
        else if (m_CurrentState == MeasurementState::Measuring)
        {
            m_MeasurementManagerActualMeasurements.SignalPresent();
        }
        return 0;
    }
    default:
        return ::DefWindowProcW(hwnd, message, wParam, lParam);
    }

    // All cases should return a value, so that we never reach this point
    HDRGUI_ASSERT(0);
}

void GUIWindow::RenderFrame()
{
    ID3D11Device* device = m_RenderResources.GetDevice();
    ID3D11DeviceContext* context = m_RenderResources.GetDeviceContext();
    IDXGISwapChain3* swapchain = m_RenderResources.GetSwapchain();
    ID3D11RenderTargetView* backbufferRTV = m_RenderResources.GetSwapchainBackbufferRTV();

    ID3D11RenderTargetView* RTVToSet[] = {backbufferRTV};

    context->OMSetRenderTargets(1, RTVToSet, NULL);

    RenderImGui();

    HRESULT hr = swapchain->Present(1, 0);
    CHECK_HR(hr, GetHWND(), L"IDXGISwapChain::Present call failed");
}

void GUIWindow::RenderImGui()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::Begin("HDR Measurement GUI", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);

    ImGui::PushTextWrapPos();

    switch (m_CurrentState)
    {
    case MeasurementState::WaitingForSpotreadPath: {
        ImGui::Text("Please select spotread binary file\nYou can find it in \"bin\" directory in "
                    "ArgyllCMS Windows executables package");
        if (ImGui::Button("Select##SpotreadPath"))
        {
            ::PostMessage(GetHWND(), WM_USER_GUI_WINDOW_PICK_SPOTREAD_PATH, 0, 0);
        }
        if (!m_CachedSpotreadPath.empty())
        {
            if (ImGui::Button(m_CachedSpotreadButtonLabel.c_str()))
            {
                InstrumentManager::GetInstance().SetSpotreadPath(std::move(m_CachedSpotreadPath));
                TransitionToAnotherState(MeasurementState::PatchesSelection);
            }
        }
        if (ImGui::Button("Open ArgyllCMS Download Page"))
        {
            ::PostMessage(GetHWND(), WM_USER_GUI_WINDOW_OPEN_ARGYLLCMS_DOWNLOAD_PAGE, 0, 0);
        }
        if (ImGui::Button("Show Opens Source Licenses"))
        {
            ::PostMessage(GetHWND(), WM_USER_GUI_WINDOW_SHOW_OPEN_SOURCE_LICENSES, 0, 0);
        }
        break;
    }
    case MeasurementState::PatchesSelection: {
        ImGui::Text("Please select list of patches to measure");
        static int currentSelection = 0;
        ImGui::Combo("##category", &currentSelection,
                     "Luminance Curve\0Min/Max Luminance\0Color Gamut\0Custom\0");
        switch (currentSelection)
        {
        case 0: { // Luminance Curve
            ImGui::Text("Window Sizes");
            static constexpr int windowSizesCount = 7;
            static float windowSizesValues[windowSizesCount] = {0.01f, 0.02f, 0.05f, 0.1f,
                                                                0.25f, 0.5f,  1.0f};
            static bool windowSizesEnable[windowSizesCount] = {true, true, true, true,
                                                               true, true, true};
            ImGui::Checkbox("1%", &windowSizesEnable[0]);
            ImGui::SameLine();
            ImGui::Checkbox("2%", &windowSizesEnable[1]);
            ImGui::SameLine();
            ImGui::Checkbox("5%", &windowSizesEnable[2]);
            ImGui::SameLine();
            ImGui::Checkbox("10%", &windowSizesEnable[3]);
            ImGui::Checkbox("25%", &windowSizesEnable[4]);
            ImGui::SameLine();
            ImGui::Checkbox("50%", &windowSizesEnable[5]);
            ImGui::SameLine();
            ImGui::Checkbox("100%", &windowSizesEnable[6]);
            static int measurementCountPow2 = 10;
            int measurementCount = intPow(2, measurementCountPow2);
            int measurementIncrement = 1024 / measurementCount;
            std::string measurementCountLabel = std::to_string(measurementCount);
            ImGui::Text("Measurement count per patch size");
            ImGui::SliderInt("##Measurement count per patch size", &measurementCountPow2, 6, 10,
                             measurementCountLabel.c_str());

            int windowCount = 0;
            for (bool isWindowEnabled : windowSizesEnable)
            {
                windowCount += isWindowEnabled;
            }

            ImGui::BeginDisabled(windowCount == 0);
            if (ImGui::Button("Select##Window Sizes"))
            {
                std::vector<MeasurementPatch> patches;
                patches.reserve(1024 * windowCount);
                for (int i = 0; i < windowSizesCount; ++i)
                {
                    if (!windowSizesEnable[i])
                    {
                        continue;
                    }

                    float windowSize = windowSizesValues[i];

                    for (uint16_t i = 0; i < 1024; i += measurementIncrement)
                    {
                        patches.push_back(
                            {.WindowSize = windowSize, .HDR10R = i, .HDR10G = i, .HDR10B = i});
                    }
                }

                m_MeasurementManagerActualMeasurements.Initialize(std::move(patches));
                TransitionToAnotherState(MeasurementState::DisplaySelection);
            }
            ImGui::EndDisabled();
            break;
        }
        case 1: // Min/Max Luminance
            if (ImGui::Button("Select##Min/Max Luminance"))
            {
                std::vector<MeasurementPatch> patches;
                static float windowSizesValues[] = {0.01f, 0.02f, 0.05f, 0.1f, 0.25f, 0.5f, 1.0f};
                patches.reserve(sizeof(windowSizesValues) / sizeof(windowSizesValues[0]) + 1);
                patches.push_back({.WindowSize = 1.0f, .HDR10R = 0, .HDR10G = 0, .HDR10B = 0});
                for (float windowSize : windowSizesValues)
                {
                    patches.push_back(
                        {.WindowSize = windowSize, .HDR10R = 1024, .HDR10G = 1024, .HDR10B = 1024});
                }

                m_MeasurementManagerActualMeasurements.Initialize(std::move(patches));
                TransitionToAnotherState(MeasurementState::DisplaySelection);
            }
            break;
        case 2: // Color Gamut
            if (ImGui::Button("Select##Color Gamut"))
            {
                std::vector<MeasurementPatch> patches;
                patches.reserve(4);
                patches.push_back({.WindowSize = 1.0f, .HDR10R = 1024, .HDR10G = 0, .HDR10B = 0});
                patches.push_back({.WindowSize = 1.0f, .HDR10R = 0, .HDR10G = 1024, .HDR10B = 0});
                patches.push_back({.WindowSize = 1.0f, .HDR10R = 0, .HDR10G = 0, .HDR10B = 1024});
                patches.push_back(
                    {.WindowSize = 1.0f, .HDR10R = 1024, .HDR10G = 1024, .HDR10B = 1024});

                m_MeasurementManagerActualMeasurements.Initialize(std::move(patches));
                TransitionToAnotherState(MeasurementState::DisplaySelection);
            }
            break;
        case 3: // Custom
            ImGui::Text("Select .csv file with patches to measure.\nFormat (one line per "
                        "measurement):\nWindowSize,HDR10R,HDR10G,HDR10B\nExample:\n0.01,1024,0,0\n."
                        "csv file should not have a header");
            if (ImGui::Button("Select .csv"))
            {
                ::PostMessage(GetHWND(), WM_USER_GUI_WINDOW_PICK_PATCHES_FILE, 0, 0);
            }
            break;
        default:
            assert(0);
            break;
        }
        break;
    }
    case MeasurementState::DisplaySelection: {
        int displayCount = DisplayEnumerator::GetInstance().GetDisplayCount();

        const DisplayDesc& display = DisplayEnumerator::GetInstance().GetDisplay(m_DisplayIndex);

        ImGui::Text("Please select display");
        ImGui::Text("You can either move this window to target display, or use buttons below");
        ImGui::Text("Current display: %d", m_DisplayIndex);
        ImGui::BeginDisabled(displayCount == 1);
        if (ImGui::ArrowButton("##left", ImGuiDir_Left))
        {
            --m_DisplayIndex;
            if (m_DisplayIndex < 0)
            {
                m_DisplayIndex = displayCount - 1;
            }
            ::PostMessage(GetHWND(), WM_USER_GUI_WINDOW_MOVE_WINDOW_TO_NEW_DISPLAY, 0, 0);
        }
        ImGui::SameLine();
        if (ImGui::ArrowButton("##right", ImGuiDir_Right))
        {
            ++m_DisplayIndex;
            if (m_DisplayIndex >= displayCount)
            {
                m_DisplayIndex = 0;
            }
            ::PostMessage(GetHWND(), WM_USER_GUI_WINDOW_MOVE_WINDOW_TO_NEW_DISPLAY, 0, 0);
        }
        ImGui::EndDisabled();
        ImGui::Text("Desktop Coordinates:\n(%d, %d)-(%d, %d)\n[%d x %d]",
                    display.DesktopCoordinates.left, display.DesktopCoordinates.top,
                    display.DesktopCoordinates.right, display.DesktopCoordinates.bottom,
                    display.DesktopCoordinates.right - display.DesktopCoordinates.left,
                    display.DesktopCoordinates.bottom - display.DesktopCoordinates.top);
        ImGui::Text("HDR Enabled: %s", display.IsHDR ? "Yes" : "No");

        ImGui::BeginDisabled(!display.IsHDR);
        if (ImGui::Button("Select this display"))
        {
            m_SelectedDisplay = m_DisplayIndex;
            m_SignalGenerator.OpenAsync(
                GetHWND(), display.DesktopCoordinates.left, display.DesktopCoordinates.top,
                display.DesktopCoordinates.right - display.DesktopCoordinates.left,
                display.DesktopCoordinates.bottom - display.DesktopCoordinates.top);
            TransitionToAnotherState(MeasurementState::ReadyToStartInstrument);
        }
        ImGui::EndDisabled();

        if (!display.IsHDR)
        {
            ImGui::Text("Please make sure that display supports HDR and enable it in Display "
                        "Settings.\nYou don't need to restart the app.");
            if (ImGui::Button("Open Display Settings"))
            {
                ::PostMessage(GetHWND(), WM_USER_GUI_WINDOW_OPEN_DISPLAY_SETTINGS, 0, 0);
            }
        }

        break;
    }
    case MeasurementState::ReadyToStartInstrument: {
        ImGui::Text("Connect instrument and press proceed");
        if (ImGui::Button("Proceed"))
        {
            InstrumentManager::GetInstance().StartSpotread();
            TransitionToAnotherState(MeasurementState::InstrumentStarting);
        }
        break;
    }
    case MeasurementState::InstrumentStarting: {
        InstrumentManager::GetInstance().Update();
        switch (InstrumentManager::GetInstance().GetStatus())
        {
        case InstrumentStatus::NotRunning:
            TransitionToAnotherState(MeasurementState::ReadyToStartInstrument);
            m_LastError = std::format("Spotread failed with error:\n{}",
                                      InstrumentManager::GetInstance().GetUserMessage());
            break;
        case InstrumentStatus::StartingUp:
            ImGui::Text("Please wait...");
            break;
        case InstrumentStatus::NeedsUserInput:
            ImGui::Text("%s", InstrumentManager::GetInstance().GetUserMessage().c_str());
            if (ImGui::Button("Proceed"))
            {
                InstrumentManager::GetInstance().PushUserInput();
            }
            break;
        case InstrumentStatus::ReadyForMeasurement:
            TransitionToAnotherState(MeasurementState::ReadyForMeasurement);
            break;
        case InstrumentStatus::Measuring:
            HDRGUI_ASSERT(0);
            break;
        case InstrumentStatus::Error:
            TransitionToAnotherState(MeasurementState::ReadyForMeasurement);
            HDRGUI_ASSERT(0);
            break;
        }
        break;
    }
    case MeasurementState::ReadyForMeasurement: {
        if (DisplayEnumerator::GetInstance().GetDisplayCount() > 1 &&
            (m_DisplayIndex == m_SelectedDisplay))
        {
            ImGui::Text("Move this window to another display, to ensure it doesn't interfere with "
                        "measurements.");
        }
        else
        {
            ImGui::Text(
                "Set instrument into emissive measurement mode and place it on the test window.");
            if (ImGui::Button("Start measurements"))
            {
                m_MeasurementManagerInstrumentCheck.RestartAllMeasurements();
                TransitionToAnotherState(MeasurementState::CheckingInstrument);
            }
        }
        break;
    }
    case MeasurementState::CheckingInstrument:
        ImGui::Text("Checking instrument...");
        m_MeasurementManagerInstrumentCheck.Update();
        if (InstrumentManager::GetInstance().GetStatus() == InstrumentStatus::NeedsUserInput)
        {
            TransitionToAnotherState(MeasurementState::ReadyToStartInstrument);
            m_LastError = "Instrument Check Failed: Spotread unexpectedly requires user input "
                          "during instrument check";
            break;
        }
        if (m_MeasurementManagerInstrumentCheck.NeedDisplayNextPatch())
        {
            m_SignalGenerator.DisplayPatch(m_MeasurementManagerInstrumentCheck.GetPatchToDisplay());
        }
        if (m_MeasurementManagerInstrumentCheck.IsError())
        {
            TransitionToAnotherState(MeasurementState::ReadyToStartInstrument);
            m_LastError =
                "Measurement failed: " + InstrumentManager::GetInstance().GetUserMessage();
            break;
        }
        if (!m_MeasurementManagerInstrumentCheck.IsRunning())
        {
            if (!IsInstrumentCheckMeasurementsGood())
            {
                TransitionToAnotherState(MeasurementState::ReadyForMeasurement);
                m_LastError = "Instrument check failed: unexpected readings, make sure that "
                              "instrument is correctly placed and in measurement position";
                break;
            }

            TransitionToAnotherState(MeasurementState::Measuring);
            break;
        }

        ImGui::Text("%s", m_MeasurementManagerInstrumentCheck.GetStatusString().c_str());
        break;
    case MeasurementState::Measuring:
        ImGui::Text("Performing measurements...");
        m_MeasurementManagerActualMeasurements.Update();
        if (InstrumentManager::GetInstance().GetStatus() == InstrumentStatus::NeedsUserInput)
        {
            m_MeasurementManagerActualMeasurements.RestartLastMeasurement();
            TransitionToAnotherState(MeasurementState::InstrumentStarting);
            m_LastError = "Measurements paused, instrument requires recalibration. Measurements "
                          "will continue after recalibration and another instrument check.";
            break;
        }
        if (m_MeasurementManagerActualMeasurements.NeedDisplayNextPatch())
        {
            m_SignalGenerator.DisplayPatch(
                m_MeasurementManagerActualMeasurements.GetPatchToDisplay());
        }
        if (m_MeasurementManagerActualMeasurements.IsError())
        {
            TransitionToAnotherState(MeasurementState::ReadyToStartInstrument);
            m_LastError =
                "Measurement failed: " + InstrumentManager::GetInstance().GetUserMessage();
            break;
        }
        if (!m_MeasurementManagerActualMeasurements.IsRunning())
        {
            TransitionToAnotherState(MeasurementState::Finished);
            break;
        }

        ImGui::Text("%s", m_MeasurementManagerActualMeasurements.GetStatusString().c_str());
        break;
    case MeasurementState::Finished:
        ImGui::Text("Finished");
        if (ImGui::Button("Save"))
        {
            PostMessage(GetHWND(), WM_USER_GUI_WINDOW_SAVE_RESULT, 0, 0);
        }
        break;
    default:
        HDRGUI_ASSERT(0);
        break;
    }

    if (!m_LastError.empty())
    {
        ImGui::Text("%s", m_LastError.c_str());
    }

    ImGui::PopTextWrapPos();
    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void GUIWindow::HandlePickSpotreadPath()
{
    std::filesystem::path candidatePath =
        OpenFileDialog(GetHWND(), FileDialogOperation::Load, FileDialogTarget::File,
                       L"spotread.exe", L"spotread.exe");

    if (candidatePath.empty())
    {
        return;
    }

    if (!std::filesystem::exists(candidatePath))
    {
        MessageBoxW(GetHWND(), L"Selected file doesn't exist", L"Invalid spotread path",
                    MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        return;
    }

    SaveSpotreadPath(candidatePath);
    InstrumentManager::GetInstance().SetSpotreadPath(std::move(candidatePath));
    TransitionToAnotherState(MeasurementState::DisplaySelection);
}

void GUIWindow::HandlePickPatchesFile()
{
    std::filesystem::path selectedPath = OpenFileDialog(
        GetHWND(), FileDialogOperation::Load, FileDialogTarget::File, L"CSV files", L"*.csv");

    if (selectedPath.empty())
    {
        return;
    }

    std::ifstream patchesCSV(selectedPath);
    if (!patchesCSV.is_open())
    {
        MessageBoxW(GetHWND(), L"Failed to open selected file", L"HDR Measurement GUI Error",
                    MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        return;
    }

    std::vector<MeasurementPatch> patches;

    std::string line;
    while (std::getline(patchesCSV, line))
    {
        if (line.empty())
        {
            continue;
        }

        MeasurementPatch patch{};
        // Need to pass int's to sscanf
        int HDR10R = 0;
        int HDR10G = 0;
        int HDR10B = 0;
        int parsedChars = 0;

        int res = std::sscanf(line.c_str(), "%f,%d,%d,%d %n", &patch.WindowSize, &HDR10R, &HDR10G,
                              &HDR10B, &parsedChars);
        if (res != 4 || parsedChars != line.size() || patch.WindowSize < 0.0f ||
            patch.WindowSize > 1.0f || HDR10R < 0 || HDR10G < 0 || HDR10B < 0 || HDR10R > 1023 ||
            HDR10G > 1023 || HDR10B > 1023)
        {
            std::string errorMessage = "Failed to parse string \"" + line + "\"";
            MessageBoxA(GetHWND(), errorMessage.c_str(), "HDR Measurement GUI Error",
                        MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
            return;
        }

        patch.HDR10R = HDR10R;
        patch.HDR10G = HDR10G;
        patch.HDR10B = HDR10B;

        patches.push_back(patch);
    }

    m_MeasurementManagerActualMeasurements.Initialize(std::move(patches));
    TransitionToAnotherState(MeasurementState::DisplaySelection);
}

void GUIWindow::HandleSaveResults()
{
    std::filesystem::path savePath = OpenFileDialog(GetHWND(), FileDialogOperation::Save,
                                                    FileDialogTarget::File, L"CSV files", L"*.csv");

    if (savePath.empty())
    {
        return;
    }

    if (!SaveMeasurements(savePath))
    {
        ::MessageBoxW(GetHWND(), L"Failed to save measurements", L"Error",
                      MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        return;
    }

    ::MessageBoxW(GetHWND(), L"Measurements saved successfully", L"Success",
                  MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
}

void GUIWindow::TransitionToAnotherState(MeasurementState newState)
{
    OnEnterState(newState);
    m_CurrentState = newState;
    m_LastError.clear();
}

void GUIWindow::OnEnterState(MeasurementState state)
{
    SetSleepOverride(state >= MeasurementState::ReadyForMeasurement &&
                     state < MeasurementState::Finished);

    switch (state)
    {
    case MeasurementState::WaitingForSpotreadPath:
        break;
    case MeasurementState::DisplaySelection:
        break;
    case MeasurementState::ReadyToStartInstrument:
        InstrumentManager::GetInstance().Terminate();
        break;
    case MeasurementState::InstrumentStarting:
        break;
    case MeasurementState::ReadyForMeasurement:
        m_SignalGenerator.DisplayPatch(DefaultMeasurementPatch);
        break;
    case MeasurementState::CheckingInstrument:
        break;
    case MeasurementState::Measuring:
        break;
    case MeasurementState::Finished:
        m_SignalGenerator.DisplayPatch(DefaultMeasurementPatch);
        break;
    }
}

void GUIWindow::MoveToDisplay(int displayIndex)
{
    const RECT& displayCoordinates =
        DisplayEnumerator::GetInstance().GetDisplay(displayIndex).DesktopCoordinates;

    UINT displayWidth = displayCoordinates.right - displayCoordinates.left;
    UINT displayHeight = displayCoordinates.bottom - displayCoordinates.top;
    m_PosX = displayCoordinates.left + displayWidth * 3 / 4 - m_WindowWidth / 2;
    m_PosY = displayCoordinates.top + displayHeight / 2 - m_WindowHeight / 2;

    ::MoveWindow(GetHWND(), m_PosX, m_PosY, m_WindowWidth, m_WindowHeight, FALSE);
}

std::vector<MeasurementPatch> GUIWindow::GetInstrumentCheckPatches()
{
    std::vector<MeasurementPatch> result{
        {.WindowSize = 0.01f, .HDR10R = 0, .HDR10G = 0, .HDR10B = 0},
        {.WindowSize = 0.01f, .HDR10R = 1023, .HDR10G = 1023, .HDR10B = 1023},
        {.WindowSize = 0.01f, .HDR10R = 1023, .HDR10G = 0, .HDR10B = 0},
        {.WindowSize = 0.01f, .HDR10R = 0, .HDR10G = 1023, .HDR10B = 0},
        {.WindowSize = 0.01f, .HDR10R = 0, .HDR10G = 0, .HDR10B = 1023},
        {.WindowSize = 0.01f, .HDR10R = 0, .HDR10G = 0, .HDR10B = 0},
        {.WindowSize = 0.01f, .HDR10R = 1023, .HDR10G = 1023, .HDR10B = 1023}};
    return result;
}

bool GUIWindow::IsInstrumentCheckMeasurementsGood()
{
    // Check is *very* generous to display and intstrument precision
    // We just want to make sure that instrument is properly set up to measure display
    const std::vector<XYZValue>& measurements =
        m_MeasurementManagerInstrumentCheck.GetMeasurements();

    // Check black patches
    if ((measurements[0].Y > 15.0f) || (measurements[5].Y > 15.0f))
    {
        return false;
    }
    // No more than 5 nits difference between black measurements
    if (std::abs(measurements[0].Y - measurements[5].Y) > 5.0f)
    {
        return false;
    }

    // Check white patches
    if ((measurements[1].Y < 100.0f) || (measurements[6].Y < 100.0f))
    {
        return false;
    }
    // No more than 5% diff between white measurements
    if (std::abs(1.0f - measurements[1].Y / measurements[6].Y) > 0.05f)
    {
        return false;
    }

    // RGB measurements
    float r_x = measurements[2].X / (measurements[2].X + measurements[2].Y + measurements[2].Z);
    if ((r_x < 0.3333f) || (measurements[2].Y < 26.0f))
    {
        return false;
    }
    float g_y = measurements[3].Y / (measurements[3].X + measurements[3].Y + measurements[3].Z);
    if ((g_y < 0.3333f) || (measurements[3].Y < 68.0f))
    {
        return false;
    }
    float b_x = measurements[4].X / (measurements[4].X + measurements[4].Y + measurements[4].Z);
    float b_y = measurements[4].Y / (measurements[4].X + measurements[4].Y + measurements[4].Z);
    if ((b_x > 0.3333f) || (b_y > 0.3333f) || (measurements[2].Y < 6.0f))
    {
        return false;
    }

    return true;
}

bool GUIWindow::SaveMeasurements(std::filesystem::path outputPath)
{
    std::ofstream output(outputPath);
    if (!output.is_open())
    {
        return false;
    }

    output << "Window Size,HDR10R,HDR10G,HDR10B,XYZ X,XYZ Y,XYZ Z\n";

    const std::vector<MeasurementPatch>& patches =
        m_MeasurementManagerActualMeasurements.GetPatches();
    const std::vector<XYZValue>& measurements =
        m_MeasurementManagerActualMeasurements.GetMeasurements();
    HDRGUI_ASSERT(patches.size() == measurements.size());

    for (size_t i = 0; i < measurements.size(); ++i)
    {
        output << patches[i].WindowSize << "," << patches[i].HDR10R << "," << patches[i].HDR10G
               << "," << patches[i].HDR10B << "," << measurements[i].X << "," << measurements[i].Y
               << "," << measurements[i].Z << "\n";
    }

    output.flush();

    if (output.bad())
    {
        return false;
    }

    return true;
}

void GUIWindow::SaveSpotreadPath(const std::filesystem::path& spotreadPath)
{
    std::filesystem::path saveFilePath =
        std::filesystem::temp_directory_path() / "HDRMeasurementGUI_SpotreadPath.txt";
    std::wofstream saveFile(saveFilePath);
    saveFile << spotreadPath.wstring() << std::endl;
}

std::filesystem::path GUIWindow::LoadSpotreadPath()
{
    std::filesystem::path saveFilePath =
        std::filesystem::temp_directory_path() / "HDRMeasurementGUI_SpotreadPath.txt";
    std::wifstream saveFile(saveFilePath);
    if (!saveFile.is_open())
    {
        return {};
    }

    std::wstring pathString;
    saveFile >> pathString;
    std::filesystem::path candidatePath;

    try
    {
        candidatePath = pathString;

        if (!std::filesystem::exists(candidatePath))
        {
            return {};
        }
    }
    catch (std::filesystem::filesystem_error)
    {
        return {};
    }

    return candidatePath;
}

void GUIWindow::SetSleepOverride(bool prevent)
{
    SetThreadExecutionState(ES_CONTINUOUS |
                            (prevent ? (ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED) : 0));
}

void GUIWindow::SetDisplayIndexFromWindowPos()
{
    UINT centerX = m_PosX + m_WindowWidth / 2;
    UINT centerY = m_PosY + m_WindowHeight / 2;

    for (int i = 0; i < DisplayEnumerator::GetInstance().GetDisplayCount(); ++i)
    {
        const RECT& displayCoordinates =
            DisplayEnumerator::GetInstance().GetDisplay(i).DesktopCoordinates;

        if (centerX >= displayCoordinates.left && centerX < displayCoordinates.right &&
            centerY >= displayCoordinates.top && centerY < displayCoordinates.bottom)
        {
            m_DisplayIndex = i;
            return;
        }
    }

    // Don't touch current display index if out of bounds of any display
}
