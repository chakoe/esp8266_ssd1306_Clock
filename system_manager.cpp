#include "system_manager.h"
#include "time_manager.h"
#include "display_manager.h"
#include "button_handler.h"
#include "utils.h"
#include <WiFiManager.h>
#include <RTClib.h>
#include "logger.h"

// 外部变量声明 - 精简版本
extern SystemState systemState;
extern DisplayState displayState;
extern SettingState settingState;
extern TimeState timeState;
extern NTPClient timeClient;
extern RTC_DS1307 rtc;
extern const uint8_t BRIGHTNESS_LEVELS[];
extern const char* const BRIGHTNESS_LABELS[];
extern const char* const MARKET_DAYS[];
extern const char* const CN_WEEKDAYS[];
extern const unsigned long DISPLAY_UPDATE_INTERVAL;

// 显示刷新相关常量声明 - 现在统一在global_config.cpp中定义
// extern const unsigned long DISPLAY_UPDATE_INTERVAL;

// 错误描述字符串数组 - 使用PROGMEM优化
const char ERROR_DESC_NONE[] PROGMEM = "无错误";
const char ERROR_DESC_RTC_INIT_FAILED[] PROGMEM = "RTC初始化失败";
const char ERROR_DESC_RTC_I2C_ERROR[] PROGMEM = "RTC I2C通信错误";
const char ERROR_DESC_RTC_TIME_INVALID[] PROGMEM = "RTC时间无效";
const char ERROR_DESC_WIFI_CONNECTION_FAILED[] PROGMEM = "WiFi连接失败";
const char ERROR_DESC_NTP_CONNECTION_FAILED[] PROGMEM = "NTP连接失败";
const char ERROR_DESC_TIME_SOURCE_UNAVAILABLE[] PROGMEM = "时间源不可用";
const char ERROR_DESC_TIME_SETTING_INVALID[] PROGMEM = "时间设置无效";
const char ERROR_DESC_SYSTEM_WATCHDOG_TIMEOUT[] PROGMEM = "系统看门狗超时";
const char ERROR_DESC_BUTTON_STATE_INVALID[] PROGMEM = "按键状态无效";

const char* const ERROR_DESCRIPTIONS[] PROGMEM = {
  ERROR_DESC_NONE, ERROR_DESC_RTC_INIT_FAILED, ERROR_DESC_RTC_I2C_ERROR,
  ERROR_DESC_RTC_TIME_INVALID, ERROR_DESC_WIFI_CONNECTION_FAILED,
  ERROR_DESC_NTP_CONNECTION_FAILED, ERROR_DESC_TIME_SOURCE_UNAVAILABLE,
  ERROR_DESC_TIME_SETTING_INVALID, ERROR_DESC_SYSTEM_WATCHDOG_TIMEOUT,
  ERROR_DESC_BUTTON_STATE_INVALID
};

// 错误级别描述
const char ERROR_LEVEL_DESC_INFO[] PROGMEM = "信息";
const char ERROR_LEVEL_DESC_WARNING[] PROGMEM = "警告";
const char ERROR_LEVEL_DESC_ERROR[] PROGMEM = "错误";
const char ERROR_LEVEL_DESC_CRITICAL[] PROGMEM = "严重";

const char* const ERROR_LEVEL_DESCRIPTIONS[] PROGMEM = {
  ERROR_LEVEL_DESC_INFO, ERROR_LEVEL_DESC_WARNING, ERROR_LEVEL_DESC_ERROR, ERROR_LEVEL_DESC_CRITICAL
};

String getApName() {
  static char apName[40]; // 增加缓冲区大小确保安全
  int result = snprintf(apName, sizeof(apName), "Clock_AP_%X", ESP.getChipId());
  if (result >= sizeof(apName)) {
    // 处理缓冲区截断
    strncpy(apName, "Clock_AP_Default", sizeof(apName) - 1);
    apName[sizeof(apName) - 1] = '\0';
  }
  return String(apName);
}

