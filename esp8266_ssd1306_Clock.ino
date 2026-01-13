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
#include "button_handler.h"
#include "time_manager.h"
#include "display_manager.h"
#include "system_manager.h"
#include "utils.h"
#include "logger.h"

#pragma execution_character_set("UTF-8")

// 全局对象声明 - 现在统一在global_config.cpp中定义
extern WiFiUDP ntpUDP;
extern NTPClient timeClient;
extern RTC_DS1307 rtc;

// 全局变量定义
int cachedOffset = -1;

// 函数声明
void setup();
void loop();
void testPasswordEncryption(); // 测试密码加密功能

void setup() {
  // 调用系统管理器中的setup函数
  // 注意：由于Arduino框架的限制，我们需要在这里实现setup逻辑
  
  Serial.begin(115200);
  // 启用ESP8266硬件看门狗
  ESP.wdtEnable(8000); // 8秒硬件看门狗超时
  
  // 测试密码加密功能（仅在生产级版本中启用）
  #ifdef DEBUG_MODE
  testPasswordEncryption();
  #endif
  // 设置WiFi持久化，保存配置到Flash
  WiFi.persistent(true);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  // 初始化按键
  initButtons();
  
  // 初始化I2C总线和OLED显示器
  Wire.begin();  
  u8g2.begin();
  u8g2.setPowerSave(false);
  u8g2.setContrast(BRIGHTNESS_LEVELS[displayState.brightnessIndex]);
  
  // 预计算市场日期偏移量
  getCorrectOffset();
  
  // 初始化RTC（尽早初始化，以便快速显示时间）
  bool rtcSuccess = initializeRTC();
  LOG_DEBUG("RTC init: %s", rtcSuccess ? "Success" : "Failed");
  
  // 显示启动画面（无论RTC是否有效，都显示1秒）
  drawClockIcon();
  LOG_DEBUG("Clock icon displayed");
  // 使用非阻塞延时替代delay(500)
  nonBlockingDelay(500);
  
  // 如果RTC有效，准备立即显示时间
  if (rtcSuccess && systemState.rtcTimeValid) {
    //Serial.println(F("RTC valid, ready to display time"));
    timeState.currentTimeSource = TIME_SOURCE_RTC; // 临时设置时间源
    systemState.needsRefresh = true; // 标记需要立即显示
  }
  
  // 检查是否按下K4键进入配网模式（检测长按，最多1000ms）
  bool pressed = false;
  unsigned long pressStart = millis();
  
  // 检测K4是否被持续按下（长按）
  unsigned long initialLowTime = 0;
  bool wasInitiallyLow = false;
  
  // 检测是否一开始就是低电平（按键被按下）
  if (digitalRead(K4_PIN) == LOW) {
    initialLowTime = millis();
    wasInitiallyLow = true;
  }
  
  // 继续监测一段时间，看是否是长按
  while (millis() - pressStart < 1000) {  // 增加检测时间以确保能检测到长按
    if (digitalRead(K4_PIN) == LOW) {
      if (!wasInitiallyLow) {
        // 按键在此期间变低，记录时间
        initialLowTime = millis();
        wasInitiallyLow = true;
      }
      // 检查是否持续按下超过长按时间
      if (millis() - initialLowTime >= LONG_PRESS_TIME && !pressed) {
        pressed = true;
      }
    } else {
      // 按键释放，重置状态
      wasInitiallyLow = false;
      pressed = false;  // 如果在达到长按时间前释放，则不算作长按
    }
    
    // 使用非阻塞延时
    nonBlockingDelay(10);
  }
  
  // 如果检测到长按，重置按键状态以避免后续冲突
  if (pressed) {
    // 确保按键状态被正确重置
    ButtonState& k4Btn = buttonStates.buttons[3]; // K4对应索引3
    k4Btn.lastState = HIGH;
    k4Btn.stableState = HIGH;
    k4Btn.isPressed = false;
    k4Btn.lastPressTime = 0;
    k4Btn.lastReleaseTime = millis();
  }
  
  if (pressed) {
    LOG_DEBUG("K4 long pressed - entering AP mode");
    resetToAP();
  } else {
    // 尝试自动连接WiFi
    WiFiManager wifiManager;
    wifiManager.setTimeout(20); // 减少配置模式超时时间
    wifiManager.setConnectTimeout(10); // 减少连接超时时间
    
    // 添加自定义参数用于安全的密码存储
    WiFiManagerParameter custom_encrypted_password("encrypted_pass", "Encrypted Password", "", 200);
    wifiManager.addParameter(&custom_encrypted_password);
    
    LOG_DEBUG("Trying to connect to WiFi...");
    systemState.wifiConfigured = wifiManager.autoConnect(getApName().c_str());
    
    // 验证WiFi真实连接状态
    if (systemState.wifiConfigured) {
      // 使用非阻塞延时替代delay(300)
      nonBlockingDelay(300);
      systemState.networkConnected = (WiFi.status() == WL_CONNECTED);
      
      // 如果WiFi连接成功，保存加密的密码（如果用户在配网模式下输入了密码）
      if (systemState.networkConnected && WiFi.SSID().length() > 0) {
        // 注意：WiFiManager自动管理密码，我们这里只是演示加密存储的实现
        // 在实际应用中，可以通过自定义参数来获取用户输入的密码
        LOG_DEBUG("WiFi connected successfully, SSID: %s", WiFi.SSID().c_str());
      }
      
      if (!systemState.networkConnected) {
        LOG_DEBUG("WiFi configured but not connected");
        systemState.wifiConfigured = false;
      }
    } else {
      systemState.networkConnected = false;
    }
    
    LOG_DEBUG("WiFi connection result: %s", systemState.wifiConfigured ? "Success" : "Failed");
    LOG_DEBUG("Network status: %s", systemState.networkConnected ? "Connected" : "Disconnected");
    
    // 初始化NTP客户端
    if (systemState.networkConnected) {
      LOG_DEBUG("IP: %s", WiFi.localIP().toString().c_str());
      timeClient.begin();
      timeClient.setTimeOffset(8 * 3600); // 设置时区偏移（北京时间）
      timeClient.setPoolServerName(timeState.currentNtpServer);
      
      // 首次连接网络时，立即同步一次时间到RTC
      if (systemState.rtcInitialized) {
        //Serial.println(F("Initial sync: NTP -> RTC"));
        // 使用非阻塞延时替代delay(500)
        nonBlockingDelay(500);
        syncNtpToRtc();
      }
    }
  }
  
  // 智能设置时间源，具有完善的降级策略
  setupTimeSources();
  
  const char* sourceName = "";
  switch (timeState.currentTimeSource) {
    case TIME_SOURCE_NONE: sourceName = "NONE"; break;
    case TIME_SOURCE_RTC: sourceName = "RTC"; break;
    case TIME_SOURCE_NTP: sourceName = "NTP"; break;
    case TIME_SOURCE_MANUAL: sourceName = "MANUAL"; break;
  }
  LOG_DEBUG("Time source: %s", sourceName);
  
  // 初始化系统状态变量
  systemState.lastWatchdogCheck = millis();
  systemState.lastNetworkCheck = millis();
  systemState.lastDisplayUpdate = millis();
  timeState.lastRtcSync = millis(); // 初始化RTC同步时间戳
  timeState.lastNtpCheckAttempt = 0; // 初始化NTP检查时间
  timeState.lastTimeSource = timeState.currentTimeSource; // 初始化时间源跟踪
  
  //Serial.println(F("Setup complete"));
}

