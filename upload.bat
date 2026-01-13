@echo off
chcp 65001 >nul

echo ========================================================
echo Upload Production Firmware
echo ========================================================
echo.

cd /d "%~dp0"

REM Check if firmware exists
if not exist "bin\esp8266_ssd1306_Clock.ino.bin" (
    echo ERROR: Firmware file not found!
    echo Expected: bin\esp8266_ssd1306_Clock.ino.bin
    echo.
    echo Please compile first:
    echo   compile_to_bin.bat
    pause
    exit /b 1
)

echo Firmware file: bin\esp8266_ssd1306_Clock.ino.bin
for %%A in ("bin\esp8266_ssd1306_Clock.ino.bin") do (
    set /a SIZE=%%~zA/1024
    echo Size: !SIZE! KB
)
echo.

echo [1/2] Checking connected boards...
arduino-cli board list
echo.

set /p COM_PORT="Enter COM port (e.g., COM3): "
if "%COM_PORT%"=="" (
    echo ERROR: COM port not specified
    pause
    exit /b 1
)

echo.
echo [2/2] Uploading firmware...
echo Port: %COM_PORT%
echo Board: esp8266:esp8266:d1_mini
echo.
echo IMPORTANT: Hold FLASH button on ESP8266 during upload!
echo.

arduino-cli upload -p %COM_PORT% --fqbn esp8266:esp8266:d1_mini --input-dir bin

if %errorlevel% equ 0 (
    echo.
    echo ========================================================
    echo UPLOAD SUCCESSFUL!
    echo ========================================================
    echo.
    echo Your ESP8266 clock will restart automatically.
    echo You should see the clock display on the OLED screen.
) else (
    echo.
    echo ========================================================
    echo UPLOAD FAILED
    echo ========================================================
    echo.
    echo Possible reasons:
    echo 1. Wrong COM port - try a different port
    echo 2. Not holding FLASH button - hold it during upload
    echo 3. Device not connected - check USB cable
    echo 4. Driver issues - install CH340 driver
    echo.
    echo Try again, making sure to:
    echo   - Press and hold the FLASH button
    echo   - Click to upload
    echo   - Release FLASH button after upload starts
)

echo.
pause
