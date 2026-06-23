# Bangla Keyboard Layout for Mac

A modern, free **Bangla keyboard layout installer for macOS** (Apple Silicon &
Intel). It installs the traditional **fixed (positional) Bangla layout** —
typed the same way as on Windows — plus a set of free Unicode Bangla fonts.

> Built and maintained by **[BiswasHost](https://www.biswashost.com)**.
> No Java, no background app — just native macOS keyboard layouts.

<p align="center">
  <img src="assets/icon-preview-unicode.png" width="120" alt="Bangla Unicode">
  &nbsp;&nbsp;
  <img src="assets/icon-preview-classic.png" width="120" alt="Bangla Classic">
</p>

## What you get

| Layout | Output | Use with |
|---|---|---|
| **Bangla Unicode** | Proper Unicode Bangla (U+0980 block) | Any Unicode Bangla font (14 included) |
| **Bangla Classic** | Legacy ASCII encoding for "MJ"-family fonts | **SutonnyMJ** (see note below — not included) |
| **14 free fonts** | SolaimanLipi, Kalpurush, Siyam Rupali, AdorshoLipi, Lohit, Mukti, Akaash, … | installed to `/Library/Fonts` |

### Natural, Windows-style typing
The **Bangla Unicode** layout reproduces the familiar fixed-layout typing order,
including the things people miss on other Mac layouts:

- Type the vowel-sign **before** its consonant (prebase) — it attaches to the
  right letter: `ম ে ন → মনে`, `ত ে ব → তবে`, `ে ক ন → কেন`.
- Conjuncts ride through correctly: `ি ক ্ ষ → ক্ষি`, `হ চ ে ্ ছ → হচ্ছে`.
- Reph after a consonant: `স Shift+A → র্স` (`ভার্সন`).
- ya/ra-fola: `ি ফ ্র → ফ্রি`. Independent vowels: `স ে G ি → সেই`.

## Keyboard layout

<p align="center">
  <img src="assets/keyboard-layout-unicode.png" width="820" alt="Bangla Unicode keyboard layout chart">
</p>

Big white glyph = normal key; small red glyph = **Shift**. `◌` marks a vowel-sign
(matra) or fola. Type the vowel-sign **before** the consonant it belongs to.

## Install

This build is **not code-signed** (no paid Apple Developer ID), so macOS
Gatekeeper will block a plain double-click. Pick whichever is easier:

### ✅ Easiest — the installer package (`.pkg`)
1. From [**Releases**](../../releases) download **`Bangla Keyboard.pkg`**.
2. **Right-click it → Open → Open** (a normal double-click is blocked).
3. Follow the installer and enter your admin password.

### Or — the installer app (adds Reinstall / Uninstall)
1. From [**Releases**](../../releases) download the **`.dmg`** and open it.
2. Try to open **`Bangla Keyboard Installer`** once → macOS blocks it.
3. **System Settings → Privacy & Security**, scroll down, click **"Open Anyway"**,
   then open the app again. It offers **Install / Reinstall / Uninstall**.

### After installing (either way)
1. **Log out and log back in** (or restart) — macOS caches keyboard layouts.
2. **System Settings → Keyboard → Text Input → Edit… → `+` → Bangla** → add
   **Bangla Unicode** and/or **Bangla Classic**.
3. Switch input with the menu-bar flag icon or **Control-Space**.

### "App is damaged and can't be opened"?
That's **Gatekeeper on an unsigned download — not real damage** (common on Apple
Silicon). Easiest fix: **use the `.pkg`** above. If you prefer the app, drag it
out of the disk image to (say) your Desktop, then in **Terminal** run:

```bash
xattr -dr com.apple.quarantine ~/Desktop/"Bangla Keyboard Installer.app"
```

…and open it. (This warning goes away entirely only with a notarized build,
which needs a paid Apple Developer ID.)

## ⚠️ About the Bangla Classic layout (SutonnyMJ)

The **Bangla Classic** layout outputs the **legacy ASCII (non-Unicode)**
encoding that "MJ"-family fonts use. Those fonts are **proprietary and are not
included** in this project. To use the Classic layout you must install a
compatible font such as **SutonnyMJ** **from your own legitimate source**, then
select that font in your app.

The **Bangla Unicode** layout has no such requirement — it works with any of the
bundled Unicode fonts (or system Bangla fonts).

### Typing ও‑কার / এ‑কার / ঐ‑কার in Classic (v1.1.0+)

Type the consonant **first**, then the vowel sign — **no space needed**:

- `স` then `ে` → **সে**, &nbsp; `স` then `ে` then `া` → **সো**, &nbsp; `ত` then `ে` then `া` → **তো**
- This is the natural (Windows‑style) order, and it means **Space is always a clean
  word‑separator** — so words like *আমার সোনার বাংলা* no longer stick together.

> Earlier builds used a leading **Space** to trigger the left‑side vowel; that
> accidentally ate the gap between words. If you used the old space habit, just
> drop the space. **After updating, remove and re‑add the Classic layout, then log
> out/in** — macOS caches keyboard layouts.

## Licensing — please read

- **This installer, the build scripts, the icons, and the keyboard-layout
  modifications** are by BiswasHost and released under the **[MIT License](LICENSE)**.
- **The bundled fonts** are free/libre and stay under **their own** licenses
  (GNU GPL or SIL OFL). Full details and license texts:
  [`fonts-licenses/`](fonts-licenses/) and [`fonts-licenses/FONTS.md`](fonts-licenses/FONTS.md).
- This is an **unofficial, community project** and is **not affiliated with any
  commercial keyboard or font vendor**. See **[DISCLAIMER.md](DISCLAIMER.md)**.
  Fonts not included for licensing reasons: the five **Nikosh** fonts
  (CC BY-NC-ND) and all **MJ**-family fonts (proprietary).

## Build from source

```bash
src/build/build.sh 1.0.0
# → dist/Bangla Keyboard.pkg  and  dist/Bangla Keyboard.dmg
```

Requirements: macOS with the standard command-line tools (`pkgbuild`,
`productbuild`, `hdiutil`). The keyboard layouts live in `src/keylayouts/`, the
fonts in `src/fonts/`, and the icons in `assets/`.

## Notes

- Unsigned build: Gatekeeper will ask you to right-click → Open the first time.
- Works on macOS 11 (Big Sur) and later, Apple Silicon and Intel.
- The last letter of a word appears when you press the next key or space — this
  is normal for fixed-layout reordering.

---

Made with care by **[BiswasHost](https://www.biswashost.com)** 🇧🇩

## ☕ Support

This project is free and open-source. If it made your Bangla typing on the Mac
easier, you can **buy me a coffee** — it genuinely helps me keep building and
maintaining free tools like this. 🙏

- **bKash** (Personal · *Send Money*): **`01710378396`**

ধন্যবাদ! / Thank you!
