@echo off
rem ===========================================================================
rem  build-msvc.bat - build HQ-Map with Microsoft Visual C++
rem
rem  The project's original toolchain is Borland C++ 4.52, driven by Borland
rem  MAKE via `makefile` (see README.md). This script is an alternative for
rem  machines that have Visual Studio instead of a 1990s Borland install.
rem
rem  It deliberately does NOT use `makefile`: that file's MSVC20/MSVC40 blocks
rem  define tools but no link rules, because every link and resource rule is
rem  guarded by !if "$(COMPILER)"=="BORLANDC".
rem
rem  Usage:
rem      build-msvc.bat          build bin\hq_map.exe and bin\hq_mapdm.exe
rem      build-msvc.bat clean    remove obj\msvc and the generated .rc
rem ===========================================================================

setlocal

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
set "OBJ=%ROOT%\obj\msvc"
set "BIN=%ROOT%\bin"
set "GENRC=%ROOT%\rsh\menu.msvc.rc"

if /i "%~1"=="clean" (
    if exist "%OBJ%" rd /s /q "%OBJ%"
    if exist "%GENRC%" del /q "%GENRC%"
    echo Cleaned.
    exit /b 0
)

rem --- locate Visual Studio --------------------------------------------------
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo ERROR: vswhere.exe not found. Is Visual Studio installed?
    exit /b 1
)

set "VSPATH="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"

if not defined VSPATH (
    echo ERROR: no Visual Studio installation with the C++ toolset was found.
    echo        Install the "Desktop development with C++" workload.
    exit /b 1
)
echo Toolchain: %VSPATH%

rem  The sources are Win16/Win32-era and are built as 32-bit.
rem  Both streams are swallowed: VsDevCmd.bat writes harmless noise to stderr
rem  (for example "'vswhere.exe' is not recognized") even on a successful run.
call "%VSPATH%\VC\Auxiliary\Build\vcvars32.bat" >nul 2>&1
if errorlevel 1 (
    echo ERROR: vcvars32.bat failed. Re-running to show its output:
    call "%VSPATH%\VC\Auxiliary\Build\vcvars32.bat"
    exit /b 1
)

if not exist "%OBJ%" md "%OBJ%"
if not exist "%BIN%" md "%BIN%"

rem --- preserve pre-existing binaries ----------------------------------------
rem  bin\*.exe is gitignored, so an original 1999 build sitting here cannot be
rem  recovered from git once overwritten. Keep a one-time copy.
for %%E in (hq_map hq_mapdm) do (
    if exist "%BIN%\%%E.exe" (
        if not exist "%BIN%\%%E.prebuild.exe" (
            copy /y "%BIN%\%%E.exe" "%BIN%\%%E.prebuild.exe" >nul
            echo Preserved existing %%E.exe as %%E.prebuild.exe
        )
    )
)

cd /d "%ROOT%"

rem --- generate an rc the Microsoft resource compiler accepts -----------------
rem  rsh\menu.rc lines 189 and 208 read:
rem      CAPTION "Advanced Heroquest Map Generator V" AHQ_MAP_VERSION
rem  Borland's brc concatenates those adjacent string literals; Microsoft's rc
rem  does not, and fails with a misleading cascade:
rem      error RC2112 : BEGIN expected in dialog
rem      error RC2135 : file not found: MS Sans Serif
rem  Fold the version into a single literal in a generated copy, so the 1999
rem  source stays untouched. Read and write as CP1252 to preserve the umlauts.
echo Generating rsh\menu.msvc.rc ...
powershell -NoProfile -Command ^
  "$cp = [System.Text.Encoding]::GetEncoding(1252);" ^
  "$q = [char]34;" ^
  "$v = ([regex]::Match([System.IO.File]::ReadAllText('version.h', $cp), $q + '([^' + $q + ']*)' + $q)).Groups[1].Value;" ^
  "if (-not $v) { Write-Error 'could not read AHQ_MAP_VERSION from version.h'; exit 1 };" ^
  "$t = [System.IO.File]::ReadAllText('rsh\menu.rc', $cp);" ^
  "$t = $t.Replace('V' + $q + ' AHQ_MAP_VERSION', 'V' + $v + $q);" ^
  "[System.IO.File]::WriteAllText('rsh\menu.msvc.rc', $t, $cp);"
if errorlevel 1 (
    echo ERROR: could not generate menu.msvc.rc
    exit /b 1
)

