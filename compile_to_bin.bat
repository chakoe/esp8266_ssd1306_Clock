@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ========================================================
echo Compile Firmware to bin\ Directory
echo ========================================================
echo.

cd /d "%~dp0"

echo [1/4] Cleaning previous builds...
if exist "bin\" (
    rd /s /q "bin\"
)
if exist "build\" (
    rd /s /q "build\"
)
echo O Previous builds cleaned
echo.

echo [2/4] Creating bin directory...
if not exist "bin\" mkdir bin
echo O bin\ directory ready
echo.

echo [3/4] Compiling to bin directory...
echo ========================================================
echo Compilation Start: %time%
echo ========================================================
echo.

arduino-cli compile --fqbn esp8266:esp8266:d1_mini --build-property "build.extra_flags=-DPRODUCTION_MODE" --output-dir bin --verbose

set COMPILE_RESULT=%errorlevel%

echo.
echo ========================================================
echo Compilation End: %time%
echo Result: %COMPILE_RESULT%
echo ========================================================
echo.

echo [4/4] Checking for firmware in bin directory...
if exist "bin\*.bin" (
    echo ========================================================
    echo SUCCESS! Firmware files found in bin\
    echo ========================================================
    echo.
    dir /b bin\*.bin
    echo.
    for %%A in (bin\*.bin) do (
        echo File: %%~nxA
        for %%B in ("%%A") do (
            set /a SIZE=%%~zB/1024
            echo Size: !SIZE! KB
        )
        echo Path: %%A
        echo.
    )
    echo.
    echo Firmware location: %cd%\bin\
    echo.
    echo You can now upload using:
    echo   arduino-cli upload -p COM3 --fqbn esp8266:esp8266:d1_mini --input-dir bin
) else (
    echo ========================================================
    echo No .bin files found in bin\
    echo ========================================================
    echo.
    echo Contents of bin\ directory:
    dir /b bin\ 2>nul
    echo.
    echo This is unexpected. Please check the compilation
    echo output above for errors.
)

echo.
if %COMPILE_RESULT% neq 0 (
    echo Compilation failed with error code %COMPILE_RESULT%
) else (
    echo Compilation completed successfully
)

echo.
pause
