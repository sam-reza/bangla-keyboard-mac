// Headless test for the Bangla engine — feeds the SPEC §7 corpus and asserts
// output. Mirrors verify.py. No OS / IME plumbing, no ICU.
//
// Build (from windows/engine/):
//   MSVC : cl /utf-8 /EHsc /std:c++17 test.cpp engine.cpp && test.exe
//   MinGW: g++ -std=c++17 -static test.cpp engine.cpp -o test && ./test
//
// NFC note: the engine emits already-composed (NFC) Bengali by construction —
// e-kar+aa is combined to the single code point ো (U+09CB), au to ৌ (U+09CC),
// and every other unit is a single precomposed scalar — so an exact UTF-16
// compare here is equivalent to the spec's "compare NFC-normalized output".
#include "engine.h"

#include <cstdio>
#include <string>
#include <vector>

using bangla::Engine;
using bangla::Result;
using bangla::ResultKind;
using bangla::Str;

// US-QWERTY char -> Windows Set-1 scan code (the §6 physical positions).
static unsigned scanOf(char c) {
    switch (c) {
        case '`': return 0x29; case '1': return 0x02; case '2': return 0x03;
        case '3': return 0x04; case '4': return 0x05; case '5': return 0x06;
        case '6': return 0x07; case '7': return 0x08; case '8': return 0x09;
        case '9': return 0x0A; case '0': return 0x0B; case '-': return 0x0C;
        case '=': return 0x0D;
        case 'q': return 0x10; case 'w': return 0x11; case 'e': return 0x12;
        case 'r': return 0x13; case 't': return 0x14; case 'y': return 0x15;
        case 'u': return 0x16; case 'i': return 0x17; case 'o': return 0x18;
        case 'p': return 0x19; case '[': return 0x1A; case ']': return 0x1B;
        case '\\': return 0x2B;
        case 'a': return 0x1E; case 's': return 0x1F; case 'd': return 0x20;
        case 'f': return 0x21; case 'g': return 0x22; case 'h': return 0x23;
        case 'j': return 0x24; case 'k': return 0x25; case 'l': return 0x26;
        case ';': return 0x27; case '\'': return 0x28;
        case 'z': return 0x2C; case 'x': return 0x2D; case 'c': return 0x2E;
        case 'v': return 0x2F; case 'b': return 0x30; case 'n': return 0x31;
        case 'm': return 0x32; case ',': return 0x33; case '.': return 0x34;
        case '/': return 0x35;
        default:  return 0;
    }
}

// Feed a compact key spec: space-separated tokens, '^' prefix = Shift.
// e.g. "c j" or "^h f n ^a b". Returns committed text + final flush.
static Str typeKeys(const std::string& spec) {
    Engine e;
    Str committed;
    std::string tok;
    auto play = [&](const std::string& t) {
        if (t.empty()) return;
        bool shift = t[0] == '^';
        char key = shift ? t[1] : t[0];
        Result r = e.process(scanOf(key), shift);
        switch (r.kind) {
            case ResultKind::Ignore: break;
            case ResultKind::Compose: break;                  // composing is transient
            case ResultKind::Commit: committed += r.text; break;
            case ResultKind::CommitThenCompose: committed += r.text; break;
        }
    };
    for (char ch : spec) {
        if (ch == ' ') { play(tok); tok.clear(); }
        else tok += ch;
    }
    play(tok);
    committed += e.flush();
    return committed;
}

static std::string toUtf8(const Str& s) { // BMP-only, enough for diagnostics
    std::string out;
    for (char16_t c : s) {
        unsigned v = c;
        if (v < 0x80) out += char(v);
        else if (v < 0x800) { out += char(0xC0 | (v >> 6)); out += char(0x80 | (v & 0x3F)); }
        else { out += char(0xE0 | (v >> 12)); out += char(0x80 | ((v >> 6) & 0x3F)); out += char(0x80 | (v & 0x3F)); }
    }
    return out;
}

static std::string codepoints(const Str& s) {
    std::string out; char buf[16];
    for (char16_t c : s) { std::snprintf(buf, sizeof buf, "U+%04X ", unsigned(c)); out += buf; }
    return out;
}

struct Case { const char* word; const char* spec; const char16_t* expected; };

int main() {
    const std::vector<Case> corpus = {
        {"কে",     "c j",         u"কে"},
        {"কি",     "d j",         u"কি"},
        {"কো",     "c j f",       u"কো"},
        {"কৈ",     "^c j",        u"কৈ"},
        {"ক্ষি",   "d j g ^n",    u"ক্ষি"},
        {"হচ্ছে",  "i c y g ^y",  u"হচ্ছে"},
        {"ভার্সন", "^h f n ^a b", u"ভার্সন"},
        {"কর্ম",   "j m ^a",      u"কর্ম"},
        {"ফ্রি",   "d ^r z",      u"ফ্রি"},
        {"বাংলা",  "h f ^q ^v f", u"বাংলা"},
        {"আমি",    "f d m",       u"আমি"},
        {"ভাই",    "^h f d",      u"ভাই"},
        {"বল",     "h ^v",        u"বল"},
    };

    int passed = 0;
    for (const auto& c : corpus) {
        Str got = typeKeys(c.spec);
        Str exp = c.expected;
        bool ok = got == exp;
        passed += ok;
        std::printf("[%s] %-8s keys='%s'\n", ok ? "PASS" : "FAIL", c.word, c.spec);
        if (!ok) {
            std::printf("        got     : %s  (%s)\n", toUtf8(got).c_str(), codepoints(got).c_str());
            std::printf("        expected: %s  (%s)\n", toUtf8(exp).c_str(), codepoints(exp).c_str());
        }
    }
    std::printf("\n%d/%d passed\n", passed, int(corpus.size()));
    return passed == int(corpus.size()) ? 0 : 1;
}
