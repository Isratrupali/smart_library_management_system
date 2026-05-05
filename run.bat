@echo off
:: ============================================================
:: FILE: run.bat
:: PURPOSE: Start the SmartLibrary server.
::          Always switches to this script's folder so frontend/* paths work.
:: ============================================================
cd /d "%~dp0"

echo ============================================
echo   SmartLibrary SLMS — Starting Server
echo ============================================
echo.

:: Check that slms.exe exists (i.e., compile.bat was run)
if not exist "slms.exe" (
    echo [ERROR] slms.exe not found!
    echo [INFO] Run compile.bat first to build the project.
    pause
    exit /b 1
)

echo [SERVER] Starting SmartLibrary on http://localhost:8080
echo [SERVER] Press Ctrl+C to stop the server.
echo.
echo Demo Login Credentials:
echo   Student:   alice@student.edu  / password123
echo   Librarian: sarah@library.edu  / password123
echo   Admin:     admin@library.edu  / password123
echo.
echo Opening browser...

:: Open the browser after a short delay (to give server time to start)
start "" timeout /t 2 >nul
start "" "http://localhost:8080"

:: Start the server (this command blocks until Ctrl+C)
slms.exe

echo.
echo [SERVER] Server stopped.
pause
