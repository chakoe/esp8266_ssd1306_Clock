/**
 * @file global_config.h
 * @brief 全局配置和变量声明头文件
 * 
 * 集中管理所有全局变量和配置常量，减少extern声明
 * 
 * @author ESP8266 SSD1306 Clock Project
 * @version 2.0
 * @date 2025-12-23
 */

#ifndef GLOBAL_CONFIG_H
#define GLOBAL_CONFIG_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <NTPClient.h>
#include <RTClib.h>
#include <WiFiUdp.h>
#include "config.h"

// AES加密相关常量
#define AES_KEY_SIZE 16
#define AES_IV_SIZE 16
#define MAX_ENCRYPTED_PASSWORD_SIZE 200

// 测试模式标志
extern bool g_testMode;

// 全局对象声明
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern WiFiUDP ntpUDP;
extern NTPClient timeClient;
extern RTC_DS1307 rtc;

// 时间源枚举
typedef enum {
    TIME_SOURCE_NONE,
    TIME_SOURCE_RTC,
    TIME_SOURCE_NTP,
    TIME_SOURCE_MANUAL  // 手动设置的软件时钟
} TimeSource;

// 按钮状态结构体
typedef struct {
    uint8_t pin;
    bool currentState;
    bool lastState;
    bool stableState;
    unsigned long lastDebounceTime;
    unsigned long lastPressTime;
    unsigned long lastReleaseTime;
    bool isPressed;
    unsigned long pressDuration;
    uint8_t clickCount;
    unsigned long lastProcessTime;
} ButtonState;

// 按钮状态结构体数组
typedef struct {
    ButtonState buttons[4];
} ButtonStateArray;

extern ButtonStateArray buttonStates;

// 系统状态结构体
struct SystemState {
    volatile bool wifiConfigured;
    volatile bool rtcInitialized;
    volatile bool rtcTimeValid;
    volatile bool networkConnected;
    volatile unsigned long lastWatchdogCheck;
    volatile unsigned long lastDisplayUpdate;
    volatile unsigned long lastMainLoopTime;
    unsigned long lastButtonPressTime[4];
    unsigned long lastForcedRefresh;
    unsigned long lastNetworkCheck;
    volatile unsigned long wifiReconnectAttempt;
    bool forceDisplayTimeError;
    bool lastForceDisplayTimeError;
    bool needsRefresh;
    char encryptedWifiPassword[MAX_ENCRYPTED_PASSWORD_SIZE]; // 加密的WiFi密码存储
};

extern SystemState systemState;

// 显示相关状态结构体
struct DisplayState {
    int lastDisplayedSecond;
    unsigned long statusOverlayUntil;
    bool largeFont;
    int brightnessIndex;
    bool showStatus;
    char timeSourceStatus[40];
};

extern DisplayState displayState;

// 设置模式状态结构体
struct SettingState {
    bool settingMode;
    int settingField;
    int settingValues[6];
    int settingMaxValues[6];
    int settingMinValues[6];
    bool brightnessSettingMode;
    bool timeSourceSettingMode;
    int selectedTimeSourceIndex;
};

extern SettingState settingState;

// 时间管理状态结构体
struct TimeState {
    TimeSource currentTimeSource;
    TimeSource lastTimeSource;
    bool softwareClockValid;
    unsigned long softwareClockBase;
    unsigned long softwareClockTime; // 修改为时间戳格式
    char currentNtpServer[30];
    int ntpFailCount;
    unsigned long lastRtcSync;
    bool timeSourceChanged;
    bool ntpCheckInProgress;
    unsigned long lastNtpCheckAttempt;
    unsigned long ntpCheckStartTime;  // NTP检查开始时间，用于超时自动重置
    int currentNtpServerIndex;
    unsigned long lastTimeSourceSwitch;
};

extern TimeState timeState;

// 全局常量声明
extern const uint8_t BRIGHTNESS_LEVELS[];
extern const char* const BRIGHTNESS_LABELS[];
extern const char* const MARKET_DAYS[];
extern const char* const CN_WEEKDAYS[];
extern const char* const NTP_SERVERS[];
extern const int NTP_SERVER_COUNT;
extern const unsigned long DISPLAY_UPDATE_INTERVAL;

#endif