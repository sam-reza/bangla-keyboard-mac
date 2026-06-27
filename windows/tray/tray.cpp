// Bangla Keyboard — tray switcher (standalone, no IME registration).
//
// A background app with a system-tray icon + popup menu (Bangla Unicode /
// Bangla Classic / English). A global low-level keyboard hook runs each keystroke
// through the shared engines and injects the result with SendInput, so it types
// Bangla in ANY app without registering a TSF IME or admin.
//
//   Bangla Unicode  — standard Unicode Bangla (../engine/engine.*). Reorders a
//                     prebase vowel / reph, so it injects by back-spacing the live
//                     syllable and retyping (one backspace == one code point).
//   Bangla Classic  — legacy ANSI Bangla encoding (../engine/classic.*), matching
//                     the macOS Classic layout. Visual order, so injection is
//                     append-only (robust). Renders as Bangla only in a legacy
//                     ANSI Bangla font (the one used for old documents).
//   English         — passthrough.
//
// Switch with the tray menu, a left-click on the icon, or Ctrl+Alt+B (toggles
// English <-> the last Bangla mode).
//
// Build: g++ -std=c++17 -O2 -static -mwindows -municode -finput-charset=UTF-8 \
//        tray.cpp ../engine/engine.cpp ../engine/classic.cpp -o ../dist/bangla-tray.exe \
//        -lgdi32 -luser32 -lshell32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <vector>
#include "../engine/engine.h"
#include "../engine/classic.h"

using bangla::Engine;
using bangla::ClassicEngine;
using bangla::Result;
using bangla::ResultKind;
using bangla::Str;

enum Mode { MODE_ENGLISH = 0, MODE_UNICODE = 1, MODE_CLASSIC = 2 };

// ---- state -----------------------------------------------------------------
static HINSTANCE       g_hInst;
static HWND            g_hWnd;
static HHOOK           g_hook;
static Engine          g_uni;              // Unicode engine
static ClassicEngine   g_classic;          // Classic engine
static Str             g_shown;            // Unicode mode: current on-screen syllable
static Mode            g_mode = MODE_ENGLISH;
static Mode            g_lastBangla = MODE_UNICODE;  // what Ctrl+Alt+B / click returns to
static bool            g_classicHintShown = false;
static NOTIFYICONDATAW g_nid    = {};
static HICON           g_icoUni = nullptr; // অ on flag-red circle
static HICON           g_icoCls = nullptr; // ক on brick-red circle
static HICON           g_icoEn  = nullptr; // E on grey circle

#define WM_TRAY     (WM_APP + 1)
#define ID_UNICODE  1001
#define ID_CLASSIC  1002
#define ID_ENGLISH  1003
#define ID_ABOUT    1010
#define ID_EXIT     1011
#define HOTKEY_TOGGLE 1

// ---- key injection ---------------------------------------------------------
static void sendBackspaces(int n) {
    if (n <= 0) return;
    std::vector<INPUT> in; in.reserve(n * 2);
    for (int i = 0; i < n; ++i) {
        INPUT d = {}; d.type = INPUT_KEYBOARD; d.ki.wVk = VK_BACK; in.push_back(d);
        INPUT u = {}; u.type = INPUT_KEYBOARD; u.ki.wVk = VK_BACK; u.ki.dwFlags = KEYEVENTF_KEYUP; in.push_back(u);
    }
    SendInput((UINT)in.size(), in.data(), sizeof(INPUT));
}
static void sendUnicode(const Str& s) {
    if (s.empty()) return;
    std::vector<INPUT> in; in.reserve(s.size() * 2);
    for (char16_t c : s) {
        INPUT d = {}; d.type = INPUT_KEYBOARD; d.ki.wScan = c; d.ki.dwFlags = KEYEVENTF_UNICODE; in.push_back(d);
        INPUT u = {}; u.type = INPUT_KEYBOARD; u.ki.wScan = c; u.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP; in.push_back(u);
    }
    SendInput((UINT)in.size(), in.data(), sizeof(INPUT));
}

// Unicode mode: realise one engine Result by editing the on-screen syllable.
static void realizeUni(const Result& r) {
    switch (r.kind) {
        case ResultKind::Ignore: return;
        case ResultKind::Compose:
            sendBackspaces((int)g_shown.size()); sendUnicode(r.text); g_shown = r.text; break;
        case ResultKind::Commit:
            sendBackspaces((int)g_shown.size()); sendUnicode(r.text); g_shown.clear(); break;
        case ResultKind::CommitThenCompose:
            sendBackspaces((int)g_shown.size()); sendUnicode(r.text); sendUnicode(r.comp); g_shown = r.comp; break;
    }
}

