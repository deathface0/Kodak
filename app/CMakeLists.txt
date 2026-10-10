# The executable: overlay window, tray icon and settings.
add_executable(kodak WIN32
    src/App.cpp
    src/Overlay.cpp
    src/main.cpp
    res/kodak.manifest
)

target_link_libraries(kodak PRIVATE kodak::win d2d1 dwrite shcore kodak_build_options)
