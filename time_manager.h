/**
 * @file time_manager.h
 * @brief 时间管理模块头文件
 * 
 * 负责处理多种时间源的获取、同步和管理，包括：
 * - NTP网络时间同步
 * - RTC硬件时钟
 * - 软件备用时钟
 * - 时间源智能切换和降级策略
 * 
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2025-12-23
 */

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <NTPClient.h>
#include <RTClib.h>
#include "global_config.h"

// 外部变量声明
extern SystemState systemState;
extern DisplayState displayState;
extern SettingState settingState;
extern TimeState timeState;



// 函数声明
void setupTimeSources(); // 设置时间源
bool checkNtpConnection(bool forceCheck = false); // 检查NTP连接
bool getCurrentTime(DateTime& now); // 获取当前时间（从NTP或DS1306）
bool getCurrentTimeFromNtp(DateTime& now); // 从NTP获取当前时间
void syncNtpToRtc(); // 将NTP时间同步到DS1307
void switchTimeSource(TimeSource newSource); // 切换时间源
const char* getTimeSourceName(TimeSource source); // 获取时间源名称
bool initializeRTC(); // 初始化RTC模块
bool isRtcTimeValid(const DateTime& rtcTime); // 检查RTC时间是否有效

// 时间源设置模式相关函数
void enterTimeSourceSettingMode(); // 进入时间源设置模式
void exitTimeSourceSettingMode(); // 退出时间源设置模式
void displayTimeSourceSettingScreen(); // 显示时间源设置界面
void selectNextTimeSource(); // 选择下一个时间源

#endif