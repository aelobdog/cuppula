@echo off
setlocal EnableDelayedExpansion

:: === Config =====================================
set "BUILD_DIR=build"
set "EXE_NAME=asm.exe"
set "FULL_EXE=%BUILD_DIR%\%EXE_NAME%"

set "CFLAGS=/nologo /std:c17 /O2 /EHsc /Zi /W4 /WX"
set "FO_FLAG=/Fo:%BUILD_DIR%\"
set "FE_FLAG=/Fe:%FULL_EXE%"

:: === Create build dir if missing ================
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: === Check for cl.exe ===========================
where cl >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: cl.exe not found
    echo.
    pause
    exit /b 1
)

:: === Clean if requested =========================
if /I "%1"=="clean" (
    echo Cleaning %BUILD_DIR% ...
    rd /s /q "%BUILD_DIR%" 2>nul
    mkdir "%BUILD_DIR%"
    echo Done.
    pause
    exit /b 0
)

:: === Build ======================================
echo Building into %BUILD_DIR% ...

cl %CFLAGS% %FO_FLAG% %FE_FLAG% src/*.c

if %ERRORLEVEL% neq 0 (
    echo.
    echo Build FAILED ─ see errors above
    echo.
    pause
    exit /b 1
)

echo.
echo Success!
echo Executable: %FULL_EXE%
echo Run it:    %FULL_EXE%
echo.
