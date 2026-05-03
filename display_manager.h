#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <U8g2lib.h>
#include "global_config.h"

// 函数声明
void displayTime();
void displayStatusOverlay();
void displayOtaMode();
void displayOtaUpdating();
void displayOtaComplete();
void displayOtaFailed();
void displayOtaProgress(uint8_t progress, uint32_t uploadedSize, uint32_t totalSize);
void calculateMarketDay(time_t currentTime, int& marketIndex);
void oledShowLines(const char* l1, const char* l2 = nullptr, const char* l3 = nullptr, const char* l4 = nullptr);
void oledShowLinesSmall(const char* l1, const char* l2 = nullptr, const char* l3 = nullptr, const char* l4 = nullptr);
void displayError(const char* l1, const char* l2 = nullptr, const char* l3 = nullptr, const char* l4 = nullptr);
void drawClockIcon();
void displayErrorScreen(const char* errorMessage, const char* errorDetail = nullptr);

// 设置模式相关函数
void enterSettingMode();
void exitSettingMode();
void displaySettingScreen();
void updateSettingValue(int direction);

// 亮度设置模式相关函数
void enterBrightnessSettingMode();
void exitBrightnessSettingMode();
void displayBrightnessSettingScreen();
void updateBrightnessSetting(int direction);

// 新增工具函数声明
void drawProgmemString(const char* progmemStr, int x, int y);
void formatTimeString(char* buffer, size_t size, int hour, int minute, int second);
void formatDateString(char* buffer, size_t size, int year, int month, int day);

// 版本信息显示函数
void displayVersionInfo();

#endif