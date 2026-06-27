# Bangla Keyboard — TSF text service (the IME)

This is the Windows TIP (Text Input Processor) that hosts the reordering engine
from [`../engine/`](../engine/). It implements the SPEC behaviour as a real IME,
so prebase-vowel reordering (`ে`+`ক`→`কে`), reph-after-consonant (`ভার্সন`) and
prebase-through-conjunct (`ক্ষি`) all work in any TSF-aware app (Notepad, Word,
browsers, …).

## Status
**Scaffold — structurally complete, not yet compiled/verified on a Windows SDK box.**
The engine underneath is verified (13/13 corpus; see `../engine/`). The COM/TSF
layer here is standard boilerplate that must be built and tested with MSVC + the
Windows SDK (neither was available on the machine where it was drafted). Spots
needing SDK-time attention are tagged `// VERIFY` in the source.

## Files
| file | role |
|---|---|
| `Globals.h/.cpp` | CLSID, language-profile GUID, display-attr GUID, LANGID (`bn-BD` = 0x0845), module refcount |
| `TextService.h/.cpp` | the TIP COM object: `ITfTextInputProcessorEx` + `ITfThreadMgrEventSink` + `ITfKeyEventSink` + `ITfCompositionSink`; maps engine `Result`s → composition ops via sync read/write edit sessions |
| `Dll.cpp` | `DllMain`, class factory, `DllGetClassObject`, `DllRegisterServer`/`DllUnregisterServer` (COM server + TIP profile + categories) |
| `BanglaKeyboard.def` | exported entry points |
| `CMakeLists.txt` | build the DLL |

## How the engine maps onto TSF
`Engine::process(scanCode, shift)` returns one of four results; the key sink runs
them inside a `TF_ES_SYNC | TF_ES_READWRITE` edit session:

| engine result | TSF action |
|---|---|
| `Compose(t)` | start composition if none, then set the composition string to `t` |
| `Commit(t)` | set string to `t`, then `EndComposition` |
| `CommitThenCompose(f, c)` | set `f`, `EndComposition`, start a new composition, set `c` |
| `Ignore` | not eaten — host handles the key |

Keys are bound by **scan code** (`lParam` bits 16-23), so the layout works under
any base keyboard. `Ctrl/Alt/Win` chords flush the buffer and pass through.
Focus loss (`OnSetFocus`/`ITfThreadMgrEventSink`) and host-terminated compositions
(`OnCompositionTerminated`) flush/reset cleanly.

## Build
Prereqs: **Visual Studio 2022** (or Build Tools) with the **Desktop C++** workload
and the **Windows 10/11 SDK**.

```bat
:: from windows/tsf/, in a "x64 Native Tools Command Prompt for VS"
cmake -B build -A x64
cmake --build build --config Release
:: -> build\Release\BanglaKeyboard.dll
```
Build a 32-bit copy too (`-A Win32`) so the IME also loads in 32-bit apps.

## Register (developer install)
```bat
:: elevated prompt
regsvr32 build\Release\BanglaKeyboard.dll
:: (32-bit DLL: use C:\Windows\SysWOW64\regsvr32.exe on it)
```
Then add it: Settings → Time & language → Language → Bangla → Keyboards → add
**Bangla Keyboard (Unicode)**, or it appears in the language bar / Win+Space.
Unregister: `regsvr32 /u BanglaKeyboard.dll`.

Code-signing is strongly recommended before distributing (SmartScreen / driver-
block policies). See [`../installer/`](../installer/).

## TODO before "done"
- [ ] Compile with MSVC + Windows SDK; resolve any `// VERIFY` spots.
- [ ] **Display attribute (underline) for composing text.** Add an
      `ITfDisplayAttributeProvider` for `c_guidDisplayAttribute`, register the
      `GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER` category (commented out in `Dll.cpp`),
      and set `GUID_PROP_ATTRIBUTE` on the composing range in `SetText` (the
      `// VERIFY` spot in `TextService.cpp`). Composition works without it; this
      is the visual underline only.
- [ ] Test the SPEC §7 corpus live in Notepad + a Unicode app; compare with macOS.
- [ ] Decide on a language-bar icon (`AddLanguageProfile` icon index).
- [ ] Package a signed installer (see `../installer/`).
