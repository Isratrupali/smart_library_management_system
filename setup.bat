@echo off
:: ============================================================
:: FILE: setup.bat
:: PURPOSE: Download all required third-party libraries.
::          Run this ONCE before compiling for the first time.
:: DOWNLOADS:
::   libs/crow_all.h   — Crow C++ HTTP framework (single header)
::   libs/sqlite3.h    — SQLite3 C header
::   libs/sqlite3.c    — SQLite3 amalgamation source
:: ============================================================
cd /d "%~dp0"

echo ============================================
echo   SmartLibrary SLMS — Dependency Setup
echo   CSE327 Software Engineering Project
echo ============================================
echo.

:: Create the libs/ directory if it doesn't exist
if not exist "libs" (
    mkdir libs
    echo [SETUP] Created libs/ directory.
)

:: ============================================================
:: STEP 1: Download crow_all.h (Crow HTTP framework)
:: ============================================================
:: Crow is a C++ microframework similar to Python's Flask.
:: crow_all.h is the single-header version — no build needed.
:: We download from the official GitHub releases page.
:: ============================================================
if not exist "libs\crow_all.h" (
    echo [SETUP] Downloading crow_all.h...
    curl -L -o "libs\crow_all.h" "https://github.com/CrowCpp/Crow/releases/download/v1.2.0/crow_all.h"
    if exist "libs\crow_all.h" (
        echo [SETUP] crow_all.h downloaded successfully!
    ) else (
        echo [ERROR] Failed to download crow_all.h
        echo [INFO] Manually download from: https://github.com/CrowCpp/Crow/releases
        echo [INFO] Save it as: libs\crow_all.h
    )
) else (
    echo [SETUP] crow_all.h already exists. Skipping.
)

:: ============================================================
:: STEP 2: Download sqlite3.h (SQLite header file)
:: ============================================================
:: sqlite3.h defines the SQLite C API functions and types.
:: We need this to compile our Database.h Singleton class.
:: ============================================================
if not exist "libs\sqlite3.h" (
    echo [SETUP] Downloading sqlite3.h...
    curl -L -o "libs\sqlite3.h" "https://raw.githubusercontent.com/sqlite/sqlite/master/sqlite3.h"
    if exist "libs\sqlite3.h" (
        echo [SETUP] sqlite3.h downloaded successfully!
    ) else (
        echo [ERROR] Failed to download sqlite3.h
        echo [INFO] Download from: https://sqlite.org/download.html (amalgamation zip)
    )
) else (
    echo [SETUP] sqlite3.h already exists. Skipping.
)

:: ============================================================
:: STEP 3: Download sqlite3.c (SQLite amalgamation source)
:: ============================================================
:: sqlite3.c is the entire SQLite library in one giant C file.
:: We compile it along with our main.cpp.
:: ============================================================
if not exist "libs\sqlite3.c" (
    echo [SETUP] Downloading sqlite3.c (this may take a moment - it's 7MB)...
    curl -L -o "libs\sqlite3.c" "https://raw.githubusercontent.com/sqlite/sqlite/master/sqlite3.c"
    if exist "libs\sqlite3.c" (
        echo [SETUP] sqlite3.c downloaded successfully!
    ) else (
        echo [ERROR] Failed to download sqlite3.c
        echo [INFO] Download sqlite-amalgamation from: https://sqlite.org/download.html
    )
) else (
    echo [SETUP] sqlite3.c already exists. Skipping.
)

:: ============================================================
:: STEP 4: Verify all files exist
:: ============================================================
echo.
echo [SETUP] Verifying downloaded files...

set MISSING=0
if not exist "libs\crow_all.h" ( echo [MISSING] libs\crow_all.h  && set MISSING=1 )
if not exist "libs\sqlite3.h"  ( echo [MISSING] libs\sqlite3.h   && set MISSING=1 )
if not exist "libs\sqlite3.c"  ( echo [MISSING] libs\sqlite3.c   && set MISSING=1 )

if "%MISSING%"=="0" (
    echo [SETUP] All dependencies ready!
    echo.
    echo Next step: Run compile.bat to build the project.
) else (
    echo.
    echo [WARNING] Some files are missing. Please download them manually
    echo           from the URLs shown above and place in the libs/ folder.
)

echo.
pause
