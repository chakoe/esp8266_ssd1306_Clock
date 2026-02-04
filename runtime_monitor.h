/**
 * @file runtime_monitor.h
 * @brief 运行时监控模块
 *
 * 提供系统运行时监控功能，包括内存使用、CPU使用率、错误统计等
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2026-02-04
 */

#ifndef RUNTIME_MONITOR_H
#define RUNTIME_MONITOR_H

#include <Arduino.h>
#include "logger.h"

// 监控统计结构体
typedef struct {
    // 内存统计
    unsigned long freeHeap;          // 空闲堆内存
    unsigned long minFreeHeap;       // 最小空闲堆内存
    unsigned long maxHeapUsed;       // 最大堆内存使用
    unsigned long initialHeap;       // 初始堆内存大小

    // 运行时间统计
    unsigned long uptime;            // 运行时间（毫秒）
    unsigned long lastLoopTime;      // 上次循环时间
    unsigned long maxLoopTime;       // 最大循环时间

    // 错误统计
    uint32_t totalErrors;            // 总错误数
    uint32_t wifiErrors;             // WiFi错误数
    uint32_t ntpErrors;              // NTP错误数
    uint32_t rtcErrors;              // RTC错误数
    uint32_t i2cErrors;              // I2C错误数

    // 网络统计
    uint32_t wifiReconnectCount;     // WiFi重连次数
    uint32_t ntpSyncCount;           // NTP同步次数
    uint32_t ntpSyncSuccessCount;    // NTP同步成功次数

    // 按键统计
    uint32_t buttonPressCount;       // 按键按下次数
    uint32_t longPressCount;         // 长按次数

    // 显示统计
    uint32_t displayUpdateCount;     // 显示更新次数
    uint32_t displayRefreshCount;    // 显示刷新次数

    // 启动统计
    unsigned long bootTime;          // 启动时间
    uint32_t bootCount;              // 启动次数
} RuntimeStats;

// 全局运行时统计
extern RuntimeStats runtimeStats;

// 函数声明
void initRuntimeMonitor();
void updateRuntimeMonitor();
void updateMemoryStats();
void updateErrorStats(ErrorCode code);
void updateNetworkStats(bool connected, bool ntpSuccess);
void updateButtonStats(bool isLongPress);
void updateDisplayStats(bool isRefresh);
void printRuntimeStats();
void resetRuntimeStats();
const char* getRuntimeStatsJson();
unsigned long getUptime();
float getFreeHeapPercentage();

#endif // RUNTIME_MONITOR_H
