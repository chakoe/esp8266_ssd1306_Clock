/**
 * @file web_ota_manager.cpp
 * @brief Web OTA更新管理模块实现
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2026-02-05
 */

#include "web_ota_manager.h"
#include "global_config.h"
#include "utils.h"
#include <ESP8266WiFi.h>

// Web服务器和HTTP更新服务器
ESP8266WebServer webServer(80);
ESP8266HTTPUpdateServer httpUpdater;

// 全局Web OTA配置
WebOtaConfig webOtaConfig = {
    "admin",                       // username
    "admin123",                    // password
    true,                          // authEnabled
    5000                           // triggerTimeout (5秒)
};

// 全局Web OTA状态
WebOtaState webOtaState = {
    WEB_OTA_STATUS_IDLE,           // status
    0,                             // progress
    "",                            // error
    0,                             // startTime
    0,                             // endTime
    "",                            // uploadedFilename
    0                              // uploadedSize
};

// 按键触发状态
static unsigned long triggerStartTime = 0;
static bool triggerDetected = false;

/**
 * @brief 初始化Web OTA管理器
 */
void initWebOtaManager() {
    webOtaState.status = WEB_OTA_STATUS_IDLE;
    webOtaState.progress = 0;
    webOtaState.error[0] = '\0';

    triggerStartTime = 0;
    triggerDetected = false;

    LOG_INFO("Web OTA Manager initialized");
    LOG_INFO("Trigger: Hold K1 for %lu ms", webOtaConfig.triggerTimeout);
    LOG_INFO("Web server port: 80");
    if (webOtaConfig.authEnabled) {
        LOG_INFO("Authentication enabled: %s", webOtaConfig.username);
    } else {
        LOG_INFO("Authentication disabled");
    }
}

/**
 * @brief 更新Web OTA管理器（在主循环中调用）
 */
void updateWebOtaManager() {
    // 处理Web服务器客户端
    if (webOtaState.status == WEB_OTA_STATUS_ACTIVE) {
        webServer.handleClient();
    }
}

/**
 * @brief 检查Web OTA触发条件
 * @param k1Pressed K1按键是否按下
 * @return true 触发成功，false 未触发
 */
bool checkWebOtaTrigger(bool k1Pressed) {
    // 如果Web服务器已经在运行，不处理
    if (webOtaState.status == WEB_OTA_STATUS_ACTIVE) {
        return false;
    }

    // 检查K1按键是否按下
    if (k1Pressed) {
        if (!triggerDetected) {
            // 第一次检测到按下
            triggerStartTime = millis();
            triggerDetected = true;
            LOG_DEBUG("Web OTA trigger detected, holding K1...");
        } else {
            // 检查是否达到触发时间
            unsigned long currentMillis = millis();
            unsigned long elapsed = (currentMillis >= triggerStartTime) ?
                                  (currentMillis - triggerStartTime) :
                                  (0xFFFFFFFF - triggerStartTime + currentMillis);

            if (elapsed >= webOtaConfig.triggerTimeout) {
                // 触发成功
                LOG_INFO("Web OTA trigger activated!");
                startWebOtaServer();
                triggerDetected = false;
                triggerStartTime = 0;
                return true;
            }
        }
    } else {
        // 按键释放，重置触发状态
        if (triggerDetected) {
            unsigned long currentMillis = millis();
            unsigned long elapsed = (currentMillis >= triggerStartTime) ?
                                  (currentMillis - triggerStartTime) :
                                  (0xFFFFFFFF - triggerStartTime + currentMillis);

            if (elapsed < webOtaConfig.triggerTimeout) {
                LOG_DEBUG("Web OTA trigger cancelled (held %lu ms)", elapsed);
            }

            triggerDetected = false;
            triggerStartTime = 0;
        }
    }

    return false;
}

/**
 * @brief 启动Web OTA服务器
 */
void startWebOtaServer() {
    webOtaState.status = WEB_OTA_STATUS_ACTIVE;
    webOtaState.startTime = millis();

    // 配置HTTP更新服务器
    if (webOtaConfig.authEnabled) {
        httpUpdater.setup(&webServer, "/update", 
                         webOtaConfig.username, 
                         webOtaConfig.password);
    } else {
        httpUpdater.setup(&webServer, "/update");
    }

    // 配置根路径
    webServer.on("/", HTTP_GET, []() {
        String html = "<!DOCTYPE html><html><head>";
        html += "<meta charset='UTF-8'>";
        html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
        html += "<title>ESP8266 时钟 - OTA固件升级</title>";
        html += "<style>";
        html += "body{font-family:'Microsoft YaHei',Arial,sans-serif;margin:20px;background:#f0f0f0;}";
        html += ".container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}";
        html += "h1{color:#333;text-align:center;}";
        html += ".status{padding:15px;margin:10px 0;border-radius:5px;background:#e7f3ff;border-left:4px solid #2196F3;}";
        html += ".upload-form{margin-top:20px;}";
        html += "input[type='file']{margin:10px 0;width:100%;padding:8px;border:1px solid #ddd;border-radius:4px;}";
        html += ".btn{display:inline-block;padding:10px 20px;background:#4CAF50;color:white;border:none;border-radius:5px;cursor:pointer;font-size:16px;margin-top:10px;}";
        html += ".btn:hover{background:#45a049;}";
        html += ".info{margin-top:20px;padding:10px;background:#fff3cd;border-left:4px solid #ffc107;border-radius:5px;font-size:14px;}";
        html += "</style></head><body>";
        
        html += "<div class='container'>";
        html += "<h1>🕐 ESP8266 时钟</h1>";
        html += "<div class='status'>";
        html += "<strong>Web OTA 固件升级服务器</strong><br>";
        html += "状态: <span style='color:green'>运行中</span><br>";
        html += "请上传您的固件文件（.bin格式）";
        html += "</div>";
        
        html += "<div class='upload-form'>";
        if (webOtaConfig.authEnabled) {
            html += "<p><strong>⚠️ 需要身份验证</strong><br>";
            html += "用户名: " + String(webOtaConfig.username) + "<br>";
            html += "密码: " + String(webOtaConfig.password) + "</p>";
        }
        html += "<form method='POST' action='/update' enctype='multipart/form-data'>";
        html += "<input type='file' name='firmware' accept='.bin' required><br>";
        html += "<button type='submit' class='btn'>📤 上传固件</button>";
        html += "</form></div>";
        
        html += "<div class='info'>";
        html += "<strong>ℹ️ 使用说明:</strong><br>";
        html += "1. 选择固件文件（.bin格式）<br>";
        html += "2. 点击上传固件按钮<br>";
        html += "3. 等待上传完成<br>";
        html += "4. 设备将自动重启<br><br>";
        html += "<strong>⚠️ 注意:</strong> 升级过程中请勿断电！";
        html += "</div></div></body></html>";
        
        webServer.send(200, "text/html", html);
    });

    // 配置进度页面
    webServer.on("/progress", HTTP_GET, []() {
        String json = "{";
        json += "\"status\":\"" + String(getWebOtaStatusString(webOtaState.status)) + "\",";
        json += "\"progress\":" + String(webOtaState.progress) + ",";
        json += "\"error\":\"" + String(webOtaState.error) + "\",";
        json += "\"filename\":\"" + webOtaState.uploadedFilename + "\",";
        json += "\"size\":" + String(webOtaState.uploadedSize);
        json += "}";
        webServer.send(200, "application/json", json);
    });

    // 启动Web服务器
    webServer.begin();
    
    // 获取设备IP
    String ip = WiFi.localIP().toString();
    
    LOG_INFO("Web OTA server started");
    LOG_INFO("Access URL: http://%s", ip.c_str());
    if (webOtaConfig.authEnabled) {
        LOG_INFO("Username: %s", webOtaConfig.username);
        LOG_INFO("Password: %s", webOtaConfig.password);
    }
}

/**
 * @brief 停止Web OTA服务器
 */
void stopWebOtaServer() {
    if (webOtaState.status == WEB_OTA_STATUS_ACTIVE) {
        webServer.stop();
        webOtaState.status = WEB_OTA_STATUS_IDLE;
        webOtaState.endTime = millis();
        
        LOG_INFO("Web OTA server stopped");
    }
}

/**
 * @brief 处理Web OTA更新（由HTTPUpdateServer内部处理）
 */
void handleWebOtaUpdate() {
    // HTTPUpdateServer内部处理所有更新逻辑
    // 这里只是一个占位函数，实际更新由ESP8266HTTPUpdateServer处理
}

/**
 * @brief 重置Web OTA状态
 */
void resetWebOtaState() {
    webOtaState.status = WEB_OTA_STATUS_IDLE;
    webOtaState.progress = 0;
    webOtaState.error[0] = '\0';
    webOtaState.startTime = 0;
    webOtaState.endTime = 0;
    webOtaState.uploadedFilename = "";
    webOtaState.uploadedSize = 0;
    
    triggerStartTime = 0;
    triggerDetected = false;
    
    LOG_DEBUG("Web OTA state reset");
}

/**
 * @brief 获取Web OTA状态字符串
 * @param status Web OTA状态
 * @return 状态字符串
 */
const char* getWebOtaStatusString(WebOtaStatus status) {
    switch (status) {
        case WEB_OTA_STATUS_IDLE: return "Idle";
        case WEB_OTA_STATUS_WAITING: return "Waiting";
        case WEB_OTA_STATUS_ACTIVE: return "Active";
        case WEB_OTA_STATUS_UPLOADING: return "Uploading";
        case WEB_OTA_STATUS_SUCCESS: return "Success";
        case WEB_OTA_STATUS_FAILED: return "Failed";
        default: return "Unknown";
    }
}

/**
 * @brief 设置Web OTA认证
 * @param username 用户名
 * @param password 密码
 */
void setWebOtaAuth(const char* username, const char* password) {
    if (username != nullptr && strlen(username) > 0) {
        strncpy(webOtaConfig.username, username, sizeof(webOtaConfig.username) - 1);
        webOtaConfig.username[sizeof(webOtaConfig.username) - 1] = '\0';
    }
    
    if (password != nullptr && strlen(password) > 0) {
        strncpy(webOtaConfig.password, password, sizeof(webOtaConfig.password) - 1);
        webOtaConfig.password[sizeof(webOtaConfig.password) - 1] = '\0';
    }
    
    webOtaConfig.authEnabled = (strlen(webOtaConfig.username) > 0 && strlen(webOtaConfig.password) > 0);
    
    LOG_INFO("Web OTA authentication updated");
    if (webOtaConfig.authEnabled) {
        LOG_INFO("  Username: %s", webOtaConfig.username);
        LOG_INFO("  Password: %s", webOtaConfig.password);
    } else {
        LOG_INFO("  Authentication disabled");
    }
}
