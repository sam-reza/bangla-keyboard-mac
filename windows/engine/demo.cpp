// Bangla Keyboard — standalone demo (NOT the IME; that's the TSF DLL in ../tsf/).
//
// Proves the engine in a runnable .exe. Uses the same keylayout-driven KLEngine
// as the tray, so behaviour matches exactly (f -> া, Shift+f -> অ, Shift+f f -> আ).
//   bangla-demo.exe                 interactive Bangla Unicode (type, see output)
//   bangla-demo.exe --classic       interactive Bangla Classic (legacy ANSI font)
//   bangla-demo.exe --keys "c j f"  batch: feed a key spec (^ = Shift) and print
//   bangla-demo.exe --classic --keys "j m ^a"
#include "klengine.h"
#include "unicode_table.h"
#include "classic_table.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <cstring>

using bangla::KLEngine;
using bangla::Table;

static HANDLE hOut;

static std::string toUtf8(const std::u16string& s) {
    std::string out;
    for (char16_t c : s) {
        unsigned v = c;
        if (v < 0x80) out += char(v);
        else if (v < 0x800) { out += char(0xC0 | (v >> 6)); out += char(0x80 | (v & 0x3F)); }
        else { out += char(0xE0 | (v >> 12)); out += char(0x80 | ((v >> 6) & 0x3F)); out += char(0x80 | (v & 0x3F)); }
    }
    return out;
}
static void writeW(const std::u16string& s) {
    DWORD n = 0;
    if (GetFileType(hOut) == FILE_TYPE_CHAR && WriteConsoleW(hOut, s.c_str(), (DWORD)s.size(), &n, nullptr)) return;
    std::string u8 = toUtf8(s);
    WriteFile(hOut, u8.data(), (DWORD)u8.size(), &n, nullptr);
}
static void writeA(const char* s) {
    DWORD n = 0, len = (DWORD)std::strlen(s);
    if (GetFileType(hOut) == FILE_TYPE_CHAR && WriteConsoleA(hOut, s, len, &n, nullptr)) return;
    WriteFile(hOut, s, len, &n, nullptr);
}

static unsigned scanOf(char c) {
    switch (c) {
        case '`': return 0x29; case '1': return 0x02; case '2': return 0x03;
        case '3': return 0x04; case '4': return 0x05; case '5': return 0x06;
        case '6': return 0x07; case '7': return 0x08; case '8': return 0x09;
        case '9': return 0x0A; case '0': return 0x0B; case '-': return 0x0C; case '=': return 0x0D;
        case 'q': return 0x10; case 'w': return 0x11; case 'e': return 0x12; case 'r': return 0x13;
        case 't': return 0x14; case 'y': return 0x15; case 'u': return 0x16; case 'i': return 0x17;
        case 'o': return 0x18; case 'p': return 0x19; case '[': return 0x1A; case ']': return 0x1B;
        case '\\': return 0x2B;
        case 'a': return 0x1E; case 's': return 0x1F; case 'd': return 0x20; case 'f': return 0x21;
        case 'g': return 0x22; case 'h': return 0x23; case 'j': return 0x24; case 'k': return 0x25;
        case 'l': return 0x26; case ';': return 0x27; case '\'': return 0x28;
        case 'z': return 0x2C; case 'x': return 0x2D; case 'c': return 0x2E; case 'v': return 0x2F;
        case 'b': return 0x30; case 'n': return 0x31; case 'm': return 0x32; case ',': return 0x33;
        case '.': return 0x34; case '/': return 0x35;
        default:  return 0;
    }
}

static int batch(const Table* tb, const char* spec) {
    KLEngine e(tb);
    std::u16string out; std::string tok;
    auto play = [&](const std::string& t) {
        if (t.empty()) return;
        bool shift = t[0] == '^'; char key = shift ? t[1] : t[0];
        out += e.process(scanOf(key), shift);
    };
    for (const char* p = spec; *p; ++p) { if (*p == ' ') { play(tok); tok.clear(); } else tok += *p; }
    play(tok);
    out += e.flush();
    writeW(out); writeA("\n");
    return 0;
}

static int interactive(const Table* tb, bool classic) {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD oldMode = 0; GetConsoleMode(hIn, &oldMode);
    SetConsoleMode(hIn, ENABLE_WINDOW_INPUT);
    writeA(classic ? "Bangla Keyboard - Classic demo (legacy ANSI font to render)\n"
                   : "Bangla Keyboard - Unicode demo\n");
    writeA("Type on your US-QWERTY keyboard (Shift works). Enter = new line, Esc = quit.\n\n");

    KLEngine e(tb);
    INPUT_RECORD ir; DWORD nread = 0; bool running = true;
    while (running && ReadConsoleInputW(hIn, &ir, 1, &nread) && nread) {
        if (ir.EventType != KEY_EVENT || !ir.Event.KeyEvent.bKeyDown) continue;
        const KEY_EVENT_RECORD& k = ir.Event.KeyEvent;
        WORD vk = k.wVirtualKeyCode; unsigned scan = k.wVirtualScanCode; DWORD ctl = k.dwControlKeyState;
        bool shift = (ctl & SHIFT_PRESSED) != 0;
        bool ctrlAlt = (ctl & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED | LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0;
        if (vk == VK_ESCAPE) { writeW(e.flush()); running = false; break; }
        if (vk == VK_RETURN) { writeW(e.flush()); writeA("\r\n"); continue; }
        if (ctrlAlt) { writeW(e.flush()); continue; }
        if (e.wouldHandle(scan)) writeW(e.process(scan, shift));
        else writeW(e.flush());
    }
    SetConsoleMode(hIn, oldMode);
    writeA("\n");
    return 0;
}

int main(int argc, char** argv) {
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleOutputCP(CP_UTF8);
    bool classic = false; const char* keys = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--classic") == 0) classic = true;
        else if (std::strcmp(argv[i], "--keys") == 0 && i + 1 < argc) keys = argv[++i];
    }
    const Table* tb = classic ? &bangla::classic_table::TABLE : &bangla::unicode_table::TABLE;
    if (keys) return batch(tb, keys);
    return interactive(tb, classic);
}