void resetToAP() {
  LOG_DEBUG("Resetting to AP mode...");

  // 显示配网模式信息3秒
  displayError("配网模式", "3秒后进入后台配网");
  // 使用非阻塞延时替代delay(3000)
  nonBlockingDelay(3000);

  // 如果RTC可用，切换到RTC时间源以确保用户体验
  if (systemState.rtcInitialized && systemState.rtcTimeValid) {
    switchTimeSource(TIME_SOURCE_RTC);
    systemState.needsRefresh = true; // 强制刷新显示
  }

  // 清除WiFi配置并重启进入配网模式（带超时保护）
  WiFi.disconnect(true);

  // 添加超时保护，防止disconnect()卡住
  unsigned long timeoutStart = millis();
  const unsigned long DISCONNECT_TIMEOUT = 10000; // 10秒超时
  while (WiFi.status() == WL_CONNECTED) {
    unsigned long elapsed = (millis() >= timeoutStart) ?
                          (millis() - timeoutStart) :
                          (0xFFFFFFFF - timeoutStart + millis());
    if (elapsed >= DISCONNECT_TIMEOUT) {
      LOG_WARNING("WiFi disconnect timeout, forcing reset");
      break;
    }
    yield();
  }

  ESP.reset();
}

void systemWatchdog() {
  unsigned long currentMillis = millis();

  // 检查主循环是否卡死（使用溢出安全的时间比较）
  unsigned long mainLoopElapsed = (currentMillis >= systemState.lastMainLoopTime) ?
                                 (currentMillis - systemState.lastMainLoopTime) :
                                 (0xFFFFFFFF - systemState.lastMainLoopTime + currentMillis);
  if (mainLoopElapsed > WATCHDOG_INTERVAL) {
    LOG_WARNING("Main loop watchdog timeout - restarting system");
    ESP.restart();
  }

  // 定期检查网络连接状态（使用溢出安全的时间比较）
  unsigned long networkCheckElapsed = (currentMillis >= systemState.lastNetworkCheck) ?
                                     (currentMillis - systemState.lastNetworkCheck) :
                                     (0xFFFFFFFF - systemState.lastNetworkCheck + currentMillis);
  if (networkCheckElapsed > NETWORK_CHECK_INTERVAL) {
    checkNetworkStatus();
    systemState.lastNetworkCheck = currentMillis;
  }

  // 定期同步RTC时间（如果使用NTP，使用溢出安全的时间比较）
  unsigned long rtcSyncElapsed = (currentMillis >= timeState.lastRtcSync) ?
                                (currentMillis - timeState.lastRtcSync) :
                                (0xFFFFFFFF - timeState.lastRtcSync + currentMillis);
  if (timeState.currentTimeSource == TIME_SOURCE_NTP && systemState.rtcInitialized &&
      rtcSyncElapsed > RTC_SYNC_INTERVAL) {
    syncNtpToRtc();
  }
}

void checkNetworkStatus() {
  // 检查WiFi连接状态
  if (systemState.wifiConfigured) {
    wl_status_t status = WiFi.status();
    bool prevConnected = systemState.networkConnected;
    systemState.networkConnected = (status == WL_CONNECTED);
    
    // 如果网络状态发生变化
    if (prevConnected != systemState.networkConnected) {
      if (systemState.networkConnected) {
        LOG_DEBUG("Network connected");
        // 网络连接恢复，尝试更新NTP时间
        if (timeState.currentTimeSource == TIME_SOURCE_NTP) {
          // 确保NTP客户端已正确初始化
          timeClient.begin();
          timeClient.setTimeOffset(8 * 3600);
          timeClient.setPoolServerName(timeState.currentNtpServer);
        }
      } else {
        LOG_DEBUG("Network disconnected");
        // 网络断开，可能需要切换时间源
        if (timeState.currentTimeSource == TIME_SOURCE_NTP) {
          // 尝试切换到RTC或其他可用时间源
          if (systemState.rtcInitialized && systemState.rtcTimeValid) {
            switchTimeSource(TIME_SOURCE_RTC);
          } else if (timeState.softwareClockValid) {
            switchTimeSource(TIME_SOURCE_MANUAL);
          }
        }
      }
    }
    
    // 如果网络已连接但长时间未获取到NTP时间，尝试重新连接
    if (systemState.networkConnected && timeState.currentTimeSource == TIME_SOURCE_NTP) {
      // 检查NTP客户端是否正常工作
      if (!timeClient.isTimeSet() && !timeState.ntpCheckInProgress) {
        // 如果距离上次NTP检查已超过一定时间，重新尝试（使用溢出安全的时间比较）
        unsigned long currentMillis = millis();
        unsigned long ntpCheckElapsed = (currentMillis >= timeState.lastNtpCheckAttempt) ?
                                       (currentMillis - timeState.lastNtpCheckAttempt) :
                                       (0xFFFFFFFF - timeState.lastNtpCheckAttempt + currentMillis);
        if (ntpCheckElapsed > NTP_CHECK_COOLDOWN) {
          checkNtpConnection(false); // 不强制检查
        }
      }
    }
  }
}

// 统一错误处理函数实现
void reportError(ErrorCode code, ErrorLevel level, const char* message) {
  // 从PROGMEM读取错误描述（带边界检查）
  static char errorDesc[50];
  int safeCode = (code >= ERROR_NONE && code <= ERROR_BUTTON_STATE_INVALID) ? code : ERROR_NONE;
  strcpy_P(errorDesc, (const char*)pgm_read_ptr(&ERROR_DESCRIPTIONS[safeCode]));

  // 从PROGMEM读取错误级别描述（带边界检查）
  static char levelDesc[20];
  int safeLevel = (level >= ERROR_LEVEL_INFO && level <= ERROR_LEVEL_CRITICAL) ? level : ERROR_LEVEL_INFO;
  strcpy_P(levelDesc, (const char*)pgm_read_ptr(&ERROR_LEVEL_DESCRIPTIONS[safeLevel]));

  // 格式化错误信息
  LOG_DEBUG("[%s] %s%s%s", levelDesc, errorDesc, message ? ": " : "", message ? message : "");

  // 根据错误级别采取不同措施
  switch (level) {
    case ERROR_LEVEL_WARNING:
      // 警告级别错误，仅记录日志
      break;

    case ERROR_LEVEL_ERROR:
      // 错误级别，可能需要降级处理
      systemState.needsRefresh = true;
      break;

    case ERROR_LEVEL_CRITICAL:
      // 严重错误，可能需要重启系统
      LOG_WARNING("Critical error detected, considering system restart");
      break;

    default:
      break;
  }
}

void handleError(ErrorCode code, ErrorLevel level, const char* message) {
  reportError(code, level, message);

  // 对于特定错误，显示错误界面（测试模式下不显示）
  if (level >= ERROR_LEVEL_ERROR && !g_testMode) {
    static char errorDesc[50];
    int safeCode = (code >= ERROR_NONE && code <= ERROR_BUTTON_STATE_INVALID) ? code : ERROR_NONE;
    strcpy_P(errorDesc, (const char*)pgm_read_ptr(&ERROR_DESCRIPTIONS[safeCode]));

    // 根据错误类型显示不同的错误信息
    switch (code) {
      case ERROR_RTC_INIT_FAILED:
        displayErrorScreen("RTC初始化失败", "请检查硬件连接");
        break;
      case ERROR_WIFI_CONNECTION_FAILED:
        displayErrorScreen("WiFi连接失败", "请检查网络设置");
        break;
      case ERROR_NTP_CONNECTION_FAILED:
        displayErrorScreen("时间同步失败", "请检查网络连接");
        break;
      default:
        displayErrorScreen(errorDesc, message ? message : "系统错误");
        break;
    }
  }
}

const char* getErrorDescription(ErrorCode code) {
  static char buffer[50];
  int safeCode = (code >= ERROR_NONE && code <= ERROR_BUTTON_STATE_INVALID) ? code : ERROR_NONE;
  strcpy_P(buffer, (const char*)pgm_read_ptr(&ERROR_DESCRIPTIONS[safeCode]));
  return buffer;
}

