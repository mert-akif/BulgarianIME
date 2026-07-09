#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "bgime/Composer.hpp"
#include "bgime/Utf.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr int HOTKEY_TOGGLE = 1;
constexpr int HOTKEY_QUIT = 2;

HHOOK g_keyboardHook = nullptr;
bool g_enabled = true;
bgime::Composer g_composer{};
std::size_t g_preeditUtf16Units = 0;

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

void replacePreedit(std::u32string_view newPreedit) {
    if (g_preeditUtf16Units > 0) {
        sendBackspaces(g_preeditUtf16Units);
    }

    if (!newPreedit.empty()) {
        sendUnicodeText(newPreedit);
    }

    g_preeditUtf16Units = utf16UnitCount(newPreedit);
}

void commitCurrentPreeditInPlace() {
    g_composer.reset();
    g_preeditUtf16Units = 0;
}

void cancelCurrentPreedit() {
    if (g_preeditUtf16Units > 0) {
        sendBackspaces(g_preeditUtf16Units);
    }

    g_composer.reset();
    g_preeditUtf16Units = 0;
}

bool hasForbiddenModifierDown() {
    // Do not convert shortcut combinations. Shift is intentionally allowed.
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

bool tryGetUnicodeChar(const KBDLLHOOKSTRUCT& event, char32_t& outChar) {
    if (hasForbiddenModifierDown()) {
        return false;
    }

    BYTE state[256]{};
    if (!GetKeyboardState(state)) {
        return false;
    }

    // GetKeyboardState may lag behind inside a low-level hook.
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

LRESULT passToNext(int code, WPARAM wParam, LPARAM lParam) {
    return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
}

LRESULT CALLBACK keyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code < 0) {
        return passToNext(code, wParam, lParam);
    }

    const auto* event = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
    if (event == nullptr) {
        return passToNext(code, wParam, lParam);
    }

    // Never process our own SendInput events.
    if ((event->flags & LLKHF_INJECTED) != 0) {
        return passToNext(code, wParam, lParam);
    }

    if (!isKeyDown(wParam)) {
        return passToNext(code, wParam, lParam);
    }

    if (!g_enabled) {
        commitCurrentPreeditInPlace();
        return passToNext(code, wParam, lParam);
    }

    switch (event->vkCode) {
        case VK_BACK: {
            if (g_preeditUtf16Units == 0 && g_composer.empty()) {
                return passToNext(code, wParam, lParam);
            }

            const auto result = g_composer.handleBackspace();
            replacePreedit(result.preedit);
            return 1;
        }

        case VK_ESCAPE: {
            if (g_preeditUtf16Units == 0 && g_composer.empty()) {
                return passToNext(code, wParam, lParam);
            }

            cancelCurrentPreedit();
            return 1;
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
        case VK_DELETE: {
            commitCurrentPreeditInPlace();
            return passToNext(code, wParam, lParam);
        }

        default:
            break;
    }

    char32_t typed = 0;
    if (!tryGetUnicodeChar(*event, typed)) {
        commitCurrentPreeditInPlace();
        return passToNext(code, wParam, lParam);
    }

    if (!bgime::utf::isComposeChar(typed)) {
        commitCurrentPreeditInPlace();
        return passToNext(code, wParam, lParam);
    }

    const auto result = g_composer.handleChar(typed);
    replacePreedit(result.preedit);
    return 1;
}

} // namespace

int main() {
    SetConsoleOutputCP(CP_UTF8);
    std::cout << "BulgarianIME Debug Console Hook is running.\n";
    std::cout << "Open Notepad or any text field and type: zdravey az sım sh ch zh sht\n";
    std::cout << "Ctrl + Alt + B = enable/disable\n";
    std::cout << "Ctrl + Alt + Q = quit\n\n";

    g_keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, keyboardProc, GetModuleHandleW(nullptr), 0);
    if (g_keyboardHook == nullptr) {
        std::cout << "ERROR: Keyboard hook could not be installed. GetLastError=" << GetLastError() << "\n";
        std::cout << "Try running the exe normally first. If the target app is admin, run this as admin too.\n";
        std::cout << "Press Enter to close...";
        std::cin.get();
        return 1;
    }

    RegisterHotKey(nullptr, HOTKEY_TOGGLE, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'B');
    RegisterHotKey(nullptr, HOTKEY_QUIT, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'Q');

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_HOTKEY) {
            if (msg.wParam == HOTKEY_TOGGLE) {
                commitCurrentPreeditInPlace();
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

    if (g_keyboardHook != nullptr) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = nullptr;
    }

    return 0;
}
