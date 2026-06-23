# Security

## Reporting

Found a security issue? Please open a GitHub issue (or contact BiswasHost at
<https://www.biswashost.com>). We'll respond promptly.

## Security posture

This project is a macOS keyboard‑layout installer. It has a deliberately small
attack surface:

- **No network access, no telemetry, no analytics, no auto‑update.**
- **No credentials handled or stored.**
- The only privileged actions are: installing two `.keylayout` files + free
  fonts into `/Library`, and (optionally) removing them again.

### Code that runs with elevated privileges

| Component | Runs as | Notes |
|---|---|---|
| `src/build/scripts/preinstall` | root (during `.pkg` install) | Only `rm -f` of **this project's own** hard‑coded layout paths. No dynamic input. |
| `src/build/scripts/postinstall` | root (during `.pkg` install) | Only absolute‑path `atsutil` calls to refresh the font cache. No dynamic input. |
| `Bangla Keyboard Installer.app` (`src/build/installer.applescript`) | admin (via `osascript … with administrator privileges`) | Install/Reinstall/Uninstall. All filesystem paths are passed to AppleScript as **arguments** and shell‑escaped with `quoted form of` — see below. |

### Injection‑safe privileged execution

The installer app never interpolates a path directly into a privileged shell
string. Paths are passed as `osascript` arguments and quoted by AppleScript
itself:

```applescript
on run argv
    do shell script "/usr/sbin/installer -pkg " & quoted form of (item 1 of argv) & " -target /" with administrator privileges
end run
```

This means a path containing quotes, spaces, or shell metacharacters cannot
break out of the command (verified with a `'; touch …; '` payload during
development). The uninstall path uses the same pattern for every file it removes.

### Data files

- The `.keylayout` files are XML consumed by macOS's text‑input engine. They
  contain only the standard system DTD reference (no external entities / XXE)
  and key→character mappings. The low‑byte `&#x00..;` outputs in **Bangla
  Classic** are legacy font glyph codes — data, never executed.
- Fonts are free/libre TrueType files from the projects listed in
  `fonts-licenses/FONTS.md`.

## Known / accepted items

- **Unsigned distribution.** The `.pkg` and `.command` are not code‑signed or
  notarized, so Gatekeeper shows an "unidentified developer" warning and users
  must right‑click → **Open** the first time. If you build a signed/notarized
  copy with an Apple Developer ID, that warning goes away. This is a trust/UX
  item, not a code vulnerability.
- Because installation writes to `/Library`, an admin password is required —
  this is expected for system‑wide keyboard layouts and fonts.
