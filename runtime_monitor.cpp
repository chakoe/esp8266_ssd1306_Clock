/**
 * @file runtime_monitor.cpp
 * @brief 运行时监控模块实现
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2026-02-04
 */

#include "runtime_monitor.h"
#include "global_config.h"
#include "config.h"
#include "utils.h"
#include <ESP8266WiFi.h>

// 全局运行时统计
RuntimeStats runtimeStats = {
    0,      // freeHeap
    0,      // minFreeHeap
    0,      // maxHeapUsed
    0,      // initialHeap
    0,      // uptime
    0,      // lastLoopTime
    0,      // maxLoopTime
    0,      // totalErrors
    0,      // wifiErrors
    0,      // ntpErrors
    0,      // rtcErrors
    0,      // i2cErrors
    0,      // wifiReconnectCount
    0,      // ntpSyncCount
    0,      // ntpSyncSuccessCount
    0,      // buttonPressCount
    0,      // longPressCount
    0,      // displayUpdateCount
    0,      // displayRefreshCount
    0,      // bootTime
    0       // bootCount
};

/**
 * @brief 初始化运行时监控
 */
void initRuntimeMonitor() {
    runtimeStats.bootTime = millis();
    runtimeStats.bootCount++;
    runtimeStats.initialHeap = ESP.getFreeHeap();
    runtimeStats.minFreeHeap = runtimeStats.initialHeap;

    LOG_INFO("Runtime Monitor initialized");
    LOG_INFO("Boot count: %u", runtimeStats.bootCount);
    LOG_INFO("Initial heap: %lu bytes", runtimeStats.initialHeap);
}

/**
 * @brief 更新运行时监控
 */
void updateRuntimeMonitor() {
    unsigned long currentMillis = millis();

    // 更新运行时间
    runtimeStats.uptime = currentMillis - runtimeStats.bootTime;

    // 更新循环时间统计
    if (runtimeStats.lastLoopTime > 0) {
        unsigned long loopTime = currentMillis - runtimeStats.lastLoopTime;
        if (loopTime > runtimeStats.maxLoopTime) {
            runtimeStats.maxLoopTime = loopTime;
        }
    }
    runtimeStats.lastLoopTime = currentMillis;

    // 更新内存统计
    updateMemoryStats();
}

/**
 * @brief 更新内存统计
 */
void updateMemoryStats() {
    unsigned long currentFreeHeap = ESP.getFreeHeap();

    runtimeStats.freeHeap = currentFreeHeap;

    if (currentFreeHeap < runtimeStats.minFreeHeap) {
        runtimeStats.minFreeHeap = currentFreeHeap;
    }

    unsigned long heapUsed = runtimeStats.initialHeap - currentFreeHeap;
    if (heapUsed > runtimeStats.maxHeapUsed) {
        runtimeStats.maxHeapUsed = heapUsed;
    }
}

/**
 * @brief 更新错误统计
 * @param code 错误代码
 */
void updateErrorStats(ErrorCode code) {
    runtimeStats.totalErrors++;

    switch (code) {
        case ERROR_WIFI_CONNECTION_FAILED:
            runtimeStats.wifiErrors++;
            break;
        case ERROR_NTP_CONNECTION_FAILED:
            runtimeStats.ntpErrors++;
            break;
        case ERROR_RTC_INIT_FAILED:
        case ERROR_RTC_I2C_ERROR:
        case ERROR_RTC_TIME_INVALID:
            runtimeStats.rtcErrors++;
            break;
        case ERROR_SYSTEM_WATCHDOG_TIMEOUT:
            // 看门狗超时会导致重启，这里记录到日志
            LOG_WARNING("Watchdog timeout detected");
            break;
        default:
            break;
    }
}

/**
 * @brief 更新网络统计
 * @param connected 是否连接
 * @param ntpSuccess NTP是否成功
 */
void updateNetworkStats(bool connected, bool ntpSuccess) {
    static bool lastConnected = false;

    // 检测WiFi重连
    if (connected && !lastConnected) {
        runtimeStats.wifiReconnectCount++;
    }
    lastConnected = connected;

    // 更新NTP统计
    if (ntpSuccess) {
        runtimeStats.ntpSyncSuccessCount++;
    }
}

/**
 * @brief 更新按键统计
 * @param isLongPress 是否长按
 */
void updateButtonStats(bool isLongPress) {
    runtimeStats.buttonPressCount++;
    if (isLongPress) {
        runtimeStats.longPressCount++;
    }
}

/**
 * @brief 更新显示统计
 * @param isRefresh 是否刷新
 */
void updateDisplayStats(bool isRefresh) {
    runtimeStats.displayUpdateCount++;
    if (isRefresh) {
        runtimeStats.displayRefreshCount++;
    }
}

