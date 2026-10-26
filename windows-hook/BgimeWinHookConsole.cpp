#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "bgime/Composer.hpp"
#include "bgime/Utf.hpp"

#include <cstdint>
#include <iostream>
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
        } else {
            cp -= 0x10000;
            output.push_back(static_cast<wchar_t>(0xD800 + ((cp >> 10) & 0x3FF)));
            output.push_back(static_cast<wchar_t>(0xDC00 + (cp & 0x3FF)));
        }
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
    return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0 ||
           (GetAsyncKeyState(VK_MENU) & 0x8000) != 0 ||
           (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 ||
           (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0;
}

HKL foregroundKeyboardLayout() {
    HWND foreground = GetForegroundWindow();
    DWORD threadId = foreground ? GetWindowThreadProcessId(foreground, nullptr) : 0;
    return GetKeyboardLayout(threadId);
}

void syncForegroundWindow() {
    HWND current = GetForegroundWindow();
    if (current != g_lastForegroundWindow) {
        commitPendingInPlace();
        g_lastForegroundWindow = current;
        std::cout << "[state] foreground changed -> pending cleared\n";
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
    const int rc = ToUnicodeEx(
        event.vkCode,
        event.scanCode,
        state,
        chars,
        static_cast<int>(std::size(chars)),
        0,
        foregroundKeyboardLayout()
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

LRESULT CALLBACK keyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code < 0) {
        return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
    }

    const auto* event = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
    if (!event || (event->flags & LLKHF_INJECTED) != 0 || !isKeyDown(wParam)) {
        return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
    }

    syncForegroundWindow();

    if (!g_enabled) {
        commitPendingInPlace();
        return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
    }

    switch (event->vkCode) {
        case VK_BACK: {
            if (g_composer.empty()) {
                return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
            }
            const auto result = g_composer.handleBackspace();
            return applyComposeResult(result) ? 1 : CallNextHookEx(g_keyboardHook, code, wParam, lParam);
        }
        case VK_ESCAPE: {
            if (g_composer.empty()) {
                return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
            }
            const auto result = g_composer.handleEscape();
            return applyComposeResult(result) ? 1 : CallNextHookEx(g_keyboardHook, code, wParam, lParam);
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
            return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
        default:
            break;
    }

    char32_t typed = 0;
    if (!tryGetUnicodeChar(*event, typed) || !bgime::utf::isComposeChar(typed)) {
        commitPendingInPlace();
        return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
    }

    const auto result = g_composer.handleChar(typed);
    return applyComposeResult(result) ? 1 : CallNextHookEx(g_keyboardHook, code, wParam, lParam);
}

bool isMouseCommitEvent(WPARAM wParam) {
    return wParam == WM_LBUTTONDOWN ||
           wParam == WM_RBUTTONDOWN ||
           wParam == WM_MBUTTONDOWN ||
           wParam == WM_XBUTTONDOWN;
}

LRESULT CALLBACK mouseProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0 && isMouseCommitEvent(wParam)) {
        if (!g_composer.empty()) {
            std::cout << "[state] mouse click -> pending cleared\n";
        }
        commitPendingInPlace();
    }
    return CallNextHookEx(g_mouseHook, code, wParam, lParam);
}

} // namespace

int main() {
    SetConsoleOutputCP(CP_UTF8);
    std::cout << "BulgarianIME optimized hook is running.\n";
    std::cout << "Only ambiguous suffixes are buffered (s/sh, z, c, t, y...).\n";
    std::cout << "Mouse clicks clear pending state without deleting visible text.\n";
    std::cout << "Ctrl + Alt + B = enable/disable\n";
    std::cout << "Ctrl + Alt + Q = quit\n\n";

    g_lastForegroundWindow = GetForegroundWindow();

    HINSTANCE instance = GetModuleHandleW(nullptr);
    g_keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, keyboardProc, instance, 0);
    if (g_keyboardHook == nullptr) {
        std::cout << "ERROR: Keyboard hook could not be installed. GetLastError=" << GetLastError() << "\n";
        return 1;
    }

    g_mouseHook = SetWindowsHookExW(WH_MOUSE_LL, mouseProc, instance, 0);
    if (g_mouseHook == nullptr) {
        std::cout << "ERROR: Mouse hook could not be installed. GetLastError=" << GetLastError() << "\n";
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = nullptr;
        return 1;
    }

    RegisterHotKey(nullptr, HOTKEY_TOGGLE, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'B');
    RegisterHotKey(nullptr, HOTKEY_QUIT, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'Q');

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_HOTKEY) {
            if (msg.wParam == HOTKEY_TOGGLE) {
                commitPendingInPlace();
                g_enabled = !g_enabled;
                std::cout << (g_enabled ? "BulgarianIME ENABLED\n" : "BulgarianIME DISABLED\n");
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
