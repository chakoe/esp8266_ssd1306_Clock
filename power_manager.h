/**
 * @file power_manager.h
 * @brief 电源管理模块
 * 
 * 负责系统电源管理，包括低功耗模式、屏幕亮度控制等
 * 
 * @author ESP8266 SSD1306 Clock Project
 * @version 2.0
 * @date 2025-12-23
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include "global_config.h"

// 电源模式枚举
enum PowerMode {
    POWER_MODE_NORMAL,      // 正常模式
    POWER_MODE_LOW_POWER,   // 低功耗模式
    POWER_MODE_SLEEP        // 睡眠模式
};

// 电源管理配置结构体
struct PowerConfig {
    PowerMode currentMode;
    unsigned long lastActivityTime;
    bool autoDimEnabled;
    unsigned long dimTimeout;    // 自动调暗超时（毫秒）
    unsigned long sleepTimeout;  // 睡眠超时（毫秒）
    int nightBrightness;         // 夜间亮度级别
    int dayBrightness;           // 日间亮度级别
};

extern PowerConfig powerConfig;

// 函数声明
void initPowerManagement();
void updatePowerManagement();
void setPowerMode(PowerMode mode);
void enableAutoDim(bool enable);
void setAutoDimTimeout(unsigned long timeout);
void setNightBrightness(int level);
void setDayBrightness(int level);
bool isNightTime();
void updateDisplayBrightness();

#endif