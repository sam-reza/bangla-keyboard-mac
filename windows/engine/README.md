# Bangla Keyboard — Windows engine

Two engines live here:

## 1. KLEngine — keylayout-driven (the product engine) ✅
`klengine.h` / `klengine.cpp` run an Apple `.keylayout` deadkey FSM (the same
algorithm as `keylayout_interp.py`) from a generated table. **Both layouts use it**,
so the Windows output is **byte-identical to the macOS build** — no hand-written
rules that can drift.

| file | role |
|---|---|
| `klengine.h/.cpp` | the table-driven engine (append-only) |
| `unicode_table.h` | Bangla Unicode FSM (`bangla::unicode_table::TABLE`) |
| `classic_table.h` | Bangla Classic / legacy-ANSI FSM (`bangla::classic_table::TABLE`) |
| `keylayout_interp.py` | faithful Python interpreter of the `.keylayout` (ground truth) |
| `gen_tables.py` | generates both `*_table.h` from the Mac `.keylayout` files |
| `gen_klengine_test.py` → `klengine_test.cpp` | C++ engine vs interpreter (20/20 Unicode, 12/12 Classic) |
| `demo.cpp` → `bangla-demo.exe` | runnable demo (`--classic`, `--keys "c j f"`) |

It is **append-only**: `process()` returns the text to append (often empty while a
deadkey is pending), `flush()` emits the dangling deadkey on a boundary. No
back-spacing, so injection (in the tray) is robust in every app.

Regenerate after any `.keylayout` change:
`python gen_tables.py && python gen_klengine_test.py`.

### Vowel behaviour (matches macOS exactly)
The fixed layout does **not** auto-convert an isolated matra to an independent
vowel. Independent vowels are typed via the অ deadkey:

| keys | output | |
|---|---|---|
| `f` | া | aa-kar (stays a matra) |
| `Shift+f` | অ | independent a |
| `Shift+f` then `f` | **আ** | অ + া → আ (U+0986) |
| `Shift+f f m d` | আমি | |
| `c j f` | কো | prebase ে reorders, +া → ো |
| `Shift+h f n Shift+a b` | ভার্সন | reph reorders |
| `h f Shift+q Shift+v f` | বাংলা | |

(Classic emits the legacy ANSI encoding, e.g. `h f Shift+q Shift+v f` → `evsjv`,
which renders as বাংলা only in a legacy ANSI Bangla font.)

## 2. engine.* — the old hand-written Unicode engine ⚠️ (IME only)
`engine.h` / `engine.cpp` / `test.cpp` / `verify.py` are the original hand port of
`../../engine/Engine.swift`. It is still used by the **TSF IME** (`../tsf/`) only.
It differs from the macOS `.keylayout` on independent vowels (it auto-converts an
isolated matra, so `f`→আ). The tray/demo use KLEngine instead for exact parity;
the IME will be migrated to KLEngine too. Its own corpus test is `enginetest` (13/13).

## Build / test
```bat
python keylayout_interp.py      :: interpreter sanity (no toolchain)
..\build-all.bat                :: builds klengine-test, enginetest, demo, tray, DLLs
:: or: cmake -B build && cmake --build build && ctest --test-dir build
```
