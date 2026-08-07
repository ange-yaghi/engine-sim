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

echo.
echo Build complete: build\engine-sim-app.exe
