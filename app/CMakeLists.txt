# The executable: overlay window, tray icon and settings.
add_executable(kodak WIN32
    src/main.cpp
    res/kodak.manifest
)

target_link_libraries(kodak PRIVATE kodak::win kodak_build_options)
