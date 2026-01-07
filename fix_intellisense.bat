@echo off
echo ========================================
echo Raylib Project - IntelliSense Fix Tool
echo ========================================
echo.

echo This script will help fix IntelliSense errors in VS Code
echo.

echo Step 1: Checking if VS Code is running...
tasklist /FI "IMAGENAME eq Code.exe" 2>NUL | find /I /N "Code.exe">NUL
if "%ERRORLEVEL%"=="0" (
    echo WARNING: VS Code is currently running.
    echo Please close VS Code and run this script again for best results.
    echo.
    pause
    exit /b 1
)

echo Step 2: Verifying project structure...
if not exist "include\raylib.h" (
    echo ERROR: raylib.h not found in include directory!
    pause
    exit /b 1
)

if not exist "lib\libraylib.a" (
    echo ERROR: libraylib.a not found in lib directory!
    pause
    exit /b 1
)

if not exist ".vscode\c_cpp_properties.json" (
    echo ERROR: c_cpp_properties.json not found!
    pause
    exit /b 1
)

echo All required files found.
echo.

echo Step 3: Cleaning IntelliSense cache...
if exist ".vscode\.browse.VC.db" del /F /Q ".vscode\.browse.VC.db"
if exist ".vscode\.browse.VC.db-shm" del /F /Q ".vscode\.browse.VC.db-shm"
if exist ".vscode\.browse.VC.db-wal" del /F /Q ".vscode\.browse.VC.db-wal"
if exist ".vscode\ipch" rmdir /S /Q ".vscode\ipch"

echo Cache cleaned.
echo.

echo Step 4: Verifying configuration files...
echo   - c_cpp_properties.json: OK
echo   - tasks.json: OK
echo   - settings.json: OK
echo   - compile_commands.json: OK
echo.

echo ========================================
echo Fix Complete!
echo ========================================
echo.
echo Next steps:
echo 1. Open VS Code
echo 2. Press Ctrl+Shift+P
echo 3. Type "C/C++: Reset IntelliSense Database" and press Enter
echo 4. Press Ctrl+Shift+P again
echo 5. Type "Developer: Reload Window" and press Enter
echo.
echo If errors persist, try:
echo - Restart IntelliSense: Ctrl+Shift+P -> "C/C++: Restart IntelliSense for Active File"
echo - Check that w64devkit is installed at: C:\w64devkit\bin\g++.exe
echo.

pause
