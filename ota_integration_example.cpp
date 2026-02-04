/**
 * @file ota_integration_example.cpp
 * @brief OTA集成示例代码
 *
 * 展示如何将OTA功能集成到主程序中
 *
 * 使用方法：
 * 1. 将此文件中的代码片段复制到对应的源文件中
 * 2. 根据实际需求修改配置
 * 3. 重新编译和上传固件
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2026-02-04
 */

// =============================================================================
// 步骤1: 在 esp8266_ssd1306_Clock.ino 中添加以下代码
// =============================================================================

// 在文件开头的 #include 部分添加：
/*
#include "ota_manager.h"
*/

// 在 setup() 函数中，WiFi连接成功后添加：
/*
// ===== OTA 初始化代码 =====
if (systemState.networkConnected) {
    // 初始化OTA管理器
    initOtaManager();

    // 设置当前版本号（根据实际情况修改）
    setOtaVersion("2.1.0");

    // 配置更新服务器URL（替换为你的服务器地址）
    strcpy(otaConfig.updateServerUrl,
           "http://192.168.1.100/firmware/esp8266_ssd1306_Clock.ino.bin");

    // 可选：启用自动更新检查
    // otaConfig.autoUpdateEnabled = true;
    // otaConfig.checkInterval = 86400000; // 24小时

    LOG_INFO("OTA Manager initialized");
    LOG_INFO("Current version: %s", otaConfig.currentVersion);
    LOG_INFO("Update server: %s", otaConfig.updateServerUrl);
}
// ===== OTA 初始化代码结束 =====
*/

// 在 loop() 函数中，yield() 之前添加：
/*
// ===== OTA 处理代码 =====
// 处理OTA更新（自动检查和更新）
handleOtaUpdate();

// 检查串口命令，允许手动触发OTA
if (Serial.available() > 0) {
    char command = Serial.read();

    // 'u' 命令：触发OTA更新
    if (command == 'u' || command == 'U') {
        if (otaState.status == OTA_STATUS_IDLE) {
            LOG_INFO("========================================");
            LOG_INFO("  Manual OTA Update Triggered");
            LOG_INFO("========================================");

            if (startOtaUpdate(otaConfig.updateServerUrl)) {
                LOG_INFO("OTA update started successfully");
                LOG_INFO("Please wait for the update to complete...");
                LOG_INFO("Device will restart automatically");
            } else {
                LOG_WARNING("OTA update failed: %s", otaState.error);
            }
        } else {
            LOG_WARNING("OTA is busy, current status: %s",
                       getOtaStatusString(otaState.status));
        }
    }

    // 's' 命令：显示OTA状态
    else if (command == 'o' || command == 'O') {
        LOG_INFO("========================================");
        LOG_INFO("  OTA Status Information");
        LOG_INFO("========================================");
        LOG_INFO("Status: %s", getOtaStatusString(otaState.status));
        LOG_INFO("Progress: %d%%", otaState.progress);
        LOG_INFO("Current Version: %s", otaConfig.currentVersion);
        LOG_INFO("Latest Version: %s", otaConfig.latestVersion);
        LOG_INFO("Auto Update: %s", otaConfig.autoUpdateEnabled ? "Enabled" : "Disabled");
        LOG_INFO("Last Check: %lu ms ago", millis() - otaConfig.lastCheckTime);

        if (otaState.status == OTA_STATUS_FAILED ||
            otaState.status == OTA_STATUS_ERROR) {
            LOG_INFO("Error: %s", otaState.error);
        }
        LOG_INFO("========================================");
    }

    // 'v' 命令：设置新的固件URL
    else if (command == 'v' || command == 'V') {
        LOG_INFO("Enter new firmware URL (press Enter when done):");

        // 读取URL（简化版，实际使用需要更复杂的输入处理）
        String newUrl = Serial.readStringUntil('\n');
        newUrl.trim();

        if (newUrl.length() > 0 && newUrl.length() < sizeof(otaConfig.updateServerUrl) - 1) {
            strcpy(otaConfig.updateServerUrl, newUrl.c_str());
            LOG_INFO("Firmware URL updated to: %s", otaConfig.updateServerUrl);
        } else {
            LOG_WARNING("Invalid URL length");
        }
    }
}
// ===== OTA 处理代码结束 =====
*/


// =============================================================================
// 步骤2: 在 display_manager.cpp 中添加OTA进度显示
// =============================================================================

// 在 display_manager.h 中添加函数声明：
/*
void displayOtaProgress();
*/

// 在 display_manager.cpp 中添加实现：
/*
/**
 * @brief 显示OTA更新进度
 *\/
void displayOtaProgress() {
    u8g2.clearBuffer();

    // 标题
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.setCursor(10, 20);
    u8g2.print("OTA Update");

    // 状态
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.setCursor(10, 40);
    u8g2.print("Status: ");
    u8g2.print(getOtaStatusString(otaState.status));

    // 进度百分比
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.setCursor(10, 60);
    u8g2.print("Progress: ");

    u8g2.setFont(u8g2_font_ncenB24_tr);
    u8g2.setCursor(70, 62);
    u8g2.print(otaState.progress);
    u8g2.print("%");

    u8g2.sendBuffer();
}
*/


// =============================================================================
// 步骤3: 在 button_handler.cpp 中添加OTA触发功能（可选）
// =============================================================================

// 修改按键处理逻辑，添加长按触发OTA：
/*
// 在某个按键的长时间按下处理中添加
if (pressDuration >= 5000 && pressDuration < 10000) { // 长按5-10秒
    LOG_INFO("Long press detected, checking OTA status...");

    if (systemState.networkConnected) {
        LOG_INFO("Starting OTA update...");
        if (startOtaUpdate(otaConfig.updateServerUrl)) {
            LOG_INFO("OTA update started");
            // 进入OTA显示模式
            settingState.brightnessSettingMode = false;
            settingState.settingMode = false;
            settingState.timeSourceSettingMode = false;

            // 显示OTA进度
            displayOtaProgress();
        } else {
            LOG_WARNING("OTA failed: %s", otaState.error);
        }
    } else {
        LOG_WARNING("WiFi not connected, OTA not available");
    }
}
*/


// =============================================================================
// 步骤4: 在 ota_manager.cpp 中增强进度回调（可选）
// =============================================================================

// 修改 otaProgressCallback 函数以支持屏幕显示：
/*
void otaProgressCallback(int progress, int total) {
    otaState.progress = (progress * 100) / total;

    // 每10%更新一次日志
    if (otaState.progress % 10 == 0) {
        LOG_DEBUG("OTA Update Progress: %d%%", otaState.progress);
    }

    // 如果在OTA模式下，更新屏幕显示
    // （需要在display_manager中实现displayOtaProgress函数）
    // displayOtaProgress();
}
*/


// =============================================================================
// 步骤5: 创建简单的Web服务器用于OTA（高级功能）
// =============================================================================

// 如果需要Web界面OTA，添加以下代码：

/*
#include <ESP8266WebServer.h>

ESP8266WebServer otaServer(80);

void setupOtaWebServer() {
    // OTA状态页面
    otaServer.on("/ota/status", HTTP_GET, []() {
        String json = "{";
        json += "\"status\":\"" + String(getOtaStatusString(otaState.status)) + "\",";
        json += "\"progress\":" + String(otaState.progress) + ",";
        json += "\"currentVersion\":\"" + String(otaConfig.currentVersion) + "\",";
        json += "\"latestVersion\":\"" + String(otaConfig.latestVersion) + "\"";
        json += "}";

        otaServer.send(200, "application/json", json);
    });

    // 触发OTA更新
    otaServer.on("/ota/update", HTTP_POST, []() {
        String firmwareUrl = otaServer.arg("url");

        if (firmwareUrl.length() == 0) {
            firmwareUrl = String(otaConfig.updateServerUrl);
        }

        if (firmwareUrl.length() > 0) {
            LOG_INFO("Web OTA update requested: %s", firmwareUrl.c_str());

            if (startOtaUpdate(firmwareUrl.c_str())) {
                otaServer.send(200, "application/json",
                              "{\"status\":\"started\",\"url\":\"" + firmwareUrl + "\"}");
            } else {
                otaServer.send(500, "application/json",
                              "{\"status\":\"failed\",\"error\":\"" + String(otaState.error) + "\"}");
            }
        } else {
            otaServer.send(400, "application/json",
                          "{\"status\":\"error\",\"message\":\"Missing firmware URL\"}");
        }
    });

    // 设置固件URL
    otaServer.on("/ota/seturl", HTTP_POST, []() {
        String newUrl = otaServer.arg("url");

        if (newUrl.length() > 0 && newUrl.length() < sizeof(otaConfig.updateServerUrl) - 1) {
            strcpy(otaConfig.updateServerUrl, newUrl.c_str());
            otaServer.send(200, "application/json",
                          "{\"status\":\"success\",\"url\":\"" + newUrl + "\"}");
        } else {
            otaServer.send(400, "application/json",
                          "{\"status\":\"error\",\"message\":\"Invalid URL\"}");
        }
    });

    // 启动服务器
    otaServer.begin();
    LOG_INFO("OTA Web server started on port 80");
}

void handleOtaWebServer() {
    otaServer.handleClient();
}

// 在setup()中调用：
// setupOtaWebServer();

// 在loop()中调用：
// handleOtaWebServer();
*/


// =============================================================================
// 使用示例
// =============================================================================

/*
// ===== 串口命令示例 =====

// 1. 检查OTA状态
// 发送: o
// 输出:
// ========================================
//   OTA Status Information
// ========================================
// Status: Idle
// Progress: 0%
// Current Version: 2.1.0
// Latest Version:
// Auto Update: Disabled
// Last Check: 1234567 ms ago
// ========================================

// 2. 触发OTA更新
// 发送: u
// 输出:
// ========================================
//   Manual OTA Update Triggered
// ========================================
// [日期时间] OTA Update started successfully
// [日期时间] Please wait for the update to complete...
// [日期时间] Device will restart automatically

// 3. 设置新的固件URL
// 发送: v
// 输出:
// Enter new firmware URL (press Enter when done):
// 输入: http://192.168.1.100/firmware/new.bin
// 输出:
// [日期时间] Firmware URL updated to: http://192.168.1.100/firmware/new.bin

// ===== Web API示例 =====

// 1. 检查OTA状态
// GET http://device-ip/ota/status
// 响应:
// {
//   "status": "Idle",
//   "progress": 0,
//   "currentVersion": "2.1.0",
//   "latestVersion": ""
// }

// 2. 触发OTA更新
// POST http://device-ip/ota/update
// 参数: url=http://192.168.1.100/firmware.bin
// 响应:
// {
//   "status": "started",
//   "url": "http://192.168.1.100/firmware.bin"
// }

// 3. 设置固件URL
// POST http://device-ip/ota/seturl
// 参数: url=http://new-server.com/firmware.bin
// 响应:
// {
//   "status": "success",
//   "url": "http://new-server.com/firmware.bin"
// }
*/


// =============================================================================
// 固件编译和部署
// =============================================================================

/*
// ===== 编译固件 =====

// 使用Arduino CLI：
// arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 --export-binaries

// 使用Arduino IDE：
// 草图 -> 导出编译的二进制文件

// 固件文件位置：
// build/esp8266.esp8266.nodemcuv2/esp8266_ssd1306_Clock.ino.bin

// ===== 部署固件到服务器 =====

// 1. 将 .bin 文件上传到你的Web服务器
// 2. 确保文件可以通过HTTP访问
// 3. 更新设备中的固件URL配置

// 示例命令（使用curl测试）：
// curl -I http://192.168.1.100/firmware/esp8266_ssd1306_Clock.ino.bin

// ===== 更新设备固件 =====

// 方法1：通过串口
// 打开串口监视器，发送 'u' 命令

// 方法2：通过Web API
// 使用curl或浏览器发送POST请求
// curl -X POST http://device-ip/ota/update?url=http://192.168.1.100/firmware.bin

// 方法3：通过按键（如果已实现）
// 长按指定按键5秒触发OTA
*/


// =============================================================================
// 故障排除
// =============================================================================

/*
// 常见问题和解决方案：

// 问题1：OTA更新失败，提示"HTTP_UPDATE_FAILED"
// 解决：
// - 检查WiFi连接状态
// - 验证固件URL是否正确
// - 测试服务器是否可以访问
// - 检查固件文件是否存在

// 问题2：更新后设备不启动
// 解决：
// - 使用串口上传恢复固件
// - 检查固件是否与设备兼容
// - 查看串口日志了解启动错误

// 问题3：进度显示不准确
// 解决：
// - 检查setOtaProgressCallback()是否正确调用
// - 确保网络连接稳定
// - 增加日志输出频率

// 问题4：Web服务器无法访问
// 解决：
// - 检查防火墙设置
// - 确认设备IP地址
// - 检查端口80是否被占用
*/
