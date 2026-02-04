@echo off
echo ====================================
echo ESP8266 Clock 固件上传工具
echo ====================================
echo.
echo 正在上传修复后的固件到 COM3...
echo.

REM 检查固件文件是否存在
if not exist "build\esp8266.esp8266.nodemcuv2\esp8266_ssd1306_Clock.ino.bin" (
    echo 错误: 固件文件不存在！
    echo 请先编译项目
    pause
    exit /b 1
)

echo 固件文件: build\esp8266.esp8266.nodemcuv2\esp8266_ssd1306_Clock.ino.bin
echo 目标串口: COM3
echo.
echo ====================================
echo 重要提示
echo ====================================
echo 如果遇到权限错误，请：
echo 1. 关闭所有串口监视器
echo 2. 关闭Arduino IDE
echo 3. 以管理员身份运行此脚本
echo.
echo ====================================
echo.

REM 尝试使用Arduino CLI上传
echo 正在使用Arduino CLI上传...
arduino-cli upload -p COM3 --fqbn esp8266:esp8266:nodemcuv2 --input-dir build\esp8266.esp8266.nodemcuv2

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ====================================
    echo 上传成功！
    echo ====================================
    echo.
    echo 已修复的问题：
    echo - 改进了GitHub API连接
    echo - 添加了重试机制（最多3次）
    echo - 使用HTTPS安全连接
    echo - 改进了错误处理
    echo - 增加了超时时间
    echo.
    echo 新功能：
    echo - 自动检测GitHub最新版本
    echo - 智能版本比较
    echo - 一键自动更新
    echo.
    echo 测试命令（通过串口）：
    echo   c - 检查最新版本
    echo   a - 检查并自动更新
    echo   o - 查看OTA状态
    echo.
) else (
    echo.
    echo ====================================
    echo 上传失败！
    echo ====================================
    echo.
    echo 可能的解决方案：
    echo 1. 以管理员身份运行此脚本
    echo 2. 关闭所有占用COM3的程序
    echo 3. 检查ESP8266是否正确连接
    echo 4. 尝试重新插拔USB线
    echo.
    echo 如果仍然失败，请尝试：
    echo 1. 打开设备管理器
    echo 2. 找到COM3端口
    echo 3. 右键 -> 卸载设备
    echo 4. 重新插拔USB线
    echo.
)

pause
