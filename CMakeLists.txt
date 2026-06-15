cmake_minimum_required(VERSION 3.25)

include_guard(GLOBAL)

project(HDRDisplayReadGUI)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(FXC_EXECUTABLE fxc.exe
        NO_DEFAULT_PATH
        PATHS "[HKLM/SOFTWARE/Microsoft/Windows Kits/Installed Roots;KitsRoot10]bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64"
        REGISTRY_VIEW HOST
        DOC "FXC shader compiler")
if(NOT FXC_EXECUTABLE)
message(FATAL_ERROR "Cannot find FXC.")
endif()

message(STATUS "Using FXC - ${FXC_EXECUTABLE}")

# Shader Build
add_custom_command(
        OUTPUT
                ${CMAKE_CURRENT_BINARY_DIR}/generated/signal_generator_vs.dxbc
                ${CMAKE_CURRENT_BINARY_DIR}/generated/signal_generator_vs.dxbc.h
                ${CMAKE_CURRENT_BINARY_DIR}/generated/signal_generator_ps.dxbc
                ${CMAKE_CURRENT_BINARY_DIR}/generated/signal_generator_ps.dxbc.h
        COMMAND ${FXC_EXECUTABLE} /nologo /T vs_5_0 /E VSMain /WX $<$<NOT:$<CONFIG:Release>>:/O3> $<$<CONFIG:Debug>:/Zi> /Fo ${CMAKE_CURRENT_BINARY_DIR}/generated/signal_generator_vs.dxbc ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/signal_generator.hlsl
        COMMAND ${CMAKE_COMMAND} -DINPUT_FILE="${CMAKE_CURRENT_BINARY_DIR}/generated/signal_generator_vs.dxbc" -DOUTPUT_FILE="${CMAKE_CURRENT_BINARY_DIR}/generated/signal_generator_vs.dxbc.h" -DVARIABLE_NAME=signal_generator_vs_bytecode -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_file.cmake"
        COMMAND ${FXC_EXECUTABLE} /nologo /T ps_5_0 /E PSMain /WX $<$<NOT:$<CONFIG:Release>>:/O3> $<$<CONFIG:Debug>:/Zi> /Fo ${CMAKE_CURRENT_BINARY_DIR}/generated/signal_generator_ps.dxbc ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/signal_generator.hlsl
        COMMAND ${CMAKE_COMMAND} -DINPUT_FILE="${CMAKE_CURRENT_BINARY_DIR}/generated/signal_generator_ps.dxbc" -DOUTPUT_FILE="${CMAKE_CURRENT_BINARY_DIR}/generated/signal_generator_ps.dxbc.h" -DVARIABLE_NAME=signal_generator_ps_bytecode -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_file.cmake"
        DEPENDS 
                cmake/embed_file.cmake
                src/shaders/cpp_shared.hlsli
                src/shaders/signal_generator.hlsl
)

# Embed Resources
add_custom_command(
        OUTPUT
                ${CMAKE_CURRENT_BINARY_DIR}/generated/open_source_licenses.h
        COMMAND ${CMAKE_COMMAND} -DINPUT_FILE="${CMAKE_CURRENT_SOURCE_DIR}/resources/open_source_licenses.txt" -DOUTPUT_FILE="${CMAKE_CURRENT_BINARY_DIR}/generated/open_source_licenses.h" -DVARIABLE_NAME=open_source_licenses_data -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_file.cmake"
        DEPENDS 
                ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_file.cmake
                ${CMAKE_CURRENT_SOURCE_DIR}/resources/open_source_licenses.txt
)

add_executable(HDRDisplayReadGUI)

set_target_properties(HDRDisplayReadGUI PROPERTIES
    WIN32_EXECUTABLE TRUE
    VS_DPI_AWARE "PerMonitor"
)

target_link_libraries(HDRDisplayReadGUI
        PRIVATE
        d3d11.lib
        dxgi.lib
)

target_sources(HDRDisplayReadGUI
        PRIVATE
        FILE_SET header_files TYPE HEADERS
        BASE_DIRS src
        FILES
        src/precompiled_header.h
        src/embeds.h
        src/resource.h
        src/subprocess.h
        src/instrument_manager.h
        src/measurement_manager.h
        src/signal_generator_window.h
        src/display_enumerator.h
        src/render_resources.h
        src/helpers.h
        src/base_window.h
        src/gui_window.h
        src/file_dialog.h

        PRIVATE
        FILE_SET generated_files TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/generated
        FILES
        ${CMAKE_CURRENT_BINARY_DIR}/generated/signal_generator_vs.dxbc.h
        ${CMAKE_CURRENT_BINARY_DIR}/generated/signal_generator_ps.dxbc.h
        ${CMAKE_CURRENT_BINARY_DIR}/generated/open_source_licenses.h

        # ImGUI sources
        PRIVATE
        external/imgui/imgui.cpp
        external/imgui/imgui_demo.cpp
        external/imgui/imgui_draw.cpp
        external/imgui/imgui_tables.cpp
        external/imgui/imgui_widgets.cpp
        external/imgui/backends/imgui_impl_dx11.cpp
        external/imgui/backends/imgui_impl_win32.cpp

        # Windows .rc resources
        PRIVATE
        src/resource.rc

        # HDRDisplayReadGUI's source code
        PRIVATE
        src/main.cpp
        src/embeds.cpp
        src/header_impl.cpp
        src/subprocess.cpp
        src/instrument_manager.cpp
        src/measurement_manager.cpp
        src/signal_generator_window.cpp
        src/display_enumerator.cpp
        src/render_resources.cpp
        src/helpers.cpp
        src/base_window.cpp
        src/gui_window.cpp
        src/file_dialog.cpp
)

set_source_files_properties(src/header_impl.cpp
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON
)

target_precompile_headers(HDRDisplayReadGUI PRIVATE src/precompiled_header.h)

target_include_directories(HDRDisplayReadGUI PRIVATE
        src
        external/imgui
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT HDRDisplayReadGUI)
