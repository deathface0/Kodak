# Everything that talks to Windows: capture, OCR, PNG/clipboard output, hotkey,
# tray and single instance.
add_library(kodak_win STATIC
    src/Clipboard.cpp
    src/GdiScreenSource.cpp
    src/GlobalHotkey.cpp
    src/SingleInstance.cpp
    src/TrayIcon.cpp
)
add_library(kodak::win ALIAS kodak_win)

target_include_directories(kodak_win
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(kodak_win
    PUBLIC kodak::core user32 gdi32 shell32
    PRIVATE kodak_build_options)
