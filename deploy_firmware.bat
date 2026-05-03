@echo off
echo ==========================================
echo ESP8266 Clock Production Deployment
echo 版本: 2.0
echo 构建时间: %date% %time%
echo ==========================================
echo.

echo [INFO] 检查部署依赖...
if not exist "esp8266_ssd1306_Clock.ino" (
    echo [ERROR] 未找到主程序文件 esp8266_ssd1306_Clock.ino
    pause
    exit /b 1
)

echo [SUCCESS] 依赖检查完成

echo [INFO] 编译生产固件...
arduino-cli compile --fqbn esp8266:esp8266:d1_mini esp8266_ssd1306_Clock.ino
if %errorlevel% neq 0 (
    echo [ERROR] 固件编译失败
    pause
    exit /b 1
)
echo [SUCCESS] 固件编译完成

echo.
echo [INFO] 部署准备完成
echo.
echo 部署摘要:
echo - 项目: ESP8266 SSD1306 Clock
echo - 版本: 2.0
echo - 固件已编译完成
echo.
echo 下一步操作:
echo 1. 将ESP8266设备连接到电脑
echo 2. 在设备管理器中确认COM端口号
echo 3. 运行以下命令上传固件（将COM3替换为实际端口号）:
echo    arduino-cli upload --fqbn esp8266:esp8266:d1_mini --port COM3 esp8266_ssd1306_Clock.ino
echo.
echo [SUCCESS] ^^^^^ 生产环境部署准备完成！^^^^^
pause