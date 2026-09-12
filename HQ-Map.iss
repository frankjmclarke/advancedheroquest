; ===========================================================================
;  HQ-Map installer (Inno Setup 6)
;
;  Build with:   package.bat          (stages the files, then calls ISCC)
;  or directly:  ISCC /DMyAppVersion=1.0 HQ-Map.iss
;
;  Installs PER USER into %LOCALAPPDATA%\Programs\HQ-Map, deliberately not
;  into Program Files. The program keeps its profile next to its own
;  executable (Profile_UseFile uses GetModuleFileName), so a location a
;  standard user cannot write to would silently fail to save settings.
;  PrivilegesRequired=lowest also means no UAC prompt.
; ===========================================================================

#ifndef MyAppVersion
  #define MyAppVersion "1.0"
#endif

#define MyAppName    "HQ-Map"
#define MyAppExeName "hq_map.exe"

[Setup]
AppId={{7C6BDAE2-2E50-4BB3-89E3-8591A8FA420D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher=Juergen Albuschies
VersionInfoVersion=1.0.0.0
VersionInfoDescription=Advanced HeroQuest Map Generator

DefaultDirName={localappdata}\Programs\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
DisableDirPage=no

PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog

OutputDir=dist
OutputBaseFilename={#MyAppName}-{#MyAppVersion}-setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern

LicenseFile=dist\HQ-Map\NOTICE.txt
InfoAfterFile=dist\HQ-Map\README.txt
UninstallDisplayName={#MyAppName} {#MyAppVersion}
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Files]
Source: "dist\HQ-Map\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "dist\HQ-Map\README.txt";      DestDir: "{app}"; Flags: ignoreversion
Source: "dist\HQ-Map\NOTICE.txt";      DestDir: "{app}"; Flags: ignoreversion
Source: "dist\HQ-Map\tables\*";        DestDir: "{app}\tables"; Flags: ignoreversion recursesubdirs createallsubdirs

; Settings file: never overwrite an existing one, and leave it behind on
; uninstall so a reinstall keeps the user's preferences.
Source: "dist\HQ-Map\ahq_map.ini";     DestDir: "{app}"; Flags: onlyifdoesntexist uninsneveruninstall

[Dirs]
; Default output folder for saved maps. Left in place on uninstall so a
; user's own saved maps are not deleted with the program.
Name: "{app}\maps"; Flags: uninsneveruninstall

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"
Name: "{autodesktop}\{#MyAppName}";  Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent
