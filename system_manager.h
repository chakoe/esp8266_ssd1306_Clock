#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include "config.h"
#include "global_config.h"

// 系统状态全局变量 - 通过systemState结构体访问
// 例如：systemState.wifiConfigured, systemState.rtcInitialized, 等

// 显示刷新相关常量
extern const unsigned long DISPLAY_UPDATE_INTERVAL; // 显示刷新间隔，避免过于频繁刷新

// 函数声明
String getApName();
void resetToAP();
void systemWatchdog(); // 系统看门狗，防止死锁
void checkNetworkStatus(); // 检查网络连接状态
int getCorrectOffset(); // 获取市场日偏移量

// 统一错误处理函数
void reportError(ErrorCode code, ErrorLevel level, const char* message = nullptr);
void handleError(ErrorCode code, ErrorLevel level, const char* message = nullptr);
const char* getErrorDescription(ErrorCode code);

// WiFi密码加密存储函数
void saveEncryptedWifiPassword(const String& password);
String loadEncryptedWifiPassword();
String encryptPassword(const String& password);
String decryptPassword(const String& encrypted);
void generateAESKey(uint8_t* key);
String encryptPasswordAES(const String& password, const uint8_t* key);
String decryptPasswordAES(const String& encrypted, const uint8_t* key);
bool connectWifiWithEncryption(const String& ssid, const String& password);

#endif