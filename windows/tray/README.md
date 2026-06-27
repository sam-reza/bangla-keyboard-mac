# Bangla Keyboard — tray switcher (Bijoy-style)

`bangla-tray.exe` is a standalone background app with a **system-tray icon + popup
menu** (Unicode / Bijoy Classic / English) — the same switch-from-the-tray UX as
classic Bijoy, with the **same two layouts the macOS build ships**. It does **not**
need the TSF IME, registration, or admin: just run the .exe.

## The three modes
- **Unicode** (green **অ**) — standard Unicode Bangla, works with any Bangla font.
  Uses the reordering engine ([`../engine/engine.*`](../engine/)).
- **Bijoy Classic** (blue **ক**) — the legacy **SutonnyMJ / Bijoy ASCII** encoding,
  byte-identical to the macOS Classic layout. Run through the Classic FSM
  ([`../engine/classic.*`](../engine/), generated from the Mac `.keylayout`).
  ⚠️ Renders as Bangla **only in the SutonnyMJ font** (or another Bijoy ASCII font)
  — set your document/app font to SutonnyMJ. We don't bundle it (commercial font).
- **English** (red **E**) — passthrough.

## How it works
- Switch with: **left-click the tray icon** (toggles English ⇄ last Bangla mode),
  the **right-click menu**, or **Ctrl+Alt+B**.
- A global low-level keyboard hook (`WH_KEYBOARD_LL`) runs each keystroke through
  the selected engine and injects the result with `SendInput`, so it works in
  **any** app (Notepad, Word, browsers, chat).
  - Unicode reorders (prebase vowel, reph), so it injects by back-spacing the live
    syllable and retyping.
  - Classic is visual-order, so injection is **append-only** (more robust).
- Same fixed (Bijoy-style) key layout as the rest of the project / the Mac build.

## Run it
1. Build (see [`../build-all.bat`](../build-all.bat)) → `../dist/bangla-tray.exe`.
2. Double-click it. A tray icon appears (starts in **English**).
3. Press **Ctrl+Alt+B** (or click the icon) to switch to **Bangla**, then type.
4. Right-click the icon → **Close** to quit.

To start it automatically at login, drop a shortcut to `bangla-tray.exe` in
`shell:startup` (Win+R → `shell:startup`).

## Tray vs IME — which one?
| | tray app (`bangla-tray.exe`) | TSF IME (`../tsf/`) |
|---|---|---|
| Install | none — just run | `regsvr32` (admin), add keyboard |
| Switch | tray icon / Ctrl+Alt+B | Win+Space (Windows switcher) |
| Works in | every app (global hook + inject) | every TSF-aware app (composition) |
| Robustness | edits text by injecting backspaces — can be off in apps with grapheme-cluster backspace, password boxes, or some games | proper IME composition; the "correct" Windows way |
| Feel | exactly like classic Bijoy | like a built-in Windows language |

Both share the **same engine**, so typing behaviour matches. Use the tray app for
the Bijoy-style experience; use the IME for the most robust integration.

## Limitations / TODO
- **Classic needs the SutonnyMJ font** to render (we can't bundle it). Without it
  the text shows as Latin/symbol characters (the raw Bijoy ASCII), same as on Mac.
- Unicode injection (backspace-diff) can misbehave in apps that delete by grapheme cluster,
  in password fields, or in some full-screen games. The TSF IME avoids this.
- x64 only — a global hook works across all apps regardless of their bitness, so no
  separate 32-bit build is needed.
- Unsigned — code-sign before distributing (a keyboard hook + unsigned exe will
  draw SmartScreen / AV attention).