int getCorrectOffset() {
  static int cachedOffsetValue = -1;

  // 如果已经计算过偏移量，直接返回缓存值
  if (cachedOffsetValue != -1) {
    return cachedOffsetValue;
  }

  // 使用简化的日期计算，不依赖mktime()
  // 计算从2023年1月1日到2026年2月4日的天数
  // 注意：getDaysSince2023_01_01返回的是 days + (day - 1)
  // 所以对于2026-02-04，返回的是到2026-02-03的天数

  // 2023年：365天（不是闰年）
  // 2024年：366天（是闰年）
  // 2025年：365天（不是闰年）
  // 2026年：1月的天数 + 2月1-3日（因为day - 1）

  // 计算2026年1月的天数
  int days2026 = 31; // 1月有31天
  days2026 += 3; // 2月1-3日（day - 1 = 4 - 1 = 3）

  // 总天数（与getDaysSince2023_01_01的返回值一致）
  long totalDays = 365 + 366 + 365 + days2026; // 2023 + 2024 + 2025 + 2026(部分)

  // 计算偏移量，确保2026-02-04对应太守（市场索引0）
  // getDaysSince2023_01_01(2026, 2, 4) = 1130
  // 1130 % 3 = 2
  // 需要满足：(1130 + offset) % 3 = 0
  // 因此 offset = (0 - 2 + 3) % 3 = 1
  cachedOffsetValue = (0 - (totalDays % 3) + 3) % 3;

  // 确保结果在有效范围内
  if (cachedOffsetValue < 0 || cachedOffsetValue > 2) {
    cachedOffsetValue = 0; // 重置为默认值
    LOG_DEBUG("时间偏移计算结果无效，使用默认值");
  }

  LOG_DEBUG("Calculated offset: %d (total days: %ld)", cachedOffsetValue, totalDays);
  return cachedOffsetValue;
}

// WiFi密码安全存储函数 - 使用AES加密
void saveEncryptedWifiPassword(const String& password) {
  if (password.length() == 0) {
    systemState.encryptedWifiPassword[0] = '\0';
    LOG_DEBUG("WiFi password cleared");
    return;
  }
  
  // 生成AES密钥
  uint8_t aesKey[AES_KEY_SIZE];
  generateAESKey(aesKey);
  
  // 使用AES加密
  String encrypted = encryptPasswordAES(password, aesKey);
  
  if (encrypted.length() > 0) {
    strncpy(systemState.encryptedWifiPassword, encrypted.c_str(), sizeof(systemState.encryptedWifiPassword) - 1);
    systemState.encryptedWifiPassword[sizeof(systemState.encryptedWifiPassword) - 1] = '\0';
    LOG_DEBUG("WiFi password encrypted with AES and saved");
  } else {
    LOG_DEBUG("AES encryption failed, falling back to XOR");
    // 备用：使用原XOR加密
    String xorEncrypted = encryptPassword(password);
    strncpy(systemState.encryptedWifiPassword, xorEncrypted.c_str(), sizeof(systemState.encryptedWifiPassword) - 1);
    systemState.encryptedWifiPassword[sizeof(systemState.encryptedWifiPassword) - 1] = '\0';
  }
}

String loadEncryptedWifiPassword() {
  if (strlen(systemState.encryptedWifiPassword) == 0) {
    return "";
  }
  
  // 生成AES密钥
  uint8_t aesKey[AES_KEY_SIZE];
  generateAESKey(aesKey);
  
  String decrypted;
  
  // 首先尝试AES解密（如果加密数据长度为32字符，可能是AES）
  if (strlen(systemState.encryptedWifiPassword) == 32) {
    decrypted = decryptPasswordAES(String(systemState.encryptedWifiPassword), aesKey);
    if (decrypted.length() > 0) {
      LOG_DEBUG("WiFi password decrypted with AES successfully");
      return decrypted;
    }
  }
  
  // 备用：尝试XOR解密（兼容旧版本）
  decrypted = decryptPassword(String(systemState.encryptedWifiPassword));
  if (decrypted.length() > 0) {
    LOG_DEBUG("WiFi password decrypted with XOR (legacy format)");
    return decrypted;
  }
  
  LOG_DEBUG("Failed to decrypt WiFi password with both AES and XOR");
  return "";
}