rem --- flags -----------------------------------------------------------------
set "CFLAGS=/nologo /c /W3 /DSTDC_HEADERS=1 /DHAVE_DIRENT_H=1 /DSTRICT=1 /D_CRT_SECURE_NO_WARNINGS"
set "CINC=/I. /Imy_lib /Imy_lib\windows /Irsh"
set "LIBS=kernel32.lib user32.lib gdi32.lib shell32.lib comdlg32.lib version.lib winspool.lib ole32.lib advapi32.lib"
set "LFLAGS=/nologo /subsystem:windows /machine:X86 /STACK:20480 /HEAP:4096"

rem  STACK and HEAP mirror hq_map32.def, which the Borland build passes to
rem  tlink32. The .def itself is not reused here: its IMPORTS section is
rem  Borland module-definition syntax that link.exe does not accept.

rem --- source lists ----------------------------------------------------------
set "SRC_COMMON=pice.c move.c rolldice.c table.c features.c random.c stairs.c test.c queue.c set.c map.c liste.c text.c makemap.c icon.c image.c pcx.c bmp.c mem.c"
set "SRC_MYLIB=my_lib\path.c my_lib\termproc.c my_lib\grect.c my_lib\routine.c"
set "SRC_WIN=my_lib\windows\w_mouse.c my_lib\windows\w_dialog.c my_lib\windows\memory.c my_lib\windows\file_io.c my_lib\windows\boxf.c my_lib\windows\mfdb.c my_lib\windows\filesel.c my_lib\windows\openwork.c my_lib\windows\window.c my_lib\windows\ro_help.c my_lib\windows\w_draw.c my_lib\windows\w_print.c my_lib\windows\profile.c"
rem  maindm.c and winddm.c are two lines each: #define DEMO 1 followed by an
rem  #include of the corresponding full-version source. Same code, compiled
rem  a second time with the demo switch on.
set "SRC_FULL=main.c wind.c"
set "SRC_DEMO=maindm.c winddm.c"

echo.
echo Compiling...
for %%F in (%SRC_COMMON% %SRC_MYLIB% %SRC_WIN% %SRC_FULL% %SRC_DEMO%) do (
    cl %CFLAGS% %CINC% /Fo"%OBJ%"\ %%F >nul 2>&1
    if errorlevel 1 (
        echo.
        echo ERROR compiling %%F :
        cl %CFLAGS% %CINC% /Fo"%OBJ%"\ %%F
        exit /b 1
    )
)
echo   ok

echo Compiling resources...
rem  Run from rsh\ so the relative BITMAP paths in grafic.rc resolve.
pushd "%ROOT%\rsh"
rc /nologo /D__WIN32__ /DSTRICT=1 /I. /I.. /fo "%OBJ%\menu.res" menu.msvc.rc
if errorlevel 1 ( popd & echo ERROR: rc failed on menu.msvc.rc & exit /b 1 )
rc /nologo /D__WIN32__ /DSTRICT=1 /I. /I.. /fo "%OBJ%\grafic.res" grafic.rc
if errorlevel 1 ( popd & echo ERROR: rc failed on grafic.rc & exit /b 1 )
popd
rem  Two RC2182 "duplicate dialog control ID 2" warnings are pre-existing in
rem  the 1999 menu.rc and are expected.

set "OBJ_COMMON="
for %%F in (%SRC_COMMON% %SRC_MYLIB% %SRC_WIN%) do call :addobj %%~nF
set "OBJ_RES=%OBJ%\menu.res %OBJ%\grafic.res"

echo Linking bin\hq_map.exe ...
link %LFLAGS% /out:"%BIN%\hq_map.exe" "%OBJ%\main.obj" "%OBJ%\wind.obj" %OBJ_COMMON% %OBJ_RES% %LIBS%
if errorlevel 1 ( echo ERROR: link failed & exit /b 1 )

echo Linking bin\hq_mapdm.exe ...
link %LFLAGS% /out:"%BIN%\hq_mapdm.exe" "%OBJ%\maindm.obj" "%OBJ%\winddm.obj" %OBJ_COMMON% %OBJ_RES% %LIBS%
if errorlevel 1 ( echo ERROR: link failed & exit /b 1 )

echo.
echo Build complete:
for %%E in ("%BIN%\hq_map.exe" "%BIN%\hq_mapdm.exe") do echo    %%~fE  (%%~zE bytes)
echo.
echo Note: bin\ahq_map.ini still points at the original author's paths
echo       (P:\hero\maps, P:\hero\tables). Edit it before running, or the
echo       file dialogs will open on a drive that does not exist.
exit /b 0

:addobj
set "OBJ_COMMON=%OBJ_COMMON% "%OBJ%\%~1.obj""
exit /b 0
