/**
 * @file setup_manager.cpp
 * @brief 系统初始化管理模块实现
 *
 * 实现系统的初始化流程，将setup()函数拆分为多个模块化的初始化函数
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2026-02-05
 */

#include "setup_manager.h"
#include "button_handler.h"
#include "time_manager.h"
#include "display_manager.h"
#include "system_manager.h"
#include "utils.h"
#include "eeprom_config.h"
#include "web_ota_manager.h"
#include "logger.h"

// 外部变量声明
extern SystemState systemState;
extern DisplayState displayState;
extern TimeState timeState;
extern NTPClient timeClient;
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern const uint8_t BRIGHTNESS_LEVELS[];

/**
 * @brief 初始化基础系统
 */
void initBasicSystem() {
  // 初始化串口
  Serial.begin(115200);
  // 可以在代码中添加以下代码检查实际Flash大小
  // Serial.printf("Flash size: %d bytes\n", ESP.getFlashChipRealSize());
  // Serial.printf("Flash size (config): %d bytes\n", ESP.getFlashChipSize());

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

  LOG_DEBUG("Basic system initialized");
}

/**
 * @brief 初始化硬件外设
 */
void initHardwarePeripherals() {
  // 初始化按键
  initButtons();

  // 初始化Web OTA管理器
  initWebOtaManager();

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

  LOG_DEBUG("Hardware peripherals initialized");
}

/**
 * @brief 初始化RTC并显示启动画面
 * @return true RTC初始化成功，false RTC初始化失败
 */
bool initRTCAndBootScreen() {
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
    timeState.currentTimeSource = TIME_SOURCE_RTC; // 临时设置时间源
    systemState.needsRefresh = true; // 标记需要立即显示
  }

  return rtcSuccess;
}

/**
 * @brief 检测K4按键长按，决定是否进入配网模式
 * @return true 检测到长按，需要进入配网模式；false 未检测到长按
 */
bool checkK4LongPress() {
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
      // 按键释放，仅在未达到长按时间前重置状态
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

  return pressed;
}

/**
 * @brief 连接WiFi并初始化NTP
 * @param enterAPMode 是否进入AP模式（配网模式）
 */
void connectWiFiAndInitNTP(bool enterAPMode) {
  if (enterAPMode) {
    LOG_DEBUG("K4 long pressed - entering AP mode");
    resetToAP();
    return;
  }

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
      // 使用非阻塞延时替代delay(500)
      nonBlockingDelay(500);
      syncNtpToRtc();
    }
  }
}

/**
 * @brief 初始化系统状态变量
 */
void initSystemState() {
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

  LOG_DEBUG("System state initialized");
}

/**
 * @brief 完整的系统初始化流程
 */
void systemSetup() {
  // 1. 初始化基础系统
  initBasicSystem();

  // 2. 初始化硬件外设
  initHardwarePeripherals();

  // 3. 初始化RTC并显示启动画面
  initRTCAndBootScreen();

  // 4. 检测K4按键长按，决定是否进入配网模式
  bool enterAPMode = checkK4LongPress();

  // 5. 连接WiFi并初始化NTP
  connectWiFiAndInitNTP(enterAPMode);

  // 6. 初始化系统状态变量
  initSystemState();

  LOG_DEBUG("System setup complete");
}

/**
 * @brief 测试密码加密功能
 */
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
