#ifndef PayloadDir
  #error PayloadDir is required
#endif
#ifndef ArtifactDir
  #error ArtifactDir is required
#endif
#define AppVersion "0.1.0"

[Setup]
AppId={{DDAC0B25-B344-44A9-9A8A-5C08023A45AF}
AppName=TerminalEL
AppVersion={#AppVersion}
AppPublisher=TerminalEL contributors
AppPublisherURL=https://github.com/HakureiRaymoo/terminalEL
DefaultDirName={localappdata}\Programs\TerminalEL
DefaultGroupName=TerminalEL
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.19041
OutputDir={#ArtifactDir}
OutputBaseFilename=TerminalEL-0.1.0-windows-x64-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\WindowsTerminal.exe
CloseApplications=yes
RestartApplications=no
LicenseFile=..\LICENSE
SetupLogging=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "{#PayloadDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "settings\*"

[Dirs]
Name: "{app}\settings"; Flags: uninsneveruninstall

[Icons]
Name: "{group}\TerminalEL"; Filename: "{app}\WindowsTerminal.exe"; WorkingDir: "{%USERPROFILE}"
Name: "{group}\Uninstall TerminalEL"; Filename: "{uninstallexe}"

[Registry]
; ShellExecute (including Win+R) resolves wtel via per-user App Paths.
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\wtel.exe"; ValueType: string; ValueName: ""; ValueData: "{app}\WindowsTerminal.exe"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\wtel.exe"; ValueType: string; ValueName: "Path"; ValueData: "{app}"

[Run]
Filename: "{app}\WindowsTerminal.exe"; WorkingDir: "{%USERPROFILE}"; Description: "Launch TerminalEL"; Flags: nowait postinstall skipifsilent
