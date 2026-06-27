# Bangla Keyboard — Windows engine (C++ port)

Pure, OS-independent port of [`../../engine/Engine.swift`](../../engine/Engine.swift),
keyed by **Windows Set-1 scan codes** instead of macOS virtual key codes. No
Windows headers — it builds and unit-tests headless, and is reused unchanged
inside the TSF text service ([`../tsf/`](../tsf/)).

| file | role |
|---|---|
| `engine.h` / `engine.cpp` | the engine (UTF-16 / `std::u16string`) |
| `test.cpp` | headless test over the SPEC §7 corpus (no ICU) |
| `verify.py` | a behaviour-faithful Python mirror of the same algorithm |
| `CMakeLists.txt`, `build-and-test.bat` | build + run the test |

## Status: ✅ algorithm verified — **13/13** SPEC §7 corpus passes
Verified via `verify.py` (Python iterates strings by Unicode scalar, matching the
engine's semantics; Bengali is all BMP). `test.cpp` is the identical logic in
C++; run it once a C++ compiler is available:

```bat
python verify.py                :: passes now, no toolchain needed
build-and-test.bat              :: C++ test (needs MSVC 'cl' or MinGW 'g++')
:: or: cmake -B build && cmake --build build && ctest --test-dir build --output-on-failure
```

## Design notes carried over from the spec
- **UTF-16, BMP-only.** One `char16_t` == one Unicode scalar for every character
  the engine emits, so `endsWithHasanta` (last unit == `U+09CD`) and the
  single-code-point consonant test are exact — no grapheme-cluster trap (SPEC §3).
- **NFC by construction.** `e`-kar+`aa` is combined to the single code point `ো`
  (U+09CB) and `au` to `ৌ` (U+09CC); every other unit is already precomposed, so
  the output needs no NFC pass.

## ⚠️ One deliberate divergence from `Engine.swift`: reph reordering
`Engine.swift` handles reph (`র্`) with a plain `cons += "র্"` (append). That does
**not** reorder, and a faithful port of it **fails** SPEC §7's two "non-negotiable"
reph-after-consonant cases:

```
ভার্সন  keys '^h f n ^a b'  ->  got ভাসর্ন  (reph appended, on the wrong consonant)
কর্ম    keys 'j m ^a'       ->  got কমর্
```

The shipping macOS `.keylayout` — the **verified ground truth** the spec points to
— *does* reorder: in state `ka` (consonant `ক` buffered) the reph key outputs
**`র্ক`** (reph moved to the front). This port follows the `.keylayout` and SPEC
§7: when reph arrives on a *closed* consonant it is prepended
(`cons = "র্" + cons`); the folas `্র`/`্য` genuinely follow and still append.
With that one-line fix the corpus is 13/13.

**This looks like a bug in `Engine.swift` itself** (its reph append never reorders).
Recommend porting the same fix back to `Engine.swift` so all three platforms match;
until then this is a documented, evidence-backed deviation, not a silent one.