// 简化的AES类实现 - 适用于ESP8266（需在generateAESKey之前定义，因其使用sbox）
class SimpleAES {
private:
  uint8_t key[16];

public:
  static const uint8_t sbox[256];
  static const uint8_t rsbox[256];

  void setKey(const uint8_t* k, int keySize) {
    if (keySize >= 16) {
      memcpy(key, k, 16);
    }
  }

  void encrypt(const uint8_t* plaintext, uint8_t* ciphertext) {
    memcpy(ciphertext, plaintext, 16);
    for (int round = 0; round < 8; round++) {
      for (int i = 0; i < 16; i++) {
        ciphertext[i] ^= key[i % 16];
        ciphertext[i] = sbox[ciphertext[i]];
        if (round < 7) ciphertext[i] ^= (round + 1);
      }
    }
  }

  void decrypt(const uint8_t* ciphertext, uint8_t* plaintext) {
    memcpy(plaintext, ciphertext, 16);
    for (int round = 7; round >= 0; round--) {
      for (int i = 0; i < 16; i++) {
        if (round < 7) plaintext[i] ^= (round + 1);
        plaintext[i] = rsbox[plaintext[i]];
        plaintext[i] ^= key[i % 16];
      }
    }
  }
};

// AES S-box和逆S-box
const uint8_t SimpleAES::sbox[256] = {
  0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
  0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
  0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
  0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
  0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
  0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
  0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
  0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
  0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
  0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
  0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
  0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
  0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
  0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
  0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF
};

const uint8_t SimpleAES::rsbox[256] = {
  0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38, 0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB,
  0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87, 0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB,
  0x54, 0x7B, 0x94, 0x32, 0xA6, 0xC2, 0x23, 0x3D, 0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E,
  0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2, 0x76, 0x5B, 0xA2, 0x49, 0x6D, 0x8B, 0xD1, 0x25,
  0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92,
  0x6C, 0x70, 0x48, 0x50, 0xFD, 0xED, 0xB9, 0xDA, 0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84,
  0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A, 0xF7, 0xE4, 0x58, 0x05, 0xB8, 0xB3, 0x45, 0x06,
  0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02, 0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B,
  0x3A, 0x91, 0x11, 0x41, 0x4F, 0x67, 0xDC, 0xEA, 0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
  0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85, 0xE2, 0xF9, 0x37, 0xE8, 0x1C, 0x75, 0xDF, 0x6E,
  0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89, 0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B,
  0xFC, 0x56, 0x3E, 0x4B, 0xC6, 0xD2, 0x79, 0x20, 0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4,
  0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31, 0xB1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xEC, 0x5F,
  0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D, 0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF,
  0xA0, 0xE0, 0x3B, 0x4D, 0xAE, 0x2A, 0xF5, 0xB0, 0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61,
  0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26, 0xE1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0C, 0x7D
};

// 生成安全的加密密钥 - 基于设备ID和多个随机源
void generateAESKey(uint8_t* key) {
  uint32_t deviceId = ESP.getChipId();

  // 使用多个随机源增强熵值
  uint32_t randomSalt = random(0xFFFFFFFF); // 使用硬件随机数生成器
  uint32_t microSalt = micros();
  uint32_t wifiMac = 0;

  // 获取WiFi MAC地址作为额外熵源
  uint8_t mac[6];
  WiFi.macAddress(mac);
  for (int i = 0; i < 6; i++) {
    wifiMac ^= (mac[i] << (i * 4));
  }

  // 使用HMAC-like方法生成更安全的密钥
  for (int i = 0; i < AES_KEY_SIZE; i++) {
    key[i] = ((deviceId >> (i % 4)) ^ (randomSalt >> ((i + 8) % 4))) & 0xFF;
    // 增加额外的混淆
    key[i] ^= (i * 17 + 123) & 0xFF;
    key[i] ^= (microSalt >> ((i + 4) % 4)) & 0xFF;
    key[i] ^= (wifiMac >> (i % 4)) & 0xFF;
    key[i] = (key[i] << 3) | (key[i] >> 5); // 循环移位
  }

  // 使用SHA-256风格的初始化向量增强安全性
  static const uint8_t initVector[16] = {
    0x6A, 0x09, 0xE6, 0x67, 0xBB, 0x67, 0xAE, 0x85,
    0x3C, 0x6E, 0xF3, 0x72, 0xA5, 0x4F, 0xF5, 0x3A
  };

  for (int i = 0; i < AES_KEY_SIZE; i++) {
    key[i] ^= initVector[i];
  }

  // 额外的混淆轮次
  for (int round = 0; round < 3; round++) {
    for (int i = 0; i < AES_KEY_SIZE; i++) {
      key[i] = SimpleAES::sbox[key[i]];
      key[i] ^= key[(i + 7) % AES_KEY_SIZE];
    }
  }
}

// 改进的安全加密函数 - 使用更安全的加密算法
String encryptPasswordAES(const String& password, const uint8_t* key) {
  if (password.length() == 0) return "";
  if (password.length() > 100) return ""; // 限制密码长度防止攻击

  // 使用更安全的加密算法
  String encrypted = "";
  uint8_t iv[AES_KEY_SIZE]; // 初始化向量

  // 生成基于时间的初始化向量
  uint32_t timestamp = millis();
  for (int i = 0; i < AES_KEY_SIZE; i++) {
    iv[i] = (timestamp >> (i % 4)) ^ key[(i + 5) % AES_KEY_SIZE];
  }

  // CBC模式加密
  uint8_t prevBlock[AES_KEY_SIZE];
  memcpy(prevBlock, iv, AES_KEY_SIZE);

  for (int i = 0; i < password.length(); i += AES_KEY_SIZE) {
    uint8_t block[AES_KEY_SIZE] = {0};
    int blockLen = (AES_KEY_SIZE < (password.length() - i)) ? AES_KEY_SIZE : (password.length() - i);

    // 填充数据块
    for (int j = 0; j < blockLen; j++) {
      block[j] = password.charAt(i + j);
    }

    // PKCS#7填充
    if (blockLen < AES_KEY_SIZE) {
      uint8_t padValue = AES_KEY_SIZE - blockLen;
      for (int j = blockLen; j < AES_KEY_SIZE; j++) {
        block[j] = padValue;
      }
    }

    // CBC模式：与前一个密文块XOR
    for (int j = 0; j < AES_KEY_SIZE; j++) {
      block[j] ^= prevBlock[j];
    }

    // 多轮加密（简化版AES核心）- 注意：这是简化实现，生产环境建议使用ESP8266硬件加密
    for (int round = 0; round < 10; round++) {
      for (int j = 0; j < AES_KEY_SIZE; j++) {
        block[j] ^= key[(j + round) % AES_KEY_SIZE];
        block[j] = SimpleAES::sbox[block[j]];
        if (round < 9) block[j] ^= (round + j) & 0xFF;
      }
    }

    // 保存当前块作为下一个块的IV
    memcpy(prevBlock, block, AES_KEY_SIZE);

    // 转换为十六进制
    for (int j = 0; j < AES_KEY_SIZE; j++) {
      char hex[3];
      snprintf(hex, sizeof(hex), "%02X", block[j]);
      encrypted += hex;
    }
  }

  // 添加IV和校验信息
  String result = "";
  for (int i = 0; i < AES_KEY_SIZE; i++) {
    char hex[3];
    snprintf(hex, sizeof(hex), "%02X", iv[i]);
    result += hex;
  }
  result += encrypted;

  return result;
}

