# The executable: overlay window, tray icon and settings.
configure_file(res/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h @ONLY)

add_executable(kodak WIN32
    src/App.cpp
    src/Overlay.cpp
    src/SettingsDialog.cpp
    src/main.cpp
    res/kodak.manifest
    res/kodak.rc
)

# version.h is generated into the build tree; kodak.rc and the code include it.
target_include_directories(kodak PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(kodak PRIVATE kodak::win comctl32 d2d1 dwrite shcore kodak_build_options)