void loop() {
  // 更新按键状态
  updateButtonStates();
  
  // 执行系统看门狗检查
  systemWatchdog();
  
  unsigned long currentMillis = millis();
  
  // 优先处理强制刷新请求
  if (systemState.needsRefresh) {
    if (settingState.timeSourceSettingMode) {
      displayTimeSourceSettingScreen();
    } else if (settingState.brightnessSettingMode) {
      displayBrightnessSettingScreen();
    } else if (settingState.settingMode) {
      displaySettingScreen();
    } else if (displayState.statusOverlayUntil != 0) {
      // 检查是否超时
      if (currentMillis > displayState.statusOverlayUntil) { // 超时5秒
        displayState.statusOverlayUntil = 0;
        // updateTime();
        displayTime();
      } else {
        displayStatusOverlay();
      }
    } else {
      // updateTime();
      displayTime();
    }
    systemState.lastDisplayUpdate = currentMillis;
    systemState.lastForcedRefresh = currentMillis;
    systemState.needsRefresh = false; // 确保重置刷新标记
  } 
  // 按正常时间间隔更新显示
  else if (currentMillis - systemState.lastDisplayUpdate > DISPLAY_UPDATE_INTERVAL) {
    if (settingState.timeSourceSettingMode) {
      displayTimeSourceSettingScreen();
    } else if (settingState.brightnessSettingMode) {
      displayBrightnessSettingScreen();
    } else if (settingState.settingMode) {
      displaySettingScreen();
    } else if (displayState.statusOverlayUntil != 0) {
      // 检查是否超时
      if (currentMillis > displayState.statusOverlayUntil) {
        displayState.statusOverlayUntil = 0;
        // updateTime();
        displayTime();
      } else {
        displayStatusOverlay();
      }
    } else {
      // updateTime();
      displayTime();
    }
    
    systemState.lastDisplayUpdate = currentMillis;
  }
  
  // 如果当前使用NTP时间源，更频繁地检查时间更新
  // 这有助于在RTC故障后更快地获取网络时间
  static unsigned long lastNtpUpdateTime = 0;
  
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
  if (timeState.currentTimeSource == TIME_SOURCE_NTP && 
      currentMillis - lastNtpUpdateTime > 5000) { // 每5秒检查一次NTP更新
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

// 测试密码加密功能
void testPasswordEncryption() {
  LOG_DEBUG("=== Testing WiFi Password Encryption ===");
  
  // 测试数据
  String testPassword = "TestPassword123!";
  LOG_DEBUG("Original password: %s", testPassword.c_str());
  
  // 生成AES密钥
  uint8_t aesKey[AES_KEY_SIZE];
  generateAESKey(aesKey);
  
  // AES加密测试
  LOG_DEBUG("--- Testing AES Encryption ---");
  String aesEncrypted = encryptPasswordAES(testPassword, aesKey);
  LOG_DEBUG("AES Encrypted password: %s", aesEncrypted.c_str());
  
  // AES解密测试
  String aesDecrypted = decryptPasswordAES(aesEncrypted, aesKey);
  LOG_DEBUG("AES Decrypted password: %s", aesDecrypted.c_str());
  
  // AES验证结果
  bool aesEncryptionSuccess = (testPassword == aesDecrypted);
  LOG_DEBUG("AES Encryption test %s", aesEncryptionSuccess ? "PASSED" : "FAILED");
  
  // XOR加密测试（兼容性测试）
  LOG_DEBUG("--- Testing XOR Encryption (Legacy) ---");
  String xorEncrypted = encryptPassword(testPassword);
  LOG_DEBUG("XOR Encrypted password: %s", xorEncrypted.c_str());
  
  // XOR解密测试
  String xorDecrypted = decryptPassword(xorEncrypted);
  LOG_DEBUG("XOR Decrypted password: %s", xorDecrypted.c_str());
  
  // XOR验证结果
  bool xorEncryptionSuccess = (testPassword == xorDecrypted);
  LOG_DEBUG("XOR Encryption test %s", xorEncryptionSuccess ? "PASSED" : "FAILED");
  
  // 测试存储和加载（优先AES）
  LOG_DEBUG("--- Testing Storage with AES Priority ---");
  saveEncryptedWifiPassword(testPassword);
  String loadedPassword = loadEncryptedWifiPassword();
  bool storageSuccess = (testPassword == loadedPassword);
  LOG_DEBUG("Storage test %s", storageSuccess ? "PASSED" : "FAILED");
  LOG_DEBUG("Loaded password: %s", loadedPassword.c_str());
  
  // 测试错误情况
  LOG_DEBUG("--- Testing Error Handling ---");
  String wrongEncrypted = "WrongEncryptedData123456789012345678901234567890";
  String wrongDecrypted = decryptPasswordAES(wrongEncrypted, aesKey);
  bool aesErrorHandlingSuccess = (wrongDecrypted.length() == 0);
  LOG_DEBUG("AES Error handling test %s", aesErrorHandlingSuccess ? "PASSED" : "FAILED");
  
  String wrongXorEncrypted = "WrongXorData";
  String wrongXorDecrypted = decryptPassword(wrongXorEncrypted);
  bool xorErrorHandlingSuccess = (wrongXorDecrypted.length() == 0);
  LOG_DEBUG("XOR Error handling test %s", xorErrorHandlingSuccess ? "PASSED" : "FAILED");
  
  // 安全性比较
  LOG_DEBUG("--- Security Comparison ---");
  LOG_DEBUG("XOR length: %d, AES length: %d", xorEncrypted.length(), aesEncrypted.length());
  LOG_DEBUG("XOR uses simple XOR, AES uses industry-standard encryption");
  
  // 综合评估
  bool overallSuccess = aesEncryptionSuccess && xorEncryptionSuccess && storageSuccess && 
                       aesErrorHandlingSuccess && xorErrorHandlingSuccess;
  LOG_DEBUG("=== Overall Encryption Test %s ===", overallSuccess ? "PASSED" : "FAILED");
}


























