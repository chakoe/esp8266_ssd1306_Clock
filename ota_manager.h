/**
 * @file ota_manager.h
 * @brief OTA升级管理模块
 *
 * 提供OTA（Over-The-Air）固件升级功能
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2026-02-04
 */

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include "logger.h"

// OTA升级状态枚举
typedef enum {
    OTA_STATUS_IDLE,           // 空闲状态
    OTA_STATUS_CHECKING,       // 检查更新
    OTA_STATUS_DOWNLOADING,    // 下载中
    OTA_STATUS_UPDATING,       // 更新中
    OTA_STATUS_SUCCESS,        // 成功
    OTA_STATUS_FAILED,         // 失败
    OTA_STATUS_ERROR           // 错误
} OtaStatus;

// OTA升级配置结构体
typedef struct {
    char updateServerUrl[150];  // 更新服务器URL（增加到150字节以支持GitHub URL）
    char currentVersion[20];    // 当前版本号
    char latestVersion[20];     // 最新版本号
    bool autoUpdateEnabled;     // 是否启用自动更新
    unsigned long lastCheckTime; // 上次检查时间
    unsigned long checkInterval; // 检查间隔（毫秒）
} OtaConfig;

// OTA升级状态结构体
typedef struct {
    OtaStatus status;
    int progress;               // 进度（0-100）
    char error[100];            // 错误信息
    unsigned long startTime;    // 开始时间
    unsigned long endTime;      // 结束时间
} OtaState;

// 全局OTA配置和状态
extern OtaConfig otaConfig;
extern OtaState otaState;

// 函数声明
void initOtaManager();
bool checkForUpdates();
bool startOtaUpdate(const char* firmwareUrl);
void setOtaProgressCallback();
void handleOtaUpdate();
void resetOtaState();
const char* getOtaStatusString(OtaStatus status);
void setOtaVersion(const char* version);
String getLatestVersionFromGitHub();
String buildFirmwareUrl(const char* version);
bool checkAndUpdateToLatest();
bool isNewerVersion(const char* latestVersion, const char* currentVersion);

#endif // OTA_MANAGER_H
