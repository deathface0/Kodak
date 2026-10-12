# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2026-10-11

Complete rewrite. Kodak now redacts sensitive data before a screenshot leaves the computer.

### Added

- Frozen capture of the whole virtual desktop (every monitor, negative coordinates, per-monitor DPI), selected on a
  Direct2D overlay.
- Review step before anything is copied: solid boxes drawn by hand, removed with a right click, Backspace or Ctrl+Z;
  Enter or a double-click confirms, Esc cancels.
- Detection of sensitive text with the Windows OCR (enlarged 2x, read in overlapping tiles) and validated rules:
  e-mail addresses, card numbers (Luhn), IBANs (mod 97), DNI/NIE (control letter), Spanish phone numbers, IPv4
  addresses, known secret formats (AWS, GitHub, Google, Stripe, JWT, private keys), high-entropy tokens, values after
  labels such as `password:` and the user's own terms; OCR slips inside numbers and words split by the OCR are handled.
- Proposals shown as black boxes with an amber outline and listed beside the selection with a masked preview; a click
  turns one off or on.
- Detection on a worker thread with its own copy of the pixels; Esc stops it, and Enter waits for it before saving.
- Output to the clipboard (bitmap and PNG) and to a new, metadata-free PNG in `Pictures\Kodak`, never overwriting a file.
- Settings window and `%APPDATA%\Kodak\settings.json`: capture shortcut, saving to a file, which data to look for and
  the user's own terms, applied without restarting.
- Notification-area icon and menu, a single instance, warnings when the shortcut is taken or the settings file is broken.
- Own icon and version information; the executable links the C runtime statically.
- CMake build with presets, doctest unit and integration tests, GitHub Actions CI (MSVC, the core on Linux,
  clang-format) and a release workflow that publishes the zipped executable from a version tag.

### Changed

- Default shortcut is Ctrl+Shift+X, registered with `RegisterHotKey` instead of polling the keyboard
  (Ctrl+Shift+S is "Save as" in many applications).

### Removed

- Opening the capture as a temporary HTML page.
- The bundled `stb_image_write` library and the Visual Studio solution.

[2.0.0]: https://github.com/deathface0/Kodak/compare/v1.0...v2.0.0
