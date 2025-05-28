@echo off
echo Building FFXI AutoLogin GUI with Modern Design...
echo.

REM Check if we have cl available
where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Visual Studio Build Tools not found in PATH
    echo.
    echo Please run this from a "Developer Command Prompt" or install:
    echo - Visual Studio 2022 Community (free)
    echo - OR Visual Studio Build Tools 2022
    echo.
    echo You can also open this project in Visual Studio and build it there.
    pause
    exit /b 1
)

echo Compiling GUI version with modern Win32 interface...
cl /EHsc gui_main.cpp theme.cpp ../sha1.cpp ^
   /I"../include" ^
   /link user32.lib gdi32.lib comctl32.lib gdiplus.lib ws2_32.lib shlwapi.lib psapi.lib iphlpapi.lib ^
   /OUT:FFXI-Launcher-GUI.exe

if %errorlevel% equ 0 (
    echo.
    echo ✓ Build successful! FFXI-Launcher-GUI.exe created.
    echo.
    echo Features:
    echo - Modern dark theme interface
    echo - Visual character management
    echo - Encrypted password storage
    echo - Cross-platform compatible (works in Wine)
    echo.
    echo Run FFXI-Launcher-GUI.exe to start the application.
) else (
    echo.
    echo ✗ Build failed. Check the error messages above.
    echo.
    echo Common issues:
    echo - Missing include files: Make sure include/ folder exists
    echo - Library errors: Ensure you're in a Developer Command Prompt
)

pause