// Finalise whatever is pending in the current mode (space / Enter / chord / focus).
static void flushCurrent() {
    if (g_mode == MODE_UNICODE) {
        Str fin = g_uni.flush();
        if (fin != g_shown) { sendBackspaces((int)g_shown.size()); sendUnicode(fin); }
        g_shown.clear();
    } else if (g_mode == MODE_CLASSIC) {
        sendUnicode(g_classic.flush());   // append-only; just emit the dangling glyph
    }
}

// ---- the global keyboard hook ----------------------------------------------
static LRESULT CALLBACK hookProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION && g_mode != MODE_ENGLISH) {
        auto* k = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        bool injected = (k->flags & LLKHF_INJECTED) != 0;     // skip our own SendInput
        if (!injected && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
            auto down = [](int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; };
            bool ctrl  = down(VK_CONTROL), alt = down(VK_MENU);
            bool win   = down(VK_LWIN) || down(VK_RWIN);
            bool shift = down(VK_SHIFT);
            unsigned scan = k->scanCode;

            if (ctrl || alt || win) {
                flushCurrent();                               // let the shortcut through
            } else if (g_mode == MODE_UNICODE) {
                if (g_uni.wouldHandle(scan)) { realizeUni(g_uni.process(scan, shift)); return 1; }
                flushCurrent();
            } else { // MODE_CLASSIC
                if (g_classic.wouldHandle(scan)) { sendUnicode(g_classic.process(scan, shift)); return 1; }
                flushCurrent();
            }
        }
    }
    return CallNextHookEx(g_hook, code, wParam, lParam);
}

// ---- tray icon / menu ------------------------------------------------------
// Flag-style icon: green square + colored circle + white Bangla letter (matches
// the macOS icons). The circle colour tells the modes apart.
static HICON makeIcon(const wchar_t* txt, COLORREF circle) {
    const int sz = 32;
    HDC sdc = GetDC(nullptr);
    HDC dc  = CreateCompatibleDC(sdc);
    HBITMAP color = CreateCompatibleBitmap(sdc, sz, sz);
    HBITMAP mask  = CreateBitmap(sz, sz, 1, 1, nullptr);   // all-0 = fully opaque
    HGDIOBJ ob = SelectObject(dc, color);
    RECT r = {0, 0, sz, sz};
    HBRUSH green = CreateSolidBrush(RGB(0, 106, 78));      // Bangladesh-flag green
    FillRect(dc, &r, green); DeleteObject(green);
    HBRUSH cb = CreateSolidBrush(circle);                  // red / brick / grey circle
    HGDIOBJ ob2 = SelectObject(dc, cb);
    HGDIOBJ op  = SelectObject(dc, GetStockObject(NULL_PEN));
    Ellipse(dc, 3, 3, sz - 3, sz - 3);
    SelectObject(dc, op); SelectObject(dc, ob2); DeleteObject(cb);
    SetBkMode(dc, TRANSPARENT); SetTextColor(dc, RGB(255, 255, 255));
    HFONT f = CreateFontW(22, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                          DEFAULT_PITCH, L"Nirmala UI");
    HGDIOBJ of = SelectObject(dc, f);
    DrawTextW(dc, txt, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, of); DeleteObject(f);
    SelectObject(dc, ob);
    ICONINFO ii = {}; ii.fIcon = TRUE; ii.hbmColor = color; ii.hbmMask = mask;
    HICON ic = CreateIconIndirect(&ii);
    DeleteObject(color); DeleteObject(mask); DeleteDC(dc); ReleaseDC(nullptr, sdc);
    return ic;
}

static const wchar_t* modeTip() {
    switch (g_mode) {
        case MODE_UNICODE: return L"Bangla Keyboard — Bangla Unicode  (Ctrl+Alt+B = English)";
        case MODE_CLASSIC: return L"Bangla Keyboard — Bangla Classic  (Ctrl+Alt+B = English)";
        default:           return L"Bangla Keyboard — English  (Ctrl+Alt+B = Bangla)";
    }
}

