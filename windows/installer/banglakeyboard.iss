; Bangla Keyboard (Windows) installer — Inno Setup. Produces a branded
; BanglaKeyboard-Setup-<ver>.exe that installs the tray app to Program Files.
; Build (after windows\build-all.bat has produced dist\bangla-tray.exe):
;   iscc banglakeyboard.iss     -> installer\dist\BanglaKeyboard-Setup-<ver>.exe
; Unsigned for now — users click "More info -> Run anyway" on SmartScreen.
;
; Keep the version in sync across THREE places: windows\VERSION, windows\tray\tray.rc,
; and MyAppVersion below.

#define MyAppName "Bangla Keyboard"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "BiswasHost"
#define MyAppExe "bangla-tray.exe"
#define MyAppURL "https://github.com/wpexpertinbd/bangla-keyboard"

[Setup]
AppId={{5BD4FB21-7946-4912-98C0-C178E33747BD}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}/releases
; Per-user install: no admin/UAC, lands in %LocalAppData%\Programs, and the
; "start at sign-in" shortcut + Start Menu entry are correctly per-user. (A tray
; utility doesn't need a system-wide install.)
PrivilegesRequired=lowest
DefaultDirName={autopf}\Bangla Keyboard
DefaultGroupName=Bangla Keyboard
DisableProgramGroupPage=yes
DisableWelcomePage=no
OutputDir=dist
OutputBaseFilename=BanglaKeyboard-Setup-{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
SetupIconFile=..\tray\banglakeyboard.ico
UninstallDisplayIcon={app}\{#MyAppExe}
; The tray app force-closes via taskkill in [Code] before file copy, so don't
; involve the Restart Manager (it would stall on the hidden tray window).
CloseApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
WelcomeLabel1=Welcome to Bangla Keyboard
WelcomeLabel2=A free Bangla keyboard for Windows — type Bangla in any app and switch right from the system tray.%n%n      -   Bangla Unicode  (Ctrl+Alt+V)%n      -   Bangla Classic  (Ctrl+Alt+B)%n      -   Works in every app; all English shortcuts keep working%n%nMIT licensed, free & open-source — by BiswasHost.

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional icons:"; Flags: unchecked
Name: "startup"; Description: "Start Bangla Keyboard automatically when I sign in"; GroupDescription: "Startup:"

[Files]
Source: "..\dist\bangla-tray.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\tray\banglakeyboard.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "USAGE.txt"; DestDir: "{app}"; Flags: ignoreversion isreadme

[Icons]
Name: "{group}\Bangla Keyboard"; Filename: "{app}\{#MyAppExe}"; IconFilename: "{app}\banglakeyboard.ico"
Name: "{group}\Uninstall Bangla Keyboard"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Bangla Keyboard"; Filename: "{app}\{#MyAppExe}"; IconFilename: "{app}\banglakeyboard.ico"; Tasks: desktopicon
Name: "{userstartup}\Bangla Keyboard"; Filename: "{app}\{#MyAppExe}"; Tasks: startup

[Run]
Filename: "{app}\{#MyAppExe}"; Description: "Launch Bangla Keyboard now"; Flags: nowait postinstall skipifsilent

[Code]
// Force-close any running tray instance before install / uninstall so the EXE
// isn't locked (the app hides to the tray, so a window-based close won't do).
procedure KillTray;
var ResultCode: Integer;
begin
  Exec(ExpandConstant('{cmd}'), '/c taskkill /f /im {#MyAppExe}', '',
       SW_HIDE, ewWaitUntilTerminated, ResultCode);
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  KillTray;
  Result := '';
end;

function InitializeUninstall(): Boolean;
begin
  KillTray;
  Result := True;
end;
