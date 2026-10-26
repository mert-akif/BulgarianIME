#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "bgime/Composer.hpp"
#include "bgime/Utf.hpp"

#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr int HOTKEY_TOGGLE = 1;
constexpr int HOTKEY_QUIT = 2;

HHOOK g_keyboardHook = nullptr;
HHOOK g_mouseHook = nullptr;
bool g_enabled = true;
bgime::Composer g_composer{};
std::size_t g_pendingUtf16Units = 0;
HWND g_lastForegroundWindow = nullptr;

std::wstring u32ToUtf16(std::u32string_view input) {
    std::wstring output;
    output.reserve(input.size());

    for (char32_t cp : input) {
        if (cp <= 0xFFFF) {
            output.push_back(static_cast<wchar_t>(cp));
            continue;
        }

        cp -= 0x10000;
        output.push_back(static_cast<wchar_t>(0xD800 + ((cp >> 10) & 0x3FF)));
        output.push_back(static_cast<wchar_t>(0xDC00 + (cp & 0x3FF)));
    }

    return output;
}

std::size_t utf16UnitCount(std::u32string_view input) {
    std::size_t count = 0;
    for (char32_t cp : input) {
        count += cp <= 0xFFFF ? 1 : 2;
    }
    return count;
}

void sendInputs(std::vector<INPUT>& inputs) {
    if (!inputs.empty()) {
        SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
    }
}

void sendBackspaces(std::size_t count) {
    std::vector<INPUT> inputs;
    inputs.reserve(count * 2);

    for (std::size_t i = 0; i < count; ++i) {
        INPUT down{};
        down.type = INPUT_KEYBOARD;
        down.ki.wVk = VK_BACK;

        INPUT up = down;
        up.ki.dwFlags = KEYEVENTF_KEYUP;

        inputs.push_back(down);
        inputs.push_back(up);
    }

    sendInputs(inputs);
}

void sendUnicodeText(std::u32string_view text) {
    const std::wstring utf16 = u32ToUtf16(text);

    std::vector<INPUT> inputs;
    inputs.reserve(utf16.size() * 2);

    for (wchar_t ch : utf16) {
        INPUT down{};
        down.type = INPUT_KEYBOARD;
        down.ki.wScan = static_cast<WORD>(ch);
        down.ki.dwFlags = KEYEVENTF_UNICODE;

        INPUT up = down;
        up.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

        inputs.push_back(down);
        inputs.push_back(up);
    }

    sendInputs(inputs);
}

// Forget the tiny ambiguous suffix, but leave its already-visible Cyrillic
// character exactly where it is. This is what we want for clicks, cursor
// movement, focus changes, separators and toggle-off.
void commitPendingInPlace() {
    g_composer.reset();
    g_pendingUtf16Units = 0;
}

void replacePending(std::u32string_view text, bool remainsPending) {
    if (g_pendingUtf16Units > 0) {
        sendBackspaces(g_pendingUtf16Units);
    }

    if (!text.empty()) {
        sendUnicodeText(text);
    }

    g_pendingUtf16Units = remainsPending ? utf16UnitCount(text) : 0;
}

bool applyComposeResult(const bgime::ComposeResult& result) {
    switch (result.edit) {
        case bgime::EditType::PassThrough:
            return false;

        case bgime::EditType::Append:
            if (!result.text.empty()) {
                sendUnicodeText(result.text);
            }
            g_pendingUtf16Units = result.pending ? utf16UnitCount(result.text) : 0;
            return result.consumeKey;

        case bgime::EditType::ReplacePending:
            replacePending(result.text, result.pending);
            return result.consumeKey;
    }

    return false;
}

