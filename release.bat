@echo off
setlocal

set "VSROOT=Z:\visual studio build tools 2026\product"
set "PATH=%LOCALAPPDATA%\Microsoft\WinGet\Packages\WinFlexBison.win_flex_bison_Microsoft.Winget.Source_8wekyb3d8bbwe;%PATH%"
set "PATH=C:\Program Files (x86)\Microsoft Visual Studio\Installer;%PATH%"
set "PATH=%VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"

call "%VSROOT%\Common7\Tools\VsDevCmd.bat" -arch=x64 || exit /b 1

cmake -S . -B build ^
    -G Ninja ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_TOOLCHAIN_FILE="Z:\vcpkg\scripts\buildsystems\vcpkg.cmake" ^
    -DVCPKG_TARGET_TRIPLET=x64-windows-static-md ^
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ^
    || exit /b 1

cmake --build build --config Release --target engine-sim-app || exit /b 1
cmake --build build --config Release --target engine-sim-check || exit /b 1

rem Pull the version string out of s_buildVersion instead of hand-maintaining
rem it in two places. Plain findstr + batch string substitution -- a
rem powershell -Command one-liner here means a regex with embedded quotes
rem inside a batch double-quoted argument, and cmd.exe toggles its own
rem quote state on every raw " regardless of a preceding backslash, so that
rem route mangles the string. Native tools sidestep it entirely.
set "VERLINE="
for /f "tokens=2 delims==" %%A in ('findstr /c:"s_buildVersion = " src\engine_sim_application.cpp') do set "VERLINE=%%A"
set "VERLINE=%VERLINE:"=%"
set "VERLINE=%VERLINE:;=%"
set "VERSION=%VERLINE: =%"
if "%VERSION%"=="" set "VERSION=dev"

set "RELEASE_NAME=engine-sim-%VERSION%-win64"
set "RELEASE_DIR=dist\%RELEASE_NAME%"

if exist "%RELEASE_DIR%" rmdir /s /q "%RELEASE_DIR%"
mkdir "%RELEASE_DIR%\bin" || exit /b 1
mkdir "%RELEASE_DIR%\dependencies\submodules\delta-studio\engines\basic" || exit /b 1

rem engine-sim-app.exe resolves its asset/shader/font paths relative to its
rem own working directory as "../assets" and "../dependencies/..." (see
rem EngineSimApplication::initialize in src/engine_sim_application.cpp) --
rem bin\ + these sibling folders reproduces that same relative layout the
rem dev build already relies on, so no delta.conf override is needed.
copy /y build\engine-sim-app.exe "%RELEASE_DIR%\bin\" >nul || exit /b 1
copy /y build\engine-sim-check.exe "%RELEASE_DIR%\bin\" >nul || exit /b 1
copy /y README.md "%RELEASE_DIR%\" >nul
copy /y LICENSE "%RELEASE_DIR%\" >nul

xcopy /e /i /y /q assets "%RELEASE_DIR%\assets" >nul || exit /b 1
rem The piranha compiler's search paths (compiler.cpp) include "../es/"
rem relative to the exe's working directory -- bin\ + this sibling es\
rem reproduces that, same as assets\ above. Without it, every engine's
rem "import engine_sim.mr" fails to resolve and no engines load.
xcopy /e /i /y /q es "%RELEASE_DIR%\es" >nul || exit /b 1
xcopy /e /i /y /q dependencies\submodules\delta-studio\engines\basic\fonts ^
    "%RELEASE_DIR%\dependencies\submodules\delta-studio\engines\basic\fonts" >nul || exit /b 1
xcopy /e /i /y /q dependencies\submodules\delta-studio\engines\basic\shaders ^
    "%RELEASE_DIR%\dependencies\submodules\delta-studio\engines\basic\shaders" >nul || exit /b 1

if exist "dist\%RELEASE_NAME%.zip" del "dist\%RELEASE_NAME%.zip"
powershell -NoProfile -Command ^
    "Compress-Archive -Path '%RELEASE_DIR%\*' -DestinationPath 'dist\%RELEASE_NAME%.zip'" || exit /b 1

echo.
echo Release packaged: dist\%RELEASE_NAME%.zip
echo Run from: %RELEASE_DIR%\bin\engine-sim-app.exe