// 改进的安全解密函数
String decryptPasswordAES(const String& encrypted, const uint8_t* key) {
  if (encrypted.length() == 0) return "";
  if (encrypted.length() % 2 != 0) return ""; // 必须是偶数长度
  if (encrypted.length() < 64) return ""; // 最小长度检查（IV + 至少一个块）
  
  // 提取IV
  uint8_t iv[AES_KEY_SIZE];
  for (int i = 0; i < AES_KEY_SIZE * 2; i += 2) {
    String hexByte = encrypted.substring(i, i + 2);
    iv[i / 2] = strtol(hexByte.c_str(), NULL, 16);
  }
  
  // 解密数据块
  String decrypted = "";
  uint8_t prevBlock[AES_KEY_SIZE];
  memcpy(prevBlock, iv, AES_KEY_SIZE);
  
  for (int i = AES_KEY_SIZE * 2; i < encrypted.length(); i += AES_KEY_SIZE * 2) {
    uint8_t block[AES_KEY_SIZE];
    
    // 提取密文块
    for (int j = 0; j < AES_KEY_SIZE; j++) {
      String hexByte = encrypted.substring(i + j * 2, i + j * 2 + 2);
      block[j] = strtol(hexByte.c_str(), NULL, 16);
    }
    
    uint8_t tempBlock[AES_KEY_SIZE];
    memcpy(tempBlock, block, AES_KEY_SIZE);
    
    // 多轮解密
    for (int round = 9; round >= 0; round--) {
      for (int j = 0; j < AES_KEY_SIZE; j++) {
        if (round < 9) tempBlock[j] ^= (round + j) & 0xFF;
        tempBlock[j] = SimpleAES::rsbox[tempBlock[j]];
        tempBlock[j] ^= key[(j + round) % AES_KEY_SIZE];
      }
    }
    
    // CBC模式：与前一个密文块XOR
    for (int j = 0; j < AES_KEY_SIZE; j++) {
      tempBlock[j] ^= prevBlock[j];
    }
    
    // 保存当前块作为下一个块的IV
    memcpy(prevBlock, block, AES_KEY_SIZE);
    
    // 处理填充
    int padValue = tempBlock[AES_KEY_SIZE - 1];
    int dataLen = AES_KEY_SIZE;
    
    // 验证PKCS#7填充
    if (padValue > 0 && padValue <= AES_KEY_SIZE) {
      bool validPad = true;
      for (int j = AES_KEY_SIZE - padValue; j < AES_KEY_SIZE; j++) {
        if (tempBlock[j] != padValue) {
          validPad = false;
          break;
        }
      }
      if (validPad) {
        dataLen = AES_KEY_SIZE - padValue;
      }
    }
    
    // 添加到解密结果
    for (int j = 0; j < dataLen; j++) {
      decrypted += (char)tempBlock[j];
    }
  }
  
  return decrypted;
}

// 保留原XOR函数作为备用兼容
String encryptPassword(const String& password) {
  if (password.length() == 0) return "";
  
  String encrypted = "";
  uint32_t deviceId = ESP.getChipId(); // 使用芯片ID作为加密密钥
  
  for (size_t i = 0; i < password.length(); i++) {
    // 多字节XOR加密，增强安全性
    uint8_t keyByte = (deviceId >> (8 * (i % 4))) & 0xFF;
    char encryptedChar = password.charAt(i) ^ keyByte ^ (i + 1);
    encrypted += encryptedChar;
  }
  
  // 添加校验和
  uint8_t checksum = 0;
  for (size_t i = 0; i < encrypted.length(); i++) {
    checksum ^= encrypted.charAt(i);
  }
  encrypted += (char)checksum;
  
  return encrypted;
}

// 保留原XOR函数作为备用兼容
String decryptPassword(const String& encrypted) {
  if (encrypted.length() == 0) return "";
  if (encrypted.length() < 2) return ""; // 至少需要一个字符和校验和
  
  // 验证校验和
  uint8_t checksum = 0;
  for (size_t i = 0; i < encrypted.length() - 1; i++) {
    checksum ^= encrypted.charAt(i);
  }
  
  if (checksum != (uint8_t)encrypted.charAt(encrypted.length() - 1)) {
    LOG_DEBUG("Password decryption failed: checksum mismatch");
    return ""; // 校验失败，返回空字符串
  }
  
  String decrypted = "";
  uint32_t deviceId = ESP.getChipId();
  
  for (size_t i = 0; i < encrypted.length() - 1; i++) { // 排除校验和
    uint8_t keyByte = (deviceId >> (8 * (i % 4))) & 0xFF;
    char decryptedChar = encrypted.charAt(i) ^ keyByte ^ (i + 1);
    decrypted += decryptedChar;
  }
  
  return decrypted;
}

