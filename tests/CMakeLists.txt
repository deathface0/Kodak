include(FetchContent)

# doctest is header-only: download just doctest.h for the pinned release and
# verify its hash, instead of cloning the whole repository (whose examples
# exceed MAX_PATH when the build directory is deep).
FetchContent_Declare(doctest
    URL https://raw.githubusercontent.com/doctest/doctest/v2.4.11/doctest/doctest.h
    URL_HASH SHA256=44faa038e9c3f9728efbda143748d01124ea0a27f4bf78f35a15d8fab2e039fb
    DOWNLOAD_NO_EXTRACT TRUE
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/doctest/doctest)
FetchContent_MakeAvailable(doctest)

add_library(kodak_doctest INTERFACE)
target_include_directories(kodak_doctest SYSTEM INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/_deps/doctest)

# kodak_core: runs on Linux and Windows.
add_executable(kodak_core_tests
    main.cpp
    core/test_file_name.cpp
    core/test_finding.cpp
    core/test_geometry.cpp
    core/test_image.cpp
    core/test_redaction.cpp
    core/test_selection.cpp
    core/test_settings.cpp
    core/test_shortcut.cpp
    core/test_text_detector.cpp
    core/test_text_normalize.cpp
    core/test_text_rules.cpp
    core/test_text_validation.cpp
    core/test_tiling.cpp
    core/test_utf8.cpp
)
target_link_libraries(kodak_core_tests PRIVATE kodak::core kodak_doctest kodak_build_options)

add_test(NAME kodak_core_tests COMMAND kodak_core_tests)

# kodak_win: real screen, clipboard, WIC and files, Windows only. Same manifest
# as the app, so captures are in physical pixels. The screen and clipboard tests
# skip themselves when the session has no desktop or clipboard (some CI runners).
if(WIN32)
    add_executable(kodak_win_tests
        win/main.cpp
        win/test_capture.cpp
        win/test_capture_files.cpp
        win/test_clipboard.cpp
        win/test_ocr.cpp
        win/test_png.cpp
        win/test_settings_file.cpp
        win/test_text_conversion.cpp
        ${PROJECT_SOURCE_DIR}/app/res/kodak.manifest
    )
    target_link_libraries(kodak_win_tests PRIVATE kodak::win kodak_doctest kodak_build_options shlwapi)

    add_test(NAME kodak_win_tests COMMAND kodak_win_tests)
endif()
