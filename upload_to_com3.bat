@echo off
echo ====================================
echo ESP8266 Clock 固件上传工具
echo ====================================
echo.
echo 正在准备上传固件到 COM3...
echo.

REM 检查固件文件是否存在
if not exist "build\esp8266.esp8266.nodemcuv2\esp8266_ssd1306_Clock.ino.bin" (
    echo 错误: 固件文件不存在！请先编译项目。
    echo 运行: arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 --export-binaries
    pause
    exit /b 1
)

echo 固件文件: build\esp8266.esp8266.nodemcuv2\esp8266_ssd1306_Clock.ino.bin
echo 目标串口: COM3
echo.
echo 注意: 如果遇到权限错误，请以管理员身份运行此脚本
echo.

REM 尝试使用Arduino CLI上传
echo 正在使用Arduino CLI上传...
arduino-cli upload -p COM3 --fqbn esp8266:esp8266:nodemcuv2 --input-dir build\esp8266.esp8266.nodemcuv2

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ====================================
    echo 上传成功！
    echo ====================================
) else (
    echo.
    echo ====================================
    echo 上传失败！
    echo ====================================
    echo.
    echo 可能的解决方案:
    echo 1. 请以管理员身份运行此脚本
    echo 2. 检查COM3是否被其他程序占用
    echo 3. 确保ESP8266已正确连接到COM3
    echo 4. 尝试重新插拔USB线
    echo.
)

pause