static void updateTray() {
    g_nid.hIcon = (g_mode == MODE_UNICODE) ? g_icoUni : (g_mode == MODE_CLASSIC) ? g_icoCls : g_icoEn;
    lstrcpynW(g_nid.szTip, modeTip(), ARRAYSIZE(g_nid.szTip));
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

static void balloon(const wchar_t* title, const wchar_t* text) {
    g_nid.uFlags |= NIF_INFO;
    lstrcpynW(g_nid.szInfoTitle, title, ARRAYSIZE(g_nid.szInfoTitle));
    lstrcpynW(g_nid.szInfo, text, ARRAYSIZE(g_nid.szInfo));
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
    g_nid.uFlags &= ~NIF_INFO;
}

static void setMode(Mode m) {
    if (g_mode == m) return;
    flushCurrent();                 // finalise anything pending in the old mode
    g_uni.reset(); g_classic.reset(); g_shown.clear();
    g_mode = m;
    if (m == MODE_UNICODE || m == MODE_CLASSIC) g_lastBangla = m;
    updateTray();
    if (m == MODE_CLASSIC && !g_classicHintShown) {
        g_classicHintShown = true;
        balloon(L"Bangla Classic mode",
                L"Set your app/document font to a legacy ANSI Bangla font to see Bangla.");
    }
}
static void toggleEnglish() { setMode(g_mode == MODE_ENGLISH ? g_lastBangla : MODE_ENGLISH); }

static void showMenu() {
    POINT pt; GetCursorPos(&pt);
    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING | (g_mode == MODE_UNICODE ? MF_CHECKED : 0), ID_UNICODE, L"Bangla Unicode");
    AppendMenuW(m, MF_STRING | (g_mode == MODE_CLASSIC ? MF_CHECKED : 0), ID_CLASSIC, L"Bangla Classic");
    AppendMenuW(m, MF_STRING | (g_mode == MODE_ENGLISH ? MF_CHECKED : 0), ID_ENGLISH, L"English");
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m, MF_STRING, ID_ABOUT, L"About");
    AppendMenuW(m, MF_STRING, ID_EXIT, L"Close");
    SetForegroundWindow(g_hWnd);
    TrackPopupMenu(m, TPM_RIGHTBUTTON, pt.x, pt.y, 0, g_hWnd, nullptr);
    DestroyMenu(m);
}

static LRESULT CALLBACK wndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_TRAY:
            if (LOWORD(lp) == WM_RBUTTONUP || LOWORD(lp) == WM_CONTEXTMENU) showMenu();
            else if (LOWORD(lp) == WM_LBUTTONUP) toggleEnglish();
            return 0;
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case ID_UNICODE: setMode(MODE_UNICODE); break;
                case ID_CLASSIC: setMode(MODE_CLASSIC); break;
                case ID_ENGLISH: setMode(MODE_ENGLISH); break;
                case ID_ABOUT:
                    MessageBoxW(h,
                        L"Bangla Keyboard — tray switcher\n\n"
                        L"Bangla Unicode = standard Unicode Bangla (any Bangla font).\n"
                        L"Bangla Classic = legacy ANSI Bangla encoding (needs a legacy ANSI Bangla font).\n"
                        L"English = normal typing.\n\n"
                        L"Switch: click the tray icon, this menu, or Ctrl+Alt+B.\n"
                        L"Same fixed layout as the macOS build.\n"
                        L"MIT licensed.",
                        L"About Bangla Keyboard", MB_OK | MB_ICONINFORMATION);
                    break;
                case ID_EXIT: DestroyWindow(h); break;
            }
            return 0;
        case WM_HOTKEY:
            if (wp == HOTKEY_TOGGLE) toggleEnglish();
            return 0;
        case WM_DESTROY:
            Shell_NotifyIconW(NIM_DELETE, &g_nid);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(h, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int) {
    g_hInst = hInst;

    HANDLE once = CreateMutexW(nullptr, TRUE, L"BanglaKeyboardTraySingleton");
    if (once && GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(nullptr, L"Bangla Keyboard is already running (see the tray).",
                    L"Bangla Keyboard", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    WNDCLASSW wc = {};
    wc.lpfnWndProc = wndProc; wc.hInstance = hInst; wc.lpszClassName = L"BanglaKeyboardTray";
    RegisterClassW(&wc);
    g_hWnd = CreateWindowW(wc.lpszClassName, L"Bangla Keyboard", 0, 0, 0, 0, 0,
                           HWND_MESSAGE, nullptr, hInst, nullptr);

    g_icoUni = makeIcon(L"অ", RGB(244, 42, 65));   // flag red
    g_icoCls = makeIcon(L"ক", RGB(192, 57, 43));   // brick red
    g_icoEn  = makeIcon(L"E", RGB(127, 140, 141)); // grey

    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd   = g_hWnd;
    g_nid.uID    = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAY;
    g_nid.hIcon  = g_icoEn;
    lstrcpynW(g_nid.szTip, modeTip(), ARRAYSIZE(g_nid.szTip));
    Shell_NotifyIconW(NIM_ADD, &g_nid);

    g_hook = SetWindowsHookExW(WH_KEYBOARD_LL, hookProc, hInst, 0);
    RegisterHotKey(g_hWnd, HOTKEY_TOGGLE, MOD_CONTROL | MOD_ALT, 'B');

    balloon(L"Bangla Keyboard", L"In the tray. Click the icon or press Ctrl+Alt+B to type Bangla. Right-click for Bangla Unicode / Bangla Classic.");

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }

    if (g_hook) UnhookWindowsHookEx(g_hook);
    UnregisterHotKey(g_hWnd, HOTKEY_TOGGLE);
    if (g_icoUni) DestroyIcon(g_icoUni);
    if (g_icoCls) DestroyIcon(g_icoCls);
    if (g_icoEn)  DestroyIcon(g_icoEn);
    if (once) { ReleaseMutex(once); CloseHandle(once); }
    return 0;
}
