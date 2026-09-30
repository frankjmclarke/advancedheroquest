@echo off
rem ===========================================================================
rem  package.bat - build a distributable HQ-Map package
rem
rem  Produces dist\HQ-Map\, dist\release-stage\HQ-Map\, and the zip/installer.
rem  An existing dist\HQ-Map\ keeps its maps, tables, and profile; its EXE is
rem  refreshed from the same build used for the release packages.
rem
rem  The package is portable: the program reads its profile from beside its own
rem  executable (Profile_UseFile uses GetModuleFileName, not the working
rem  directory), and defaults its data paths to its own folder, so it runs from
rem  wherever it is unpacked with no configuration.
rem
rem  Usage:  package.bat
rem          package.bat release-only  build release assets without refreshing
rem                                    the local playable copy (may be running)
rem ===========================================================================

setlocal

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
set "DIST=%ROOT%\dist"
set "STAGE_ROOT=%DIST%\release-stage"
set "STAGE=%STAGE_ROOT%\HQ-Map"
set "PLAY=%DIST%\HQ-Map"

rem --- build first -----------------------------------------------------------
call "%ROOT%\build-msvc.bat"
if errorlevel 1 (
    echo ERROR: build failed
    exit /b 1
)
if not exist "%ROOT%\bin\hq_map.exe" (
    echo ERROR: bin\hq_map.exe not found after build
    exit /b 1
)

rem --- read the version out of version.h -------------------------------------
set "VER="
for /f "usebackq tokens=3 delims= " %%v in (`findstr /c:"AHQ_MAP_VERSION" "%ROOT%\version.h"`) do set "VER=%%~v"
if not defined VER set "VER=1.0"

echo.
echo Packaging HQ-Map %VER%

rem --- stage -----------------------------------------------------------------
if exist "%STAGE%" (
    powershell -NoProfile -ExecutionPolicy Bypass -Command ^
      "$root = [System.IO.Path]::GetFullPath('%STAGE_ROOT%');" ^
      "$target = [System.IO.Path]::GetFullPath('%STAGE%');" ^
      "if (-not $target.StartsWith($root + [System.IO.Path]::DirectorySeparatorChar, [System.StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe package stage path' };" ^
      "Remove-Item -LiteralPath $target -Recurse -Force"
    if errorlevel 1 exit /b 1
)
md "%STAGE%"
if errorlevel 1 exit /b 1
md "%STAGE%\maps"
if errorlevel 1 exit /b 1

copy /y "%ROOT%\bin\hq_map.exe" "%STAGE%\" >nul
if errorlevel 1 exit /b 1
xcopy /e /i /q /y "%ROOT%\tables" "%STAGE%\tables" >nul
if errorlevel 1 exit /b 1
copy /y "%ROOT%\NOTICE.md" "%STAGE%\NOTICE.txt" >nul
if errorlevel 1 exit /b 1
copy /y "%ROOT%\doc\TABLE-GUIDE.md" "%STAGE%\TABLE-GUIDE.md" >nul
if errorlevel 1 exit /b 1
copy /y "%ROOT%\doc\campaign-builder.md" "%STAGE%\CAMPAIGN-BUILDER.md" >nul
if errorlevel 1 exit /b 1
copy /y "%ROOT%\doc\hero-sessions.md" "%STAGE%\HERO-SESSIONS.md" >nul
if errorlevel 1 exit /b 1
copy /y "%ROOT%\doc\hero-weapons.md" "%STAGE%\HERO-WEAPONS.md" >nul
if errorlevel 1 exit /b 1
copy /y "%ROOT%\doc\spellcasting.md" "%STAGE%\SPELLCASTING.md" >nul
if errorlevel 1 exit /b 1
if exist "%ROOT%\dist\README.txt" (
    copy /y "%ROOT%\dist\README.txt" "%STAGE%\" >nul
    if errorlevel 1 exit /b 1
)

rem  rsh\ is NOT shipped: the tile bitmaps are compiled into the executable as
rem  resources by rc, so they are not read from disk at run time.

rem --- the shipped profile ---------------------------------------------------
rem  Absolute path= entries are stripped so the program falls back to its own
rem  folder; see default_program_dir() in main.c.
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$crlf = [char]13 + [char]10;" ^
  "$pathSections = @('karte','tabelle','fenster','editor','editpath');" ^
  "$src = [System.IO.File]::ReadAllText('%ROOT%\bin\ahq_map.ini', [System.Text.Encoding]::GetEncoding(1252));" ^
  "$sec = '';" ^
  "$out = New-Object System.Collections.Generic.List[string];" ^
  "foreach ($line in $src -split '\r?\n') {" ^
  "  $t = $line.Trim();" ^
  "  if ($t -eq '') { continue };" ^
  "  if ($t.StartsWith('[') -and $t.EndsWith(']')) { $sec = $t.Trim('[',']').ToLower(); if ($sec -eq 'windows') { continue }; $out.Add($line); continue };" ^
  "  if ($sec -eq 'windows') { continue };" ^
  "  $key = ($t -split '=',2)[0].Trim().ToLower();" ^
  "  if ($key -eq 'path') { continue };" ^
  "  if ($key -eq 'text' -and $pathSections -contains $sec) { continue };" ^
  "  if ($key -eq 'zoom' -and $sec -eq 'config') { continue };" ^
  "  if ($sec -eq 'show') { if ($key -eq 'grafik') { $out.Add('Grafik=1') } else { $out.Add(($t -split '=')[0] + '=0') }; continue };" ^
  "  $out.Add($line);" ^
  "};" ^
  "[System.IO.File]::WriteAllText('%STAGE%\ahq_map.ini', ($out -join $crlf) + $crlf, [System.Text.Encoding]::GetEncoding(1252));"
if errorlevel 1 (
    echo ERROR: could not write the packaged ahq_map.ini
    exit /b 1
)

rem --- refresh the local playable copy ---------------------------------------
if /i "%~1"=="release-only" goto package_zip
rem  Preserve user maps, campaigns, and settings already in dist\HQ-Map. Only
rem  replace the executable; keep its first pre-package version as a backup.
if not exist "%PLAY%" (
    xcopy /e /i /q /y "%STAGE%" "%PLAY%" >nul
    if errorlevel 1 exit /b 1
) else (
    if exist "%PLAY%\hq_map.exe" if not exist "%PLAY%\hq_map.prepackage.exe" (
        copy /y "%PLAY%\hq_map.exe" "%PLAY%\hq_map.prepackage.exe" >nul
        if errorlevel 1 exit /b 1
    )
    copy /y "%STAGE%\hq_map.exe" "%PLAY%\hq_map.exe" >nul
    if errorlevel 1 (
        echo ERROR: could not refresh dist\HQ-Map\hq_map.exe. Close the running app and retry.
        exit /b 1
    )
)

rem --- zip -------------------------------------------------------------------
:package_zip
set "ZIP=%DIST%\HQ-Map-%VER%-win32.zip"
if exist "%ZIP%" del /q "%ZIP%"
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "Compress-Archive -Path '%STAGE%' -DestinationPath '%ZIP%' -CompressionLevel Optimal"
if errorlevel 1 (
    echo ERROR: could not create the zip
    exit /b 1
)

rem --- installer (optional: only if Inno Setup is present) --------------------
rem  ISCC is looked for where winget puts a per-user Inno Setup install first,
rem  then the usual machine-wide locations.
set "ISCC="
if exist "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" set "ISCC=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"
if not defined ISCC if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if not defined ISCC if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"

if defined ISCC (
    echo.
    echo Building installer...
    "%ISCC%" /Q /DMyAppVersion=%VER% "%ROOT%\HQ-Map.iss"
    if errorlevel 1 (
        echo ERROR: ISCC failed
        exit /b 1
    )
) else (
    echo.
    echo Inno Setup not found, skipping the installer.
    echo   winget install --id JRSoftware.InnoSetup
)

echo.
echo Package ready:
for %%F in ("%ZIP%") do echo    %%~fF  (%%~zF bytes)
if defined ISCC for %%F in ("%DIST%\HQ-Map-%VER%-setup.exe") do echo    %%~fF  (%%~zF bytes)
echo    %STAGE%
if /i not "%~1"=="release-only" echo    %PLAY%
echo To update an installed copy, run the setup EXE; packaging alone does not install it.
exit /b 0
