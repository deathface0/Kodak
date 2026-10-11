include(FetchContent)

# nlohmann/json is header-only: download just json.hpp for the pinned release
# and verify its hash, like doctest in tests/.
FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
    URL_HASH SHA256=9bea4c8066ef4a1c206b2be5a36302f8926f7fdc6087af5d20b417d0cf103ea6
    DOWNLOAD_NO_EXTRACT TRUE
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/nlohmann_json/nlohmann)
FetchContent_MakeAvailable(nlohmann_json)

# Plain C++: no Windows headers here, so it builds and is tested on Linux too.
add_library(kodak_core STATIC
    src/FileName.cpp
    src/Finding.cpp
    src/Image.cpp
    src/Preview.cpp
    src/Redaction.cpp
    src/Settings.cpp
    src/Shortcut.cpp
    src/TextNormalize.cpp
    src/TextDetector.cpp
    src/TextRules.cpp
    src/TextValidation.cpp
    src/Tiling.cpp
    src/Utf8.cpp
)
add_library(kodak::core ALIAS kodak_core)

target_compile_features(kodak_core PUBLIC cxx_std_20)
target_include_directories(kodak_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Only Settings.cpp uses the JSON library; it does not leak into the public headers.
target_include_directories(kodak_core SYSTEM PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/_deps/nlohmann_json)
target_link_libraries(kodak_core PRIVATE kodak_build_options)
