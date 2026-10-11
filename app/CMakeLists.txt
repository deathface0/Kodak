# The executable: overlay window, tray icon and settings.
add_executable(kodak WIN32
    src/App.cpp
    src/Overlay.cpp
    src/SettingsDialog.cpp
    src/main.cpp
    res/kodak.manifest
    res/kodak.rc
)

target_link_libraries(kodak PRIVATE kodak::win comctl32 d2d1 dwrite shcore kodak_build_options)
