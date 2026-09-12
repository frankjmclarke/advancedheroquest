@echo off
rem ===========================================================================
rem  package.bat - build a distributable HQ-Map package
rem
rem  Produces dist\HQ-Map\ and dist\HQ-Map-<version>-win32.zip
rem
rem  The package is portable: the program reads its profile from beside its own
rem  executable (Profile_UseFile uses GetModuleFileName, not the working
rem  directory), and defaults its data paths to its own folder, so it runs from
rem  wherever it is unpacked with no configuration.
rem
rem  Usage:  package.bat
rem ===========================================================================

setlocal

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
set "DIST=%ROOT%\dist"
set "STAGE=%DIST%\HQ-Map"

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
if exist "%STAGE%" rd /s /q "%STAGE%"
md "%STAGE%"
md "%STAGE%\maps"

copy /y "%ROOT%\bin\hq_map.exe" "%STAGE%\" >nul
xcopy /e /i /q /y "%ROOT%\tables" "%STAGE%\tables" >nul
copy /y "%ROOT%\NOTICE.md" "%STAGE%\NOTICE.txt" >nul
if exist "%ROOT%\dist\README.txt" copy /y "%ROOT%\dist\README.txt" "%STAGE%\" >nul

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
  "  $out.Add($line);" ^
  "};" ^
  "[System.IO.File]::WriteAllText('%STAGE%\ahq_map.ini', ($out -join $crlf) + $crlf, [System.Text.Encoding]::GetEncoding(1252));"
if errorlevel 1 (
    echo ERROR: could not write the packaged ahq_map.ini
    exit /b 1
)

rem --- zip -------------------------------------------------------------------
set "ZIP=%DIST%\HQ-Map-%VER%-win32.zip"
if exist "%ZIP%" del /q "%ZIP%"
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "Compress-Archive -Path '%STAGE%' -DestinationPath '%ZIP%' -CompressionLevel Optimal"
if errorlevel 1 (
    echo ERROR: could not create the zip
    exit /b 1
)

echo.
echo Package ready:
for %%F in ("%ZIP%") do echo    %%~fF  (%%~zF bytes)
echo    %STAGE%
exit /b 0
