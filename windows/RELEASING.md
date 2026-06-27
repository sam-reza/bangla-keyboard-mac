# Releasing the Windows build

Same model as the BHServe repo: **one monorepo** (macOS `macos/` + `engine/`,
Windows `windows/`), separate tag namespaces, Windows ships as a **pre-release** so
it never disturbs the macOS "Latest" release.

## Versioning — keep three files in sync
1. `windows/VERSION`
2. `windows/tray/tray.rc` (`FILEVERSION` / `ProductVersion`)
3. `windows/installer/banglakeyboard.iss` (`#define MyAppVersion`)

## 1. Build everything
On a machine with the toolchain (MinGW x64 + x86 — e.g. w64devkit — or MSVC):
```bat
cd windows
set GXX64=C:\path\w64devkit\bin\g++.exe
set GXX32=C:\path\w64devkit32\bin\g++.exe
build-all.bat              :: -> dist\bangla-tray.exe (+ tests, DLLs)
```
Run `dist\klengine-test.exe` → expect `20/20` + `12/12`.

## 2. Build the installer (Inno Setup 6)
```bat
cd windows\installer
iscc banglakeyboard.iss     :: -> dist\BanglaKeyboard-Setup-<ver>.exe
```
Smoke-test on a clean user: install (per-user, no admin) → tray icon appears →
Ctrl+Alt+V types Bangla → uninstall removes it.

## 3. Publish CODE
```bash
git checkout main && git pull
git checkout -b windows-<topic>
# …work, scoped to windows/ (don't touch macos/ or engine/ unless it's a real
#   cross-platform fix)…
git commit -m "windows: <what> ...Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>"
git push -u origin windows-<topic>      # then PR -> merge to main
```

## 4. Cut the GitHub RELEASE — `win-v<ver>`, **pre-release**
The macOS app's release is tagged `vX.Y.Z` and marked **Latest**. Keep Windows on a
**separate `win-` tag** and **`--prerelease`** so it stays out of "Latest" (so a Mac
updater reading `releases/latest` never sees a Windows build):
```bash
gh release create win-v1.0.0 \
  windows/installer/dist/BanglaKeyboard-Setup-1.0.0.exe \
  --prerelease \
  --title "Bangla Keyboard for Windows 1.0.0" \
  --notes "…what's new… Unsigned — SmartScreen: More info -> Run anyway."
```
Attach only the installer `.exe`.

## When Windows + macOS unify (later)
Once stable, switch to one `vX.Y.Z` release per version carrying every platform's
asset (`.pkg`/`.dmg` for mac, `BanglaKeyboard-Setup-*.exe` for Windows), not a
pre-release — the VS Code / Obsidian model. (Mirror BHServe's `docs/WINDOWS-RELEASE.md`.)
