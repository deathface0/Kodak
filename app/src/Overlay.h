#pragma once

#include <kodak/core/Finding.h>
#include <kodak/core/Geometry.h>
#include <kodak/core/Image.h>
#include <kodak/win/ScreenSource.h>

#include <windows.h>

#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

#include <cstddef>
#include <functional>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace kodak::app {

/// The full-screen window over the frozen capture. The user drags a selection,
/// then reviews it: sensitive text found by the OCR is proposed for redaction
/// (a click turns a proposal off or on), solid boxes can be drawn by hand, and
/// Enter or a double-click confirms. Esc or switching away cancels.
class Overlay {
public:
    /// Receives the selected pixels, already redacted, once the user confirms.
    using SelectHandler = std::function<void(const core::Image&)>;

    Overlay(HINSTANCE instance, SelectHandler onSelect);
    ~Overlay();

    Overlay(const Overlay&) = delete;
    Overlay& operator=(const Overlay&) = delete;

    /// What the detection looks for: the user's own terms, and the kinds turned off.
    void configureDetection(std::vector<std::u32string> userTerms, std::set<core::FindingKind> disabled);

    /// Shows `capture` over the whole virtual desktop and waits for a selection.
    void show(win::ScreenCapture capture);
    bool visible() const noexcept;

private:
    enum class Mode { Selecting, Reviewing };
    enum class Drag { None, Selection, Box };
    enum class Detection { Idle, Running, Done, Unavailable };

    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handle(UINT message, WPARAM wParam, LPARAM lParam);

    void onButtonDown(core::Point p);
    void onButtonUp(core::Point p);
    void onDoubleClick(core::Point p);
    void onRightClick(core::Point p);
    void removeLastBox();

    /// Looks for sensitive text in the selection on a worker thread.
    void startDetection();
    /// Stops the worker and waits for it, so it no longer touches anything.
    void stopDetection() noexcept;
    void onDetectionDone(WPARAM generation);

    /// A row of the proposals panel, as last painted, and the proposal it shows.
    struct PanelRow {
        D2D1_RECT_F rect;
        std::size_t proposal;
    };

    void paint();
    /// Prepares the text format for the monitor under the selection; 0 if text cannot be drawn.
    float textScale();
    Microsoft::WRL::ComPtr<IDWriteTextLayout> layoutText(const std::wstring& text) const;
    /// The proposals in reading order (top to bottom, left to right).
    std::vector<std::size_t> panelOrder() const;
    /// The list of proposals beside the selection: kind and masked text, click to toggle.
    void paintPanel(float scale);
    void paintHelp(float scale);
    std::wstring helpLine() const;
    std::optional<std::size_t> panelRowAt(core::Point p) const;
    /// Highlights the proposal under the cursor, in the panel or on the image.
    void updateHover(core::Point p);
    bool createDeviceResources();
    void releaseDeviceResources() noexcept;
    core::Point clampToImage(LPARAM lParam) const noexcept;
    void redraw() noexcept;

    /// Confirms now, or as soon as the analysis finishes.
    void requestConfirm();
    /// Crops the selection, paints the boxes and enabled proposals over it and
    /// hands it to onSelect_.
    void confirm();
    /// Hides the window, stops the analysis and wipes the capture from memory.
    void close() noexcept;

    HWND window_ = nullptr;
    SelectHandler onSelect_;
    win::ScreenCapture capture_;

    Mode mode_ = Mode::Selecting;
    Drag drag_ = Drag::None;
    core::Point anchor_; // where the current drag started
    core::Point current_;
    core::Rect selection_;                 // valid while reviewing
    std::vector<core::Finding> boxes_;     // drawn by hand, in capture coordinates
    std::vector<core::Finding> proposals_; // found by the OCR, in capture coordinates
    std::vector<PanelRow> panelRows_;
    std::optional<std::size_t> hovered_; // proposal under the cursor

    std::vector<std::u32string> userTerms_;
    std::set<core::FindingKind> disabledKinds_;
    Detection detection_ = Detection::Idle;
    bool confirmPending_ = false;
    unsigned generation_ = 0; // tells the current analysis from stopped ones
    std::jthread worker_;
    std::mutex resultMutex_;
    std::vector<core::Finding> result_; // written by the worker
    bool resultAvailable_ = true;       // false when no OCR language is installed

    Microsoft::WRL::ComPtr<ID2D1Factory> factory_;
    Microsoft::WRL::ComPtr<ID2D1StrokeStyle> dashed_;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> target_;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_; // recoloured for each shape

    Microsoft::WRL::ComPtr<IDWriteFactory> writeFactory_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> helpFormat_;
    UINT helpDpi_ = 0; // DPI helpFormat_ was sized for
};

} // namespace kodak::app
