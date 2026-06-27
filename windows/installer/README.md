# Bangla Keyboard — Windows installer

> **Status: not started.** Build & verify the TIP ([`../tsf/`](../tsf/)) first.

The TIP is an in-proc COM DLL registered as a TSF text service. Two delivery
options:

## Option A — simple EXE / script (developer & early users)
Ship both DLLs (`BanglaKeyboard.dll` x64 + Win32) plus register on install:
```bat
:: install (elevated)
copy BanglaKeyboard.dll        "%ProgramFiles%\BanglaKeyboard\"
copy BanglaKeyboard32.dll      "%ProgramFiles%\BanglaKeyboard\"
regsvr32 /s "%ProgramFiles%\BanglaKeyboard\BanglaKeyboard.dll"
%SystemRoot%\SysWOW64\regsvr32.exe /s "%ProgramFiles%\BanglaKeyboard\BanglaKeyboard32.dll"
:: uninstall: regsvr32 /u on both, then delete the folder
```
`DllRegisterServer` already adds the COM server, the `bn-BD` language profile, and
the TIP categories (see `../tsf/Dll.cpp`). Wrap the above in an Inno Setup / WiX /
NSIS package for a real installer with an uninstall entry.

## Option B — MSIX (Store / modern deployment)
Package the DLL as a TSF TIP via the MSIX manifest extensions
(`uap:Extension Category="windows.inputMethodEditor"` / TSF registration). MSIX
gives clean install/uninstall and update; it requires the package (and DLL) to be
**code-signed** with a cert trusted on the target machine.

## Code signing
TSF TIPs load into other processes (incl. secure desktops if
`GUID_TFCAT_TIPCAP_SECUREMODE` is kept). Sign both DLLs and the installer:
```bat
signtool sign /fd SHA256 /a /tr http://timestamp.digicert.com /td SHA256 BanglaKeyboard.dll
```
Unsigned binaries trip SmartScreen and may be blocked by enterprise policies.

## Checklist
- [ ] Build Release x64 **and** Win32 DLLs.
- [ ] Code-sign both DLLs.
- [ ] Choose A (EXE/Inno/WiX) or B (MSIX); build the package.
- [ ] Sign the installer; test install → add keyboard → type → uninstall on a
      clean Windows 10 and 11 VM.
