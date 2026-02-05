/**
 * @file web_ota_manager.h
 * @brief Web OTA更新管理模块
 *
 * 提供基于Web界面的OTA固件更新功能
 * 通过同时按住K1和K2键5秒启动Web更新服务器
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2026-02-05
 */

#ifndef WEB_OTA_MANAGER_H
#define WEB_OTA_MANAGER_H

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include "logger.h"

// Web OTA状态枚举
typedef enum {
    WEB_OTA_STATUS_IDLE,           // 空闲状态
    WEB_OTA_STATUS_WAITING,        // 等待按键触发
    WEB_OTA_STATUS_ACTIVE,         // Web服务器运行中
    WEB_OTA_STATUS_UPLOADING,      // 上传中
    WEB_OTA_STATUS_SUCCESS,        // 成功
    WEB_OTA_STATUS_FAILED          // 失败
} WebOtaStatus;

// Web OTA配置结构体
typedef struct {
    char username[32];             // 用户名
    char password[32];             // 密码
    bool authEnabled;              // 是否启用认证
    unsigned long triggerTimeout;   // 触发超时时间（毫秒）
} WebOtaConfig;

// Web OTA状态结构体
typedef struct {
    WebOtaStatus status;
    int progress;                  // 进度（0-100）
    char error[100];               // 错误信息
    unsigned long startTime;       // 开始时间
    unsigned long endTime;         // 结束时间
    String uploadedFilename;       // 上传的文件名
    unsigned long uploadedSize;    // 上传的文件大小
} WebOtaState;

// 全局Web OTA配置和状态
extern WebOtaConfig webOtaConfig;
extern WebOtaState webOtaState;

// 函数声明
void initWebOtaManager();
void updateWebOtaManager();
bool checkWebOtaTrigger(bool k1Pressed);
void startWebOtaServer();
void stopWebOtaServer();
void handleWebOtaUpdate();
void resetWebOtaState();
const char* getWebOtaStatusString(WebOtaStatus status);
void setWebOtaAuth(const char* username, const char* password);

#endif // WEB_OTA_MANAGER_H
