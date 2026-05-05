@echo off
:: ============================================================
:: FILE: compile.bat
:: PURPOSE: Build the SmartLibrary SLMS executable.
::          Run this after setup.bat has downloaded the libraries.
:: OUTPUT: slms.exe — the standalone web server
:: REQUIRES: g++ (MinGW-w64) installed and in PATH
:: ============================================================
cd /d "%~dp0"

echo ============================================
echo   SmartLibrary SLMS — Build Script
echo   CSE327 Software Engineering Project
echo ============================================
echo.

:: ============================================================
:: STEP 1: Check that g++ is available
:: ============================================================
where g++ >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] g++ not found in PATH!
    echo [INFO] Install MinGW-w64 from: https://www.mingw-w64.org/
    echo [INFO] Make sure to add it to your PATH environment variable.
    pause
    exit /b 1
)
echo [BUILD] g++ found. Checking version...
g++ --version

:: ============================================================
:: STEP 2: Check that required library files exist
:: ============================================================
echo.
echo [BUILD] Checking library files...

if not exist "libs\crow_all.h" (
    echo [ERROR] libs\crow_all.h not found!
    echo [INFO] Run setup.bat first to download dependencies.
    pause
    exit /b 1
)
if not exist "libs\sqlite3.h" (
    echo [ERROR] libs\sqlite3.h not found!
    echo [INFO] Run setup.bat first to download dependencies.
    pause
    exit /b 1
)
if not exist "libs\sqlite3.c" (
    echo [ERROR] libs\sqlite3.c not found!
    echo [INFO] Run setup.bat first to download dependencies.
    pause
    exit /b 1
)
echo [BUILD] All library files found.

:: ============================================================
:: STEP 3: Compile the project
:: ============================================================
:: Compiler flags explained:
::   src/main.cpp         — Main application entry point
::   libs/sqlite3.c       — SQLite amalgamation (compiled as C code)
::   -o slms.exe          — Output executable name
::   -I libs/             — Include search path for crow_all.h, sqlite3.h
::   -I src/              — Include search path for our own headers
::   -lws2_32             — Windows Sockets library (required by Crow)
::   -lmswsock            — Extended Windows Sockets (required by Crow)
::   -lpthread            — POSIX threads (required by Crow's multithreading)
::   -std=c++17           — C++17 standard (required for <optional>, etc.)
::   -O2                  — Optimization level 2 (faster executable)
::   -DCROW_ENABLE_SSL=0  — Disable SSL (we don't need HTTPS for demo)
:: ============================================================
echo.
:: ============================================================
:: STEP 3: Compile SQLite as C (must be compiled as C, not C++)
:: ============================================================
echo [BUILD] Step 1/2: Compiling SQLite3...
gcc -c libs/sqlite3.c -o libs/sqlite3.o -O2
if %errorlevel% neq 0 (
    echo [ERROR] SQLite compile failed!
    pause
    exit /b 1
)
echo [BUILD] SQLite3 compiled.

:: ============================================================
:: STEP 4: Compile the main C++ project and link with sqlite3.o
:: ============================================================
:: Compiler flags:
::   src/main.cpp     — C++ source files
::   libs/sqlite3.o   — Pre-compiled SQLite object (C)
::   -I libs/         — Search libs/ for crow_all.h, asio.hpp, sqlite3.h
::   -I src/          — Search src/ for our own headers
::   -lws2_32 -lmswsock — Windows networking (required by ASIO/Crow)
::   -lpthread        — POSIX threads (required by Crow server)
::   -std=c++17       — C++17 standard
::   -O2              — Optimization level 2
:: ============================================================
echo [BUILD] Step 2/2: Compiling SmartLibrary C++...
echo [BUILD] (This may take 30-90 seconds - Crow is a large header)
g++ src/main.cpp libs/sqlite3.o -o slms.exe -I libs/ -I src/ -lws2_32 -lmswsock -lpthread -std=c++17 -O2

:: ============================================================
:: STEP 4: Check if compilation succeeded
:: ============================================================
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Compilation FAILED! See error messages above.
    echo.
    echo Common fixes:
    echo   1. Make sure setup.bat was run successfully
    echo   2. Check that crow_all.h version matches (try v1.1.0 if v1.2.0 fails)
    echo   3. Try adding -D_WIN32_WINNT=0x0601 to the compile command
    pause
    exit /b 1
)

:: ============================================================
:: STEP 5: Success message
:: ============================================================
echo.
echo ============================================
echo   BUILD SUCCESSFUL!
echo   Output: slms.exe
echo ============================================
echo.
echo Next step: Run run.bat to start the server.
echo Then open: http://localhost:8080
echo.
pause
