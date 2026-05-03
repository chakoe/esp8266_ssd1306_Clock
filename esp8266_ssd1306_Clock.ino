#include <ESP8266WiFi.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>
#include <WiFiManager.h>
#include <RTClib.h> // 添加DS1307库的引用
#include <Ticker.h> // 用于软件看门狗
#include "config.h"
#include "production_config.h"
#include "button_handler.h"
#include "time_manager.h"
#include "display_manager.h"
#include "system_manager.h"
#include "utils.h"
#include "logger.h"
#include "eeprom_config.h"
#include "web_ota_manager.h"
#include "setup_manager.h"
#include "version.h"

// 全局对象声明 - 现在统一在global_config.cpp中定义
extern WiFiUDP ntpUDP;
extern NTPClient timeClient;
extern RTC_DS1307 rtc;

// 全局变量定义

// 函数声明
void setup();
void loop();

void setup() {
  // 调用系统初始化管理器中的系统初始化函数
  // 将原本冗长的setup()函数拆分为模块化的初始化流程
  systemSetup();
}

void loop() {
  // 更新按键状态
  updateButtonStates();

  // 更新NTP同步状态（非阻塞）
  updateNtpSync();

  // 更新WiFi断开状态（非阻塞）
  updateWifiDisconnect();

  // 执行系统看门狗检查
  systemWatchdog();
  
  unsigned long currentMillis = millis();
  
  // 检查Web OTA触发（长按K1键5秒）
  bool k1Pressed = buttonStates.buttons[0].isPressed;
  if (checkWebOtaTrigger(k1Pressed)) {
    LOG_INFO("Web OTA server started, waiting for firmware upload...");
  }
  
  // 更新Web OTA管理器
  updateWebOtaManager();
  
  // 如果Web OTA服务器正在运行，持续显示OTA模式界面并跳过其他显示更新
  if (webOtaState.status == WEB_OTA_STATUS_ACTIVE) {
    // 重置状态覆盖层，避免OTA期间按键事件导致退出后状态残留
    displayState.showStatus = false;
    displayState.statusOverlayUntil = 0;
    displayOtaMode();
    // 更新主循环时间戳（用于看门狗监控）
    systemState.lastMainLoopTime = currentMillis;
    // 喂狗(重置硬件看门狗)
    ESP.wdtFeed();
    // 小延迟以避免CPU过度使用，但保证按键响应
    yield(); // 让出控制权给WiFi等后台任务
    return; // 跳过后续的显示更新
  }
  
  // 优先处理强制刷新请求
  if (systemState.needsRefresh) {
    if (settingState.timeSourceSettingMode) {
      displayTimeSourceSettingScreen();
    } else if (settingState.brightnessSettingMode) {
      displayBrightnessSettingScreen();
    } else if (settingState.settingMode) {
      displaySettingScreen();
    } else if (displayState.statusOverlayUntil != 0) {
      // 使用溢出安全的时间比较检查是否超时
      if (currentMillis >= displayState.statusOverlayUntil) {
        displayState.statusOverlayUntil = 0;
        displayState.showStatus = false;  // 超时后同步重置showStatus，避免下次按键无效
        displayTime();
      } else {
        displayStatusOverlay();
      }
    } else {
      displayTime();
    }
    systemState.lastDisplayUpdate = currentMillis;
    systemState.lastForcedRefresh = currentMillis;
    systemState.needsRefresh = false; // 确保重置刷新标记
  }
  // 按正常时间间隔更新显示（使用溢出安全的时间比较）
  else {
    unsigned long displayElapsed = (currentMillis >= systemState.lastDisplayUpdate) ?
                                   (currentMillis - systemState.lastDisplayUpdate) :
                                   (0xFFFFFFFF - systemState.lastDisplayUpdate + currentMillis);
    if (displayElapsed > DISPLAY_UPDATE_INTERVAL) {
      if (settingState.timeSourceSettingMode) {
        displayTimeSourceSettingScreen();
      } else if (settingState.brightnessSettingMode) {
        displayBrightnessSettingScreen();
      } else if (settingState.settingMode) {
        displaySettingScreen();
      } else if (displayState.statusOverlayUntil != 0) {
        // 使用溢出安全的时间比较检查是否超时
        if (currentMillis >= displayState.statusOverlayUntil) {
          displayState.statusOverlayUntil = 0;
          displayState.showStatus = false;  // 超时后同步重置showStatus，避免下次按键无效
          displayTime();
        } else {
          displayStatusOverlay();
        }
      } else {
        displayTime();
      }

      systemState.lastDisplayUpdate = currentMillis;
    }
  }
  
  // 如果当前使用NTP时间源，更频繁地检查时间更新
  // 这有助于在RTC故障后更快地获取网络时间
  static unsigned long lastNtpUpdateTime = 0;
  static bool ntpUpdateTimeInitialized = false;

  // 首次进入NTP模式时初始化，避免启动后5秒内频繁检查
  if (timeState.currentTimeSource == TIME_SOURCE_NTP && !ntpUpdateTimeInitialized) {
    lastNtpUpdateTime = currentMillis;
    ntpUpdateTimeInitialized = true;
  } else if (timeState.currentTimeSource != TIME_SOURCE_NTP) {
    ntpUpdateTimeInitialized = false;
  }

  // 检查是否需要立即进行NTP连接检查（例如在切换到NTP时间源后）
  if (timeState.currentTimeSource == TIME_SOURCE_NTP && systemState.networkConnected &&
      timeState.lastNtpCheckAttempt == 0 && !timeState.ntpCheckInProgress) {
    // 尝试进行NTP连接检查
    if (checkNtpConnection(true)) {
      LOG_DEBUG("Successfully got NTP time after source switch");
      // 成功获取NTP时间后，强制刷新显示
      systemState.needsRefresh = true;
      lastNtpUpdateTime = currentMillis; // 更新最后更新时间
    } else {
      LOG_DEBUG("Failed to get NTP time after source switch");
    }
  }

  // 如果当前使用NTP时间源且时间已有一段时间未更新，检查NTP更新
  unsigned long ntpElapsed = (currentMillis >= lastNtpUpdateTime) ?
      (currentMillis - lastNtpUpdateTime) :
      (0xFFFFFFFF - lastNtpUpdateTime + currentMillis);
  if (timeState.currentTimeSource == TIME_SOURCE_NTP &&
      ntpElapsed > 5000) { // 每5秒检查一次NTP更新
    // 使用非阻塞的checkNtpConnection替代timeClient.update()
    if (!timeState.ntpCheckInProgress) {  // 确保不在已有NTP检查进行时
      checkNtpConnection(false);
    }
    lastNtpUpdateTime = currentMillis;
  }
  
  // 更新主循环时间戳（用于看门狗监控）
  systemState.lastMainLoopTime = currentMillis;
  
  // 喂狗(重置硬件看门狗)
  ESP.wdtFeed();
  
  // 小延迟以避免CPU过度使用，但保证按键响应
  yield(); // 让出控制权给WiFi等后台任务
}

























