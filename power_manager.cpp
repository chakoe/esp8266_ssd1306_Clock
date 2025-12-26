/**
 * @file power_manager.cpp
 * @brief 电源管理模块实现
 * 
 * 实现系统电源管理功能
 * 
 * @author ESP8266 SSD1306 Clock Project
 * @version 2.0
 * @date 2025-12-23
 */

#include "power_manager.h"
#include "global_config.h"
#include "logger.h"
#include "time_manager.h"

// 外部变量声明
extern SystemState systemState;
extern DisplayState displayState;
extern SettingState settingState;
extern TimeState timeState;
extern ButtonStateArray buttonStates;

// 电源管理配置定义
PowerConfig powerConfig = {
    POWER_MODE_NORMAL,  // currentMode
    0,                  // lastActivityTime
    true,               // autoDimEnabled
    300000,             // dimTimeout (5分钟自动调暗)
    1800000,            // sleepTimeout (30分钟进入睡眠)
    1,                  // nightBrightness (夜间亮度级别)
    2                   // dayBrightness (日间亮度级别)
};

void initPowerManagement() {
    powerConfig.lastActivityTime = millis();
    updateDisplayBrightness();
    
    LOG_DEBUG("Power management initialized");
}

void updatePowerManagement() {
    unsigned long currentMillis = millis();
    unsigned long inactiveTime = currentMillis - powerConfig.lastActivityTime;
    
    // 检查是否需要更新活动时间
    static unsigned long lastButtonCheck = 0;
    if (currentMillis - lastButtonCheck > 1000) {
        // 检查是否有按键活动
        for (int i = 0; i < 4; i++) {
            if (buttonStates.buttons[i].isPressed) {
                powerConfig.lastActivityTime = currentMillis;
                if (powerConfig.currentMode != POWER_MODE_NORMAL) {
                    setPowerMode(POWER_MODE_NORMAL);
                }
                break;
            }
        }
        lastButtonCheck = currentMillis;
    }
    
    // 自动调暗和睡眠逻辑
    if (powerConfig.autoDimEnabled) {
        if (inactiveTime > powerConfig.sleepTimeout && 
            powerConfig.currentMode != POWER_MODE_SLEEP) {
            setPowerMode(POWER_MODE_SLEEP);
        } else if (inactiveTime > powerConfig.dimTimeout && 
                   powerConfig.currentMode == POWER_MODE_NORMAL) {
            setPowerMode(POWER_MODE_LOW_POWER);
        }
    }
    
    // 根据时间自动调整亮度
    static unsigned long lastBrightnessUpdate = 0;
    if (currentMillis - lastBrightnessUpdate > 60000) { // 每分钟检查一次
        updateDisplayBrightness();
        lastBrightnessUpdate = currentMillis;
    }
}

void setPowerMode(PowerMode mode) {
    if (powerConfig.currentMode == mode) {
        return;
    }
    
    PowerMode previousMode = powerConfig.currentMode;
    powerConfig.currentMode = mode;
    
    switch (mode) {
        case POWER_MODE_NORMAL:
            // 恢复正常模式
            u8g2.setPowerSave(false);
            updateDisplayBrightness();
            LOG_DEBUG("Power mode: Normal");
            break;
            
        case POWER_MODE_LOW_POWER:
            // 低功耗模式：降低亮度
            u8g2.setPowerSave(false);
            u8g2.setContrast(BRIGHTNESS_LEVELS[powerConfig.nightBrightness]);
            displayState.brightnessIndex = powerConfig.nightBrightness;
            LOG_DEBUG("Power mode: Low Power");
            break;
            
        case POWER_MODE_SLEEP:
            // 睡眠模式：关闭显示
            u8g2.setPowerSave(true);
            LOG_DEBUG("Power mode: Sleep");
            break;
    }
    
    // 标记需要刷新显示
    systemState.needsRefresh = true;
}

void enableAutoDim(bool enable) {
    powerConfig.autoDimEnabled = enable;
    if (enable) {
        powerConfig.lastActivityTime = millis();
        setPowerMode(POWER_MODE_NORMAL);
    }
    
    LOG_DEBUG("Auto dimming: %s", enable ? "Enabled" : "Disabled");
}

void setAutoDimTimeout(unsigned long timeout) {
    powerConfig.dimTimeout = timeout;
    LOG_DEBUG("Auto dim timeout set to: %lu", timeout);
}

void setNightBrightness(int level) {
    // 亮度级别检查（0-3）
    if (level >= 0 && level <= 3) {
        powerConfig.nightBrightness = level;
        LOG_DEBUG("Night brightness set to: %d", level);
    } else {
        LOG_DEBUG("Invalid night brightness level: %d", level);
    }
}

void setDayBrightness(int level) {
    // 亮度级别检查（0-3）
    if (level >= 0 && level <= 3) {
        powerConfig.dayBrightness = level;
        LOG_DEBUG("Day brightness set to: %d", level);
    } else {
        LOG_DEBUG("Invalid day brightness level: %d", level);
    }
}

bool isNightTime() {
    static unsigned long lastCheck = 0;
    static bool currentNightMode = false;
    unsigned long currentMillis = millis();
    
    // 每分钟检查一次时间变化
    if (currentMillis - lastCheck > 60000) {
        lastCheck = currentMillis;
        
        // 获取当前时间
        DateTime now;
        if (getCurrentTime(now)) {
            int hour = now.hour();
            
            // 定义夜间时间：晚上10点到早上7点
            bool newNightMode = (hour >= 22 || hour < 7);
            
            // 夜间模式变化时记录日志
            if (newNightMode != currentNightMode) {
                LOG_INFO("Night mode %s", newNightMode ? "enabled" : "disabled");
                currentNightMode = newNightMode;
            }
        } else {
            // 无法获取时间时，保持当前状态
            LOG_WARNING("Failed to get current time for night mode check");
        }
    }
    
    return currentNightMode;
}

void updateDisplayBrightness() {
    if (powerConfig.currentMode == POWER_MODE_SLEEP) {
        return;
    }
    
    int targetBrightness = isNightTime() ? powerConfig.nightBrightness : powerConfig.dayBrightness;
    
    // 确保目标亮度在有效范围内
    if (targetBrightness < 0 || targetBrightness > 3) {
        targetBrightness = 2; // 默认值
    }
    
    if (displayState.brightnessIndex != targetBrightness) {
        displayState.brightnessIndex = targetBrightness;
        u8g2.setContrast(BRIGHTNESS_LEVELS[targetBrightness]);
        
        LOG_DEBUG("Display brightness updated to: %d", targetBrightness);
    }
}