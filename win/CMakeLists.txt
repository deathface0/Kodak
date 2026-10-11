# Everything that talks to Windows: capture, OCR, PNG/clipboard output, hotkey,
# tray and single instance.
add_library(kodak_win STATIC
    src/CaptureFiles.cpp
    src/Clipboard.cpp
    src/GdiScreenSource.cpp
    src/GlobalHotkey.cpp
    src/PngEncoder.cpp
    src/SingleInstance.cpp
    src/TextConversion.cpp
    src/TrayIcon.cpp
    src/WinOcrRecognizer.cpp
)
add_library(kodak::win ALIAS kodak_win)

target_include_directories(kodak_win
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# windowsapp: activation of the Windows Runtime classes used through C++/WinRT (OCR).
target_link_libraries(kodak_win
    PUBLIC kodak::core user32 gdi32 shell32 ole32 windowscodecs windowsapp
    PRIVATE kodak_build_options)
