#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Windows.h and its macros
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "Windows.h"

// Other Windows headers
#include "Shlobj.h"
#include "d3d11.h"
#include "dwmapi.h"
#include "dxgi1_6.h"
#include "shellapi.h"
#include "wrl/client.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include "imgui.h"

// Internal headers
#include "helpers.h"
#include "resource.h"
