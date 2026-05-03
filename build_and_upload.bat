@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ========================================================
echo Build and Upload Production Firmware
echo ========================================================
echo.

cd /d "%~dp0"

echo [Step 1] Compiling firmware...
echo ========================================================
echo.

if not exist "bin\" mkdir bin

arduino-cli compile --fqbn esp8266:esp8266:d1_mini --build-property "build.extra_flags=-DPRODUCTION_MODE" --output-dir bin --verbose

if %errorlevel% neq 0 (
    echo.
    echo COMPILATION FAILED!
    pause
    exit /b 1
)

echo.
echo ========================================================
echo Compilation successful!
echo ========================================================
echo.

if not exist "bin\esp8266_ssd1306_Clock.ino.bin" (
    echo ERROR: Firmware file not found after compilation
    pause
    exit /b 1
)

for %%A in ("bin\esp8266_ssd1306_Clock.ino.bin") do (
    set /a SIZE=%%~zA/1024
    echo Firmware: bin\esp8266_ssd1306_Clock.ino.bin
    echo Size: !SIZE! KB
)
echo.

echo [Step 2] Uploading firmware...
echo ========================================================
echo.

arduino-cli board list
echo.

set /p COM_PORT="Enter COM port (e.g., COM3): "
if "%COM_PORT%"=="" (
    echo ERROR: COM port not specified
    pause
    exit /b 1
)

echo.
echo Port: %COM_PORT%
echo Board: esp8266:esp8266:d1_mini
echo.
echo IMPORTANT: Hold FLASH button on ESP8266 during upload!
echo.

arduino-cli upload -p %COM_PORT% --fqbn esp8266:esp8266:d1_mini --input-dir bin

if %errorlevel% equ 0 (
    echo.
    echo ========================================================
    echo BUILD AND UPLOAD SUCCESSFUL!
    echo ========================================================
    echo.
    echo Your ESP8266 clock will restart automatically.
    echo You should see the clock display on the OLED screen.
    echo.
    echo If this is your first time, you may need to:
    echo 1. Long press K4 button to enter WiFi configuration
    echo 2. Connect to "ESP8266-Clock" WiFi hotspot
    echo 3. Configure your WiFi settings
) else (
    echo.
    echo UPLOAD FAILED - but firmware is ready
    echo.
    echo Firmware compiled successfully to:
    echo   bin\esp8266_ssd1306_Clock.ino.bin
    echo.
    echo To upload later, run:
    echo   upload.bat
)

echo.
pause
