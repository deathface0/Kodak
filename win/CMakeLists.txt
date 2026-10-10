# Everything that talks to Windows: capture, OCR, PNG/clipboard output, hotkey,
# tray and single instance.
add_library(kodak_win STATIC
    src/SingleInstance.cpp
)
add_library(kodak::win ALIAS kodak_win)

target_include_directories(kodak_win PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(kodak_win
    PUBLIC kodak::core
    PRIVATE kodak_build_options)
