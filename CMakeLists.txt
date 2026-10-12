cmake_minimum_required(VERSION 3.21)

project(kodak
    VERSION 2.0.0
    DESCRIPTION "Screenshots that redact sensitive data before they leave the computer"
    LANGUAGES CXX)

# The static C runtime: kodak.exe runs without the Visual C++ Redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(KODAK_BUILD_TESTS "Build the Kodak unit tests" ${PROJECT_IS_TOP_LEVEL})
option(KODAK_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

# Warning flags and Win32 definitions shared by every target.
add_library(kodak_build_options INTERFACE)
target_compile_features(kodak_build_options INTERFACE cxx_std_20)
if(WIN32)
    target_compile_definitions(kodak_build_options INTERFACE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
endif()
if(MSVC)
    target_compile_options(kodak_build_options INTERFACE /W4 /permissive- /utf-8
        $<$<BOOL:${KODAK_WARNINGS_AS_ERRORS}>:/WX>)
else()
    # Aggregates such as Finding are brace-initialised partially on purpose: the
    # remaining members have default member initialisers.
    target_compile_options(kodak_build_options INTERFACE -Wall -Wextra -Wpedantic -Wno-missing-field-initializers
        $<$<BOOL:${KODAK_WARNINGS_AS_ERRORS}>:-Werror>)
endif()

# kodak_core is plain C++ and builds everywhere; the Windows layer and the app
# only on Windows, with MSVC (C++/WinRT).
add_subdirectory(core)

if(WIN32)
    if(NOT MSVC)
        message(FATAL_ERROR "kodak_win and the app need MSVC")
    endif()
    add_subdirectory(win)
    add_subdirectory(app)
endif()

if(KODAK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
