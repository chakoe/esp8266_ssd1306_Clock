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
#include "ota_manager.h"

#pragma execution_character_set("UTF-8")

// 全局对象声明 - 现在统一在global_config.cpp中定义
extern WiFiUDP ntpUDP;
extern NTPClient timeClient;
extern RTC_DS1307 rtc;

// 全局变量定义

// 函数声明
void setup();
void loop();
void testPasswordEncryption(); // 测试密码加密功能

void setup() {
  // 调用系统管理器中的setup函数
  // 注意：由于Arduino框架的限制，我们需要在这里实现setup逻辑

  Serial.begin(115200);
  // 启用ESP8266硬件看门狗 - 增加超时时间以适应长时间操作
  ESP.wdtEnable(15000); // 15秒硬件看门狗超时（原8秒可能不够）
  
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

  // 初始化EEPROM
  initEEPROM();

  // 从EEPROM加载亮度设置
  uint8_t savedBrightnessIndex = loadBrightnessIndex();
  if (savedBrightnessIndex <= 3) {
    displayState.brightnessIndex = savedBrightnessIndex;
    LOG_DEBUG("Loaded brightness index from EEPROM: %d", savedBrightnessIndex);
  } else {
    LOG_DEBUG("Using default brightness index: %d", displayState.brightnessIndex);
  }

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
  unsigned long initialLowTime = 0;
  bool wasInitiallyLow = false;

  // 检测是否一开始就是低电平（按键被按下）
  if (digitalRead(K4_PIN) == LOW) {
    initialLowTime = millis();
    wasInitiallyLow = true;
  }

  // 继续监测一段时间，看是否是长按
  while (true) {
    unsigned long currentMillis = millis();
    unsigned long elapsed = (currentMillis >= pressStart) ?
                           (currentMillis - pressStart) :
                           (0xFFFFFFFF - pressStart + currentMillis);
    if (elapsed >= 1000) break;  // 检测时间结束

    if (digitalRead(K4_PIN) == LOW) {
      if (!wasInitiallyLow) {
        // 按键在此期间变低，记录时间
        initialLowTime = millis();
        wasInitiallyLow = true;
      }
      // 检查是否持续按下超过长按时间（使用溢出安全的时间比较）
      unsigned long pressElapsed = (currentMillis >= initialLowTime) ?
                                  (currentMillis - initialLowTime) :
                                  (0xFFFFFFFF - initialLowTime + currentMillis);
      if (pressElapsed >= LONG_PRESS_TIME && !pressed) {
        pressed = true;
      }
    } else {
      // 按键释放，仅在未达到长按时间时重置状态
      unsigned long pressElapsed = (currentMillis >= initialLowTime) ?
                                  (currentMillis - initialLowTime) :
                                  (0xFFFFFFFF - initialLowTime + currentMillis);
      if (pressElapsed < LONG_PRESS_TIME) {
        // 如果在达到长按时间前释放，则不算作长按
        wasInitiallyLow = false;
        pressed = false;
      }
      // 否则保持pressed状态不变
    }

    // 使用非阻塞延时（内部已包含喂狗）
    nonBlockingDelay(10);
  }

  // 最终状态检查：确保如果用户持续按住超过长按时间，即使循环退出也能正确识别
  if (digitalRead(K4_PIN) == LOW && wasInitiallyLow) {
    unsigned long currentMillis = millis();
    unsigned long pressElapsed = (currentMillis >= initialLowTime) ?
                                (currentMillis - initialLowTime) :
                                (0xFFFFFFFF - initialLowTime + currentMillis);
    if (pressElapsed >= LONG_PRESS_TIME) {
      pressed = true;
    }
  }
  
  // 如果检测到长按，重置按键状态以避免后续冲突
  if (pressed) {
    // 确保按键状态被正确初始化以匹配当前物理状态
    ButtonState& k4Btn = buttonStates.buttons[3]; // K4对应索引3

    // 读取当前物理按键状态
    bool physicalState = (digitalRead(K4_PIN) == LOW);

    // 同步按键状态到实际物理状态
    k4Btn.lastState = physicalState ? LOW : HIGH;
    k4Btn.stableState = physicalState ? LOW : HIGH;
    k4Btn.isPressed = physicalState;  // 根据实际物理状态设置

    // 如果按键仍处于按下状态，设置按下时间戳
    if (physicalState) {
      k4Btn.lastPressTime = initialLowTime;
      k4Btn.lastReleaseTime = 0;
    } else {
      // 如果按键已释放，设置释放时间戳
      k4Btn.lastPressTime = 0;
      k4Btn.lastReleaseTime = millis();
    }

    // 重置消抖和处理时间
    k4Btn.lastDebounceTime = millis();
    k4Btn.lastProcessTime = millis(); // 重置处理时间，避免重复触发长按

    LOG_DEBUG("K4 button state synchronized: pressed=%d", physicalState);
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

    // 使用配置文件中的AP密码（如果密码为空，则不设置密码，方便用户配置）
    const char* apPassword = WIFI_MANAGER_AP_PASSWORD;
    if (strlen(apPassword) > 0) {
      systemState.wifiConfigured = wifiManager.autoConnect(getApName().c_str(), apPassword);
    } else {
      systemState.wifiConfigured = wifiManager.autoConnect(getApName().c_str());
    }
    
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

      // 初始化OTA管理器
      initOtaManager();

      // 设置当前版本号
      setOtaVersion("v2.1.0");

      // 配置GitHub Releases作为更新服务器
      strcpy(otaConfig.updateServerUrl,
             "https://github.com/chakoe/esp8266_ssd1306_Clock/releases/download/v2.1.0/esp8266_ssd1306_Clock.ino.bin");

      // 启用自动更新检查（可选）
      // otaConfig.autoUpdateEnabled = true;
      // otaConfig.checkInterval = 86400000; // 24小时检查一次

      LOG_INFO("========================================");
      LOG_INFO("  OTA Manager Initialized");
      LOG_INFO("========================================");
      LOG_INFO("Current version: %s", otaConfig.currentVersion);
      LOG_INFO("Update server: GitHub Releases");
      LOG_INFO("Firmware URL: %s", otaConfig.updateServerUrl);
      LOG_INFO("Auto update: %s", otaConfig.autoUpdateEnabled ? "Enabled" : "Disabled");
      LOG_INFO("========================================");
      LOG_INFO("OTA Commands (via Serial):");
      LOG_INFO("  'u' - Start OTA update");
      LOG_INFO("  'o' - Show OTA status");
      LOG_INFO("  'v' - Set new firmware URL");
      LOG_INFO("  'c' - Check for updates");
      LOG_INFO("  'a' - Check and auto-update");
      LOG_INFO("========================================");
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

  // 处理OTA更新
  handleOtaUpdate();

  // 定期自动检查更新（如果启用）
  static unsigned long lastAutoCheckTime = 0;
  if (otaConfig.autoUpdateEnabled) {
    unsigned long autoCheckElapsed = (currentMillis >= lastAutoCheckTime) ?
                                     (currentMillis - lastAutoCheckTime) :
                                     (0xFFFFFFFF - lastAutoCheckTime + currentMillis);

    if (autoCheckElapsed >= otaConfig.checkInterval) {
      LOG_INFO("Auto-check for updates...");
      checkAndUpdateToLatest();
      lastAutoCheckTime = currentMillis;
    }
  }

  // 检查串口命令，允许手动触发OTA
  if (Serial.available() > 0) {
    char command = Serial.read();

    // 'u' 命令：触发OTA更新
    if (command == 'u' || command == 'U') {
      if (otaState.status == OTA_STATUS_IDLE) {
        LOG_INFO("========================================");
        LOG_INFO("  Manual OTA Update Triggered");
        LOG_INFO("========================================");

        if (startOtaUpdate(otaConfig.updateServerUrl)) {
          LOG_INFO("OTA update started successfully");
          LOG_INFO("Please wait for the update to complete...");
          LOG_INFO("Device will restart automatically");
          LOG_INFO("========================================");
        } else {
          LOG_WARNING("OTA update failed: %s", otaState.error);
          LOG_INFO("========================================");
        }
      } else {
        LOG_WARNING("OTA is busy, current status: %s",
                   getOtaStatusString(otaState.status));
      }
    }

    // 'o' 命令：显示OTA状态
    else if (command == 'o' || command == 'O') {
      LOG_INFO("========================================");
      LOG_INFO("  OTA Status Information");
      LOG_INFO("========================================");
      LOG_INFO("Status: %s", getOtaStatusString(otaState.status));
      LOG_INFO("Progress: %d%%", otaState.progress);
      LOG_INFO("Current Version: %s", otaConfig.currentVersion);
      LOG_INFO("Latest Version: %s", otaConfig.latestVersion);
      LOG_INFO("Auto Update: %s", otaConfig.autoUpdateEnabled ? "Enabled" : "Disabled");

      unsigned long timeSinceLastCheck = (currentMillis >= otaConfig.lastCheckTime) ?
                                         (currentMillis - otaConfig.lastCheckTime) :
                                         (0xFFFFFFFF - otaConfig.lastCheckTime + currentMillis);
      LOG_INFO("Last Check: %lu ms ago", timeSinceLastCheck);

      if (otaState.status == OTA_STATUS_FAILED ||
          otaState.status == OTA_STATUS_ERROR) {
        LOG_INFO("Error: %s", otaState.error);
      }
      LOG_INFO("========================================");
    }

    // 'v' 命令：设置新的固件URL
    else if (command == 'v' || command == 'V') {
      LOG_INFO("Enter new firmware URL (press Enter when done):");
    }

    // 'c' 命令：检查最新版本
    else if (command == 'c' || command == 'C') {
      LOG_INFO("Checking for latest version...");
      String latest = getLatestVersionFromGitHub();
      if (latest.length() > 0) {
        LOG_INFO("Latest version: %s", latest.c_str());
        LOG_INFO("Current version: %s", otaConfig.currentVersion);

        if (isNewerVersion(latest.c_str(), otaConfig.currentVersion)) {
          LOG_INFO("New version available!");
          LOG_INFO("Send 'a' to auto-update or 'u' to update manually");
        } else {
          LOG_INFO("Already up to date");
        }
      } else {
        LOG_WARNING("Failed to check version");
      }
    }

    // 'a' 命令：检查并自动更新
    else if (command == 'a' || command == 'A') {
      LOG_INFO("Checking and updating to latest version...");
      if (checkAndUpdateToLatest()) {
        LOG_INFO("Auto-update started successfully");
        LOG_INFO("Please wait for the update to complete...");
        LOG_INFO("Device will restart automatically");
      } else {
        LOG_INFO("No update needed or update failed");
      }
    }
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


























