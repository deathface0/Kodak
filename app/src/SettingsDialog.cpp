#include "SettingsDialog.h"

#include "resource.h"

#include <kodak/win/ShortcutKeys.h>
#include <kodak/win/TextConversion.h>

#include <commctrl.h>

#include <array>
#include <utility>

namespace kodak::app {

namespace {

struct State {
    SettingsDialog::Context context;
    HWND* handle;
};

struct DetectionBox {
    int id;
    core::FindingKind kind;
};

constexpr std::array<DetectionBox, 9> detectionBoxes{{
    {IDC_DETECT_EMAIL, core::FindingKind::Email},
    {IDC_DETECT_CARD, core::FindingKind::Card},
    {IDC_DETECT_IBAN, core::FindingKind::Iban},
    {IDC_DETECT_DNINIE, core::FindingKind::DniNie},
    {IDC_DETECT_PHONE, core::FindingKind::Phone},
    {IDC_DETECT_IP, core::FindingKind::Ip},
    {IDC_DETECT_SECRET, core::FindingKind::Secret},
    {IDC_DETECT_CONTEXT, core::FindingKind::ContextValue},
    {IDC_DETECT_USERTERM, core::FindingKind::UserTerm},
}};

void setChecked(HWND dialog, int id, bool checked) {
    CheckDlgButton(dialog, id, checked ? BST_CHECKED : BST_UNCHECKED);
}

bool isChecked(HWND dialog, int id) { return IsDlgButtonChecked(dialog, id) == BST_CHECKED; }

std::wstring textOf(HWND control) {
    std::wstring text(static_cast<std::size_t>(GetWindowTextLengthW(control)), L'\0');
    GetWindowTextW(control, text.data(), static_cast<int>(text.size()) + 1);
    return text;
}

void initialise(HWND dialog, const SettingsDialog::Context& context) {
    const core::Settings& settings = context.settings;

    // The hotkey control shows Ctrl, Shift and Alt; Win has its own checkbox.
    if (const auto code = win::toHotkey(settings.hotkey)) {
        BYTE flags = 0;
        if (settings.hotkey.ctrl) flags |= HOTKEYF_CONTROL;
        if (settings.hotkey.shift) flags |= HOTKEYF_SHIFT;
        if (settings.hotkey.alt) flags |= HOTKEYF_ALT;
        SendDlgItemMessageW(dialog, IDC_HOTKEY, HKM_SETHOTKEY, MAKEWORD(code->virtualKey, flags), 0);
    }
    setChecked(dialog, IDC_HOTKEY_WIN, settings.hotkey.win);
    setChecked(dialog, IDC_SAVE_PNG, settings.saveToFile);
    for (const DetectionBox& box : detectionBoxes)
        setChecked(dialog, box.id, !settings.disabled.contains(box.kind));
    SetDlgItemTextW(dialog, IDC_TERMS, win::toUtf16(core::joinTerms(settings.userTerms, U"\r\n")).c_str());

    const std::wstring ocr =
        context.ocrLanguage.empty()
            ? L"Text recognition is unavailable: add a language with OCR in Windows Settings."
            : L"Text recognition: " + context.ocrLanguage;
    SetDlgItemTextW(dialog, IDC_OCR_INFO, ocr.c_str());
    if (context.fileHadErrors)
        SetDlgItemTextW(dialog, IDC_FILE_NOTE,
                        L"settings.json had errors; OK replaces it with these settings.");
}

/// The settings in the window, or std::nullopt and `error` if the shortcut is unusable.
std::optional<core::Settings> readSettings(HWND dialog, std::wstring& error) {
    const auto value = static_cast<WORD>(SendDlgItemMessageW(dialog, IDC_HOTKEY, HKM_GETHOTKEY, 0, 0));
    const UINT key = LOBYTE(value);
    const BYTE flags = HIBYTE(value);
    if (key == 0) {
        error = L"Press the keys of the capture shortcut in the Shortcut box.";
        return std::nullopt;
    }

    UINT modifiers = 0;
    if (flags & HOTKEYF_CONTROL) modifiers |= MOD_CONTROL;
    if (flags & HOTKEYF_SHIFT) modifiers |= MOD_SHIFT;
    if (flags & HOTKEYF_ALT) modifiers |= MOD_ALT;
    if (isChecked(dialog, IDC_HOTKEY_WIN)) modifiers |= MOD_WIN;

    const std::optional<core::Shortcut> shortcut = win::fromHotkey(modifiers, key);
    if (!shortcut) {
        error =
            L"That key is not supported. Use a letter, a digit, F1-F24, Print Screen, Pause, Insert, Delete, "
            L"Home, End, Page Up, Page Down or Space.";
        return std::nullopt;
    }
    if (!core::parseShortcut(core::formatShortcut(*shortcut))) {
        error = L"Add Ctrl, Alt, Shift or Win: on its own, that key could no longer be typed.";
        return std::nullopt;
    }

    core::Settings settings;
    settings.hotkey = *shortcut;
    settings.saveToFile = isChecked(dialog, IDC_SAVE_PNG);
    for (const DetectionBox& box : detectionBoxes) {
        if (!isChecked(dialog, box.id)) settings.disabled.insert(box.kind);
    }
    settings.userTerms = core::splitTerms(win::toUtf32(textOf(GetDlgItem(dialog, IDC_TERMS))));
    return settings;
}

INT_PTR CALLBACK dialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<State*>(GetWindowLongPtrW(dialog, DWLP_USER));
    switch (message) {
    case WM_INITDIALOG:
        state = reinterpret_cast<State*>(lParam);
        SetWindowLongPtrW(dialog, DWLP_USER, reinterpret_cast<LONG_PTR>(state));
        *state->handle = dialog;
        initialise(dialog, state->context);
        if (HICON icon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_KODAK))) {
            SendMessageW(dialog, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
            SendMessageW(dialog, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));
        }
        SetForegroundWindow(dialog);
        return TRUE;

    case WM_CTLCOLORSTATIC:
        if (GetDlgCtrlID(reinterpret_cast<HWND>(lParam)) == IDC_ERROR) { // errors in red
            const auto dc = reinterpret_cast<HDC>(wParam);
            SetTextColor(dc, RGB(196, 43, 28));
            SetBkMode(dc, TRANSPARENT);
            return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_BTNFACE));
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK: {
            std::wstring error;
            if (const auto settings = readSettings(dialog, error)) {
                const std::optional<std::wstring> failed = state->context.apply(*settings);
                if (!failed) {
                    EndDialog(dialog, IDOK);
                    return TRUE;
                }
                error = *failed;
            }
            SetDlgItemTextW(dialog, IDC_ERROR, error.c_str());
            MessageBeep(MB_ICONWARNING);
            return TRUE;
        }
        case IDCANCEL: EndDialog(dialog, IDCANCEL); return TRUE;
        }
        break;

    case WM_DESTROY:
        if (state) *state->handle = nullptr;
        break;
    }
    return FALSE;
}

} // namespace

void SettingsDialog::run(HINSTANCE instance, HWND owner, Context context, HWND& handle) {
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_HOTKEY_CLASS};
    InitCommonControlsEx(&controls);

    State state{std::move(context), &handle};
    DialogBoxParamW(instance, MAKEINTRESOURCEW(IDD_SETTINGS), owner, dialogProc,
                    reinterpret_cast<LPARAM>(&state));
    handle = nullptr;
}

} // namespace kodak::app
