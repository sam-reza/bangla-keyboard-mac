// Bangla Keyboard — reordering engine (Windows port). See engine.h / ../../SPEC.md.
//
// Build note: this file contains UTF-8 source with u"..." (char16_t) literals.
//   MSVC  : compile with /utf-8 so the literals encode as UTF-16 correctly.
//   MinGW : g++ -std=c++17 (UTF-8 source handled natively).
#include "engine.h"

#include <array>
#include <unordered_map>
#include <utility>

namespace bangla {
namespace {

// ----- character categories (UTF-16 literals) ---------------------------------
const Str kHasanta = u"্";                                   // U+09CD

bool in(const Str& u, std::initializer_list<const char16_t*> set) {
    for (const char16_t* s : set) if (u == s) return true;
    return false;
}
bool isPrebase(const Str& u)    { return in(u, {u"ি", u"ে", u"ৈ"}); }
bool isPostbase(const Str& u)   { return in(u, {u"া", u"ী", u"ু", u"ূ", u"ৃ", u"ৗ"}); }
bool isTrailSign(const Str& u)  { return in(u, {u"ং", u"ঃ", u"ঁ"}); }
bool isIndependent(const Str& u){ return in(u, {u"অ", u"আ", u"ই", u"ঈ", u"উ", u"ঊ", u"ঋ", u"এ", u"ঐ", u"ও", u"ঔ"}); }
bool isStandalone(const Str& u) { return in(u, {u"ৎ"}); }    // khanda-ta: never takes a vowel
bool isClusterMod(const Str& u) { return in(u, {u"র্", u"্র", u"্য"}); } // reph + ra-fola + ya-fola

// lone vowel sign -> independent vowel (used at commit time when there is no base)
Str indepOf(const Str& v) {
    static const std::array<std::pair<const char16_t*, const char16_t*>, 10> kIndep = {{
        {u"া", u"আ"}, {u"ি", u"ই"}, {u"ী", u"ঈ"}, {u"ু", u"উ"}, {u"ূ", u"ঊ"},
        {u"ৃ", u"ঋ"}, {u"ে", u"এ"}, {u"ৈ", u"ঐ"}, {u"ো", u"ও"}, {u"ৌ", u"ঔ"},
    }};
    for (const auto& p : kIndep) if (v == p.first) return p.second;
    return v;
}

// ----- keymap: Windows Set-1 scan code -> {unshifted, shift} -------------------
// Physical US-QWERTY positions (SPEC §6). Bound by scan code so the layout works
// under any base keyboard. `·` passthrough keys map to the literal ASCII char.
const std::unordered_map<unsigned, std::pair<Str, Str>>& keymap() {
    static const std::unordered_map<unsigned, std::pair<Str, Str>> m = {
        // digit row
        {0x29, {u"—", u"‚"}},                                    // `
        {0x02, {u"১", u"!"}}, {0x03, {u"২", u"°"}}, {0x04, {u"৩", u"#"}},
        {0x05, {u"৪", u"৳"}}, {0x06, {u"৫", u"%"}}, {0x07, {u"৬", u"^"}},
        {0x08, {u"৭", u"ঁ"}}, {0x09, {u"৮", u"*"}}, {0x0A, {u"৯", u"("}},
        {0x0B, {u"০", u")"}}, {0x0C, {u"-", u"_"}}, {0x0D, {u"=", u"+"}},
        // top letter row
        {0x10, {u"ঙ", u"ং"}}, {0x11, {u"য", u"য়"}}, {0x12, {u"ড", u"ঢ"}},
        {0x13, {u"প", u"ফ"}}, {0x14, {u"ট", u"ঠ"}}, {0x15, {u"চ", u"ছ"}},
        {0x16, {u"জ", u"ঝ"}}, {0x17, {u"হ", u"ঞ"}}, {0x18, {u"গ", u"ঘ"}},
        {0x19, {u"ড়", u"ঢ়"}}, {0x1A, {u"[", u"{"}}, {0x1B, {u"]", u"}"}},
        {0x2B, {u"ৎ", u"ঃ"}},                                    // backslash
        // home row
        {0x1E, {u"ৃ", u"র্"}}, {0x1F, {u"ু", u"ূ"}}, {0x20, {u"ি", u"ী"}},
        {0x21, {u"া", u"অ"}}, {0x22, {u"্", u"।"}}, {0x23, {u"ব", u"ভ"}},
        {0x24, {u"ক", u"খ"}}, {0x25, {u"ত", u"থ"}}, {0x26, {u"দ", u"ধ"}},
        {0x27, {u";", u":"}}, {0x28, {u"'", u"\""}},
        // bottom row
        {0x2C, {u"্র", u"্য"}}, {0x2D, {u"ও", u"ৗ"}}, {0x2E, {u"ে", u"ৈ"}},
        {0x2F, {u"র", u"ল"}}, {0x30, {u"ন", u"ণ"}}, {0x31, {u"স", u"ষ"}},
        {0x32, {u"ম", u"শ"}}, {0x33, {u",", u"<"}}, {0x34, {u".", u">"}},
        {0x35, {u"/", u"?"}},
    };
    return m;
}

constexpr unsigned kScanBackspace = 0x0E;

} // namespace

Str Engine::renderFinal() const {
    if (cons_.empty() && !vowel_.empty()) return indepOf(vowel_) + trail_;
    return render();
}

bool Engine::isConsonant(const Str& u) {
    if (u.size() != 1) return false;
    char16_t v = u[0];
    return (v >= 0x0995 && v <= 0x09B9) || v == 0x09DC || v == 0x09DD || v == 0x09DF;
}

Str Engine::combineVowel(const Str& existing, const Str& add) {
    if (existing == u"ে" && add == u"া") return u"ো"; // e-kar + aa -> o-kar
    if (existing == u"ে" && add == u"ৗ") return u"ৌ"; // e-kar + au-length -> au-kar
    return existing + add;
}

void Engine::popLast() {
    if (!trail_.empty())      trail_.pop_back();
    else if (!vowel_.empty()) vowel_.pop_back();
    else if (!cons_.empty())  cons_.pop_back();
}

Str Engine::flush() { Str s = renderFinal(); reset(); return s; }

bool Engine::wouldHandle(unsigned scanCode) const {
    if (keymap().count(scanCode)) return true;
    return scanCode == kScanBackspace && !empty();
}

Result Engine::process(unsigned scanCode, bool shift) {
    auto it = keymap().find(scanCode);
    if (it == keymap().end()) {
        // Unmapped (function keys, arrows, etc.). Backspace edits the buffer.
        if (scanCode == kScanBackspace && !empty()) {
            popLast();
            return empty() ? Result::commit(u"") : Result::compose(render());
        }
        return Result::ignore();
    }
    const Str& unit = shift ? it->second.second : it->second.first;

    // 1. Hasanta
    if (unit == kHasanta) { cons_ += unit; return Result::compose(render()); }

    // 2. Reph / folas — extend the cluster.
    if (isClusterMod(unit)) {
        // Reph (র্) is typed AFTER its consonant (fixed-layout/Windows habit) but leads
        // the cluster in Unicode, so reorder it to the front of a closed
        // consonant: ক then র্ -> র্ক (macOS .keylayout: state "ka" + reph ->
        // "র্ক"). Folas (্র ্য) genuinely follow, so they append. This corrects
        // Engine.swift's plain append, which fails SPEC §7's ভার্সন / কর্ম.
        if (unit == u"র্" && !cons_.empty() && !endsWithHasanta(cons_)) {
            cons_ = Str(u"র্") + cons_;
        } else {
            cons_ += unit;
        }
        return Result::compose(render());
    }

    // 3. Prebase vowel (ি ে ৈ) — typed BEFORE its consonant; starts a new syllable.
    if (isPrebase(unit)) {
        if (!empty()) {
            Str out = renderFinal(); reset(); vowel_ = unit;
            return Result::commitThenCompose(std::move(out), render());
        }
        vowel_ = unit;
        return Result::compose(render());
    }

    // 4. Postbase matra (া ী ু ূ ৃ ৗ)
    if (isPostbase(unit)) {
        if (cons_.empty() && vowel_.empty() && trail_.empty()) {
            reset();                              // isolated -> independent vowel
            return Result::commit(indepOf(unit));
        }
        vowel_ = combineVowel(vowel_, unit);
        return Result::compose(render());
    }

    // 5. Trailing sign (ং ঃ ঁ)
    if (isTrailSign(unit)) { trail_ += unit; return Result::compose(render()); }

    // 6. Independent vowel / khanda-ta — a complete letter; commit the lot.
    if (isIndependent(unit) || isStandalone(unit)) {
        Str out = renderFinal() + unit; reset();
        return Result::commit(std::move(out));
    }

    // 7. Consonant
    if (isConsonant(unit)) {
        if (cons_.empty()) {                      // base; picks up pending prebase vowel
            cons_ = unit;
            return Result::compose(render());
        }
        if (endsWithHasanta(cons_)) {             // mid-conjunct continues (ি ক ্ ষ -> ক্ষি)
            cons_ += unit;
            return Result::compose(render());
        }
        Str out = renderFinal(); reset(); cons_ = unit; // closed cluster -> new syllable
        return Result::commitThenCompose(std::move(out), render());
    }

    // 8. Digits, punctuation, danda, currency, etc. — flush + insert.
    Str out = renderFinal() + unit; reset();
    return Result::commit(std::move(out));
}

} // namespace bangla