bool hasForbiddenModifierDown() {
    // Shift is intentionally allowed; Ctrl/Alt/Win shortcuts are not converted.
    return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0 ||
           (GetAsyncKeyState(VK_MENU) & 0x8000) != 0 ||
           (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 ||
           (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0;
}

HKL foregroundKeyboardLayout() {
    HWND foreground = GetForegroundWindow();
    DWORD threadId = 0;
    if (foreground != nullptr) {
        threadId = GetWindowThreadProcessId(foreground, nullptr);
    }

    return GetKeyboardLayout(threadId);
}

void syncForegroundWindow() {
    HWND current = GetForegroundWindow();
    if (current != g_lastForegroundWindow) {
        commitPendingInPlace();
        g_lastForegroundWindow = current;
    }
}

bool tryGetUnicodeChar(const KBDLLHOOKSTRUCT& event, char32_t& outChar) {
    if (hasForbiddenModifierDown()) {
        return false;
    }

    BYTE state[256]{};
    if (!GetKeyboardState(state)) {
        return false;
    }

    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
        state[VK_SHIFT] |= 0x80;
        state[VK_LSHIFT] |= 0x80;
        state[VK_RSHIFT] |= 0x80;
    }

    if (GetKeyState(VK_CAPITAL) & 0x0001) {
        state[VK_CAPITAL] |= 0x01;
    }

    WCHAR chars[8]{};
    const HKL layout = foregroundKeyboardLayout();
    const int rc = ToUnicodeEx(
        event.vkCode,
        event.scanCode,
        state,
        chars,
        static_cast<int>(std::size(chars)),
        0,
        layout
    );

    if (rc <= 0) {
        return false;
    }

    outChar = static_cast<char32_t>(chars[0]);
    return true;
}

bool isKeyDown(WPARAM wParam) {
    return wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
}

LRESULT passKeyboardToNext(int code, WPARAM wParam, LPARAM lParam) {
    return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
}

LRESULT CALLBACK keyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code < 0) {
        return passKeyboardToNext(code, wParam, lParam);
    }

    const auto* event = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
    if (event == nullptr) {
        return passKeyboardToNext(code, wParam, lParam);
    }

    if ((event->flags & LLKHF_INJECTED) != 0) {
        return passKeyboardToNext(code, wParam, lParam);
    }

    if (!isKeyDown(wParam)) {
        return passKeyboardToNext(code, wParam, lParam);
    }

    syncForegroundWindow();

    if (!g_enabled) {
        commitPendingInPlace();
        return passKeyboardToNext(code, wParam, lParam);
    }

    switch (event->vkCode) {
        case VK_BACK: {
            if (g_composer.empty()) {
                return passKeyboardToNext(code, wParam, lParam);
            }

            const auto result = g_composer.handleBackspace();
            return applyComposeResult(result) ? 1 : passKeyboardToNext(code, wParam, lParam);
        }

        case VK_ESCAPE: {
            if (g_composer.empty()) {
                return passKeyboardToNext(code, wParam, lParam);
            }

            const auto result = g_composer.handleEscape();
            return applyComposeResult(result) ? 1 : passKeyboardToNext(code, wParam, lParam);
        }

        case VK_RETURN:
        case VK_TAB:
        case VK_SPACE:
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
        case VK_HOME:
        case VK_END:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_DELETE:
            commitPendingInPlace();
            return passKeyboardToNext(code, wParam, lParam);

        default:
            break;
    }

    char32_t typed = 0;
    if (!tryGetUnicodeChar(*event, typed)) {
        commitPendingInPlace();
        return passKeyboardToNext(code, wParam, lParam);
    }

    if (!bgime::utf::isComposeChar(typed)) {
        commitPendingInPlace();
        return passKeyboardToNext(code, wParam, lParam);
    }

    const auto result = g_composer.handleChar(typed);
    return applyComposeResult(result) ? 1 : passKeyboardToNext(code, wParam, lParam);
}

bool isMouseCommitEvent(WPARAM wParam) {
    return wParam == WM_LBUTTONDOWN ||
           wParam == WM_RBUTTONDOWN ||
           wParam == WM_MBUTTONDOWN ||
           wParam == WM_XBUTTONDOWN;
}

LRESULT CALLBACK mouseProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0 && isMouseCommitEvent(wParam)) {
        // Crucially: do NOT send Backspace here. The click may move the caret
        // before another event is processed. We simply forget composition state.
        commitPendingInPlace();
    }

    return CallNextHookEx(g_mouseHook, code, wParam, lParam);
}

void showStartupMessage() {
    MessageBoxW(
        nullptr,
        L"BulgarianIME hook mode is running.\n\n"
        L"Normal letters are committed immediately; only ambiguous sequences "
        L"such as s/sh/sht, z/zh, c/ch, t/ts and y/ya/yu are kept temporarily.\n\n"
        L"Mouse clicks now clear composition state.\n\n"
        L"Ctrl + Alt + B : enable / disable\n"
        L"Ctrl + Alt + Q : quit",
        L"BulgarianIME",
        MB_OK | MB_ICONINFORMATION
    );
}

void showToggleMessage() {
    MessageBoxW(
        nullptr,
        g_enabled ? L"BulgarianIME enabled." : L"BulgarianIME disabled.",
        L"BulgarianIME",
        MB_OK | MB_ICONINFORMATION
    );
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    g_lastForegroundWindow = GetForegroundWindow();

    g_keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, keyboardProc, instance, 0);
    if (g_keyboardHook == nullptr) {
        MessageBoxW(nullptr, L"Keyboard hook could not be installed.", L"BulgarianIME", MB_OK | MB_ICONERROR);
        return 1;
    }

    g_mouseHook = SetWindowsHookExW(WH_MOUSE_LL, mouseProc, instance, 0);
    if (g_mouseHook == nullptr) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = nullptr;
        MessageBoxW(nullptr, L"Mouse hook could not be installed.", L"BulgarianIME", MB_OK | MB_ICONERROR);
        return 1;
    }

    RegisterHotKey(nullptr, HOTKEY_TOGGLE, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'B');
    RegisterHotKey(nullptr, HOTKEY_QUIT, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'Q');

    showStartupMessage();

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_HOTKEY) {
            if (msg.wParam == HOTKEY_TOGGLE) {
                commitPendingInPlace();
                g_enabled = !g_enabled;
                showToggleMessage();
                continue;
            }

            if (msg.wParam == HOTKEY_QUIT) {
                break;
            }
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    UnregisterHotKey(nullptr, HOTKEY_TOGGLE);
    UnregisterHotKey(nullptr, HOTKEY_QUIT);

    if (g_mouseHook != nullptr) {
        UnhookWindowsHookEx(g_mouseHook);
        g_mouseHook = nullptr;
    }

    if (g_keyboardHook != nullptr) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = nullptr;
    }

    return 0;
}