/**
 * @brief 打印运行时统计
 */
void printRuntimeStats() {
    LOG_INFO("========================================");
    LOG_INFO("  Runtime Statistics");
    LOG_INFO("========================================");
    LOG_INFO("Uptime: %lu ms", runtimeStats.uptime);
    LOG_INFO("Boot Count: %u", runtimeStats.bootCount);

    LOG_DEBUG("");
    LOG_INFO("Memory:");
    LOG_INFO("  Free Heap: %lu bytes", runtimeStats.freeHeap);
    LOG_INFO("  Min Free Heap: %lu bytes", runtimeStats.minFreeHeap);
    LOG_INFO("  Max Heap Used: %lu bytes", runtimeStats.maxHeapUsed);
    LOG_INFO("  Free Heap: %.1f%%", getFreeHeapPercentage());

    LOG_DEBUG("");
    LOG_INFO("Performance:");
    LOG_INFO("  Max Loop Time: %lu ms", runtimeStats.maxLoopTime);
    LOG_INFO("  Display Updates: %u", runtimeStats.displayUpdateCount);
    LOG_INFO("  Display Refreshes: %u", runtimeStats.displayRefreshCount);

    LOG_DEBUG("");
    LOG_INFO("Errors:");
    LOG_INFO("  Total: %u", runtimeStats.totalErrors);
    LOG_INFO("  WiFi: %u", runtimeStats.wifiErrors);
    LOG_INFO("  NTP: %u", runtimeStats.ntpErrors);
    LOG_INFO("  RTC: %u", runtimeStats.rtcErrors);
    LOG_INFO("  I2C: %u", runtimeStats.i2cErrors);

    LOG_DEBUG("");
    LOG_INFO("Network:");
    LOG_INFO("  WiFi Reconnects: %u", runtimeStats.wifiReconnectCount);
    LOG_INFO("  NTP Sync Attempts: %u", runtimeStats.ntpSyncCount);
    LOG_INFO("  NTP Sync Success: %u", runtimeStats.ntpSyncSuccessCount);

    LOG_DEBUG("");
    LOG_INFO("Buttons:");
    LOG_INFO("  Total Presses: %u", runtimeStats.buttonPressCount);
    LOG_INFO("  Long Presses: %u", runtimeStats.longPressCount);

    LOG_INFO("========================================");
}

/**
 * @brief 重置运行时统计
 */
void resetRuntimeStats() {
    memset(&runtimeStats, 0, sizeof(RuntimeStats));
    runtimeStats.bootTime = millis();
    runtimeStats.bootCount = 1;
    runtimeStats.minFreeHeap = ESP.getFreeHeap();

    LOG_INFO("Runtime statistics reset");
}

/**
 * @brief 获取运行时统计JSON
 * @return JSON字符串
 */
const char* getRuntimeStatsJson() {
    static char jsonBuffer[512];
    unsigned long uptimeSeconds = runtimeStats.uptime / 1000;

    snprintf(jsonBuffer, sizeof(jsonBuffer),
             "{"
             "\"uptime\":%lu,"
             "\"freeHeap\":%lu,"
             "\"minFreeHeap\":%lu,"
             "\"maxHeapUsed\":%lu,"
             "\"totalErrors\":%u,"
             "\"wifiErrors\":%u,"
             "\"ntpErrors\":%u,"
             "\"rtcErrors\":%u,"
             "\"wifiReconnectCount\":%u,"
             "\"ntpSyncSuccessCount\":%u,"
             "\"buttonPressCount\":%u,"
             "\"longPressCount\":%u"
             "}",
             uptimeSeconds,
             runtimeStats.freeHeap,
             runtimeStats.minFreeHeap,
             runtimeStats.maxHeapUsed,
             runtimeStats.totalErrors,
             runtimeStats.wifiErrors,
             runtimeStats.ntpErrors,
             runtimeStats.rtcErrors,
             runtimeStats.wifiReconnectCount,
             runtimeStats.ntpSyncSuccessCount,
             runtimeStats.buttonPressCount,
             runtimeStats.longPressCount);

    return jsonBuffer;
}

/**
 * @brief 获取运行时间
 * @return 运行时间（毫秒）
 */
unsigned long getUptime() {
    return runtimeStats.uptime;
}

/**
 * @brief 获取空闲堆内存百分比
 * @return 空闲堆内存百分比
 */
float getFreeHeapPercentage() {
    unsigned long totalHeap = runtimeStats.initialHeap;
    if (totalHeap == 0) return 0.0f;

    return ((float)runtimeStats.freeHeap / totalHeap) * 100.0f;
}
