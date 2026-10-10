# Kodak

[![CI](https://github.com/deathface0/Kodak/actions/workflows/ci.yml/badge.svg?branch=v2)](https://github.com/deathface0/Kodak/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

Screenshots for Windows that redact sensitive data before they leave the computer. Everything runs locally.

> **Work in progress.** This branch (`v2`) is a full rewrite. The original version is tagged
> [`v1.0`](https://github.com/deathface0/Kodak/tree/v1.0).

## Layout

| Directory | CMake target | Contents |
| --- | --- | --- |
| `core/` | `kodak_core` | Plain C++ (no Windows headers): image and geometry types, detectors, text rules, redaction |
| `win/` | `kodak_win` | Everything that talks to Windows: capture, OCR, PNG and clipboard output, hotkey, tray |
| `app/` | `kodak` | The executable: overlay window, review, settings; `res/` holds the manifest |
| `tests/` | `kodak_core_tests` | doctest unit tests; `tests/core/` also runs on Linux |

Public headers live in `<dir>/include/kodak/<layer>/`, sources in `<dir>/src/`.

## Building

Requires CMake 3.21+ and Visual Studio 2022 (MSVC). On Linux only `kodak_core` and its tests build.

```
cmake --preset msvc
cmake --build --preset msvc-debug
ctest --preset msvc-debug
```

## License

[MIT](LICENSE)
