# Plain C++: no Windows headers here, so it builds and is tested on Linux too.
add_library(kodak_core STATIC
    src/Image.cpp
)
add_library(kodak::core ALIAS kodak_core)

target_compile_features(kodak_core PUBLIC cxx_std_20)
target_include_directories(kodak_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(kodak_core PRIVATE kodak_build_options)
