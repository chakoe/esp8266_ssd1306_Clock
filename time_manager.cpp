#include "time_manager.h"
#include "system_manager.h"
#include "display_manager.h"
#include "utils.h"
#include <Wire.h>
#include <time.h>
#include "config.h"
#include "logger.h"

// 外部变量声明
extern SystemState systemState;
extern DisplayState displayState;
extern SettingState settingState;
extern TimeState timeState;
extern NTPClient timeClient;
extern RTC_DS1307 rtc;

// NTP服务器列表声明 - 现在统一在global_config.cpp中定义
extern const char* const NTP_SERVERS[];
extern const int NTP_SERVER_COUNT;

// 辅助函数声明
bool isLeapYear(int year);
int getDaysInMonth(int month, int year);
void ensureNtpClientInitialized();

extern NTPClient timeClient;
extern RTC_DS1307 rtc;

bool initializeRTC() {
  // 尝试初始化RTC (添加I2C错误检测)
  Wire.beginTransmission(0x68); // DS1307 I2C地址
  byte error = Wire.endTransmission();
  if (error != 0) {
    systemState.rtcInitialized = false;
    static char errorMsg[50];
    snprintf(errorMsg, sizeof(errorMsg), "I2C错误代码: %d", error);
    handleError(ERROR_RTC_I2C_ERROR, ERROR_LEVEL_ERROR, errorMsg);
    return false;
  }
  
  if (!rtc.begin()) {
    systemState.rtcInitialized = false;
    handleError(ERROR_RTC_INIT_FAILED, ERROR_LEVEL_ERROR, "RTC.begin() failed");
    return false;
  }
  
  systemState.rtcInitialized = true;
  
  // 禁用DS1307的SQW输出，避免干扰
  rtc.writeSqwPinMode(DS1307_OFF);
  
  // 检查RTC是否正在运行
  if (!rtc.isrunning()) {
    // 如果RTC没有运行，尝试设置初始时间
    // 首先尝试使用编译时间
    DateTime compileTime(F(__DATE__), F(__TIME__));
    
    // 检查编译时间是否合理（大于2020年）
    if (compileTime.year() >= 2020) {
      // rtc.adjust()函数返回void，无法检查返回值
      rtc.adjust(compileTime);
      LOG_INFO("RTC was not running, set to compilation time");
    } else {
      // 编译时间不合理，设置一个默认时间
      // rtc.adjust()函数返回void，无法检查返回值
      rtc.adjust(DateTime(2023, 1, 1, 12, 0, 0));
      LOG_INFO("RTC was not running, set to default time");
    }
  }
  
  // 验证RTC时间是否有效
  DateTime rtcTime = rtc.now();
  systemState.rtcTimeValid = isRtcTimeValid(rtcTime);
  
  if (!systemState.rtcTimeValid) {
    static char timeErrorMsg[60];
    snprintf(timeErrorMsg, sizeof(timeErrorMsg), "Invalid RTC time: %04d-%02d-%02d %02d:%02d:%02d", 
             rtcTime.year(), rtcTime.month(), rtcTime.day(), 
             rtcTime.hour(), rtcTime.minute(), rtcTime.second());
    handleError(ERROR_RTC_TIME_INVALID, ERROR_LEVEL_WARNING, timeErrorMsg);
  }
  
  // 打印RTC状态
  LOG_DEBUG("RTC time: %04d/%02d/%02d %02d:%02d:%02d", 
         rtcTime.year(), rtcTime.month(), rtcTime.day(), 
         rtcTime.hour(), rtcTime.minute(), rtcTime.second());
  
  return systemState.rtcTimeValid;
}

// 辅助函数：判断是否为闰年
bool isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// 辅助函数：获取指定月份的天数
int getDaysInMonth(int month, int year) {
  if (month == 2) {
    return isLeapYear(year) ? 29 : 28;
  } else if (month == 4 || month == 6 || month == 9 || month == 11) {
    return 30;
  }
  return 31;
}

bool isRtcTimeValid(const DateTime& rtcTime) {
  // 检查时间是否在合理范围内
  if (rtcTime.year() < 2020 || rtcTime.year() > 2099) {
    LOG_DEBUG("Invalid year: %d", rtcTime.year());
    return false;
  }
  
  // 检查月份
  if (rtcTime.month() < 1 || rtcTime.month() > 12) {
    LOG_DEBUG("Invalid month: %d", rtcTime.month());
    return false;
  }
  
  // 检查日期
  int daysInMonth = getDaysInMonth(rtcTime.month(), rtcTime.year());
  
  if (rtcTime.day() < 1 || rtcTime.day() > daysInMonth) {
    LOG_DEBUG("Invalid day: %d", rtcTime.day());
    return false;
  }
  
  // 检查时间
  if (rtcTime.hour() > 23 || rtcTime.minute() > 59 || rtcTime.second() > 59) {
    LOG_DEBUG("Invalid time: %02d:%02d:%02d", rtcTime.hour(), rtcTime.minute(), rtcTime.second());
    return false;
  }
  
  // 额外的安全检查：验证时间戳是否在合理范围内
  if (rtcTime.unixtime() < 1577836800UL || rtcTime.unixtime() > 2524608000UL) { // 2020-01-01 到 2050-01-01
    LOG_DEBUG("Time outside valid range: %lu", rtcTime.unixtime());
    return false;
  }
  
  return true;
}

// 确保NTP客户端正确初始化的辅助函数（仅在必要时调用）
void ensureNtpClientInitialized() {
  // 只负责初始化NTP客户端，不设置ntpCheckInProgress标志
  // 该标志由调用者checkNtpConnection()管理
  if (!timeClient.isTimeSet()) {
    timeClient.end();
    // 使用非阻塞延时替代delay(10)
    nonBlockingDelay(10);
    timeClient.begin();
    timeClient.setTimeOffset(8 * 3600); // 设置北京时间
    timeClient.setPoolServerName(timeState.currentNtpServer);
    // 使用非阻塞延时替代delay(50)
    nonBlockingDelay(50);
  }
}

bool checkNtpConnection(bool forceCheck) {
  if (!systemState.wifiConfigured || WiFi.status() != WL_CONNECTED) {
    LOG_DEBUG("WiFi not connected for NTP");
    return false;
  }

  // 检查并重置可能卡住的ntpCheckInProgress标志
  unsigned long currentMillis = millis();
  if (timeState.ntpCheckInProgress) {
    unsigned long checkElapsed = (currentMillis >= timeState.ntpCheckStartTime) ?
                                (currentMillis - timeState.ntpCheckStartTime) :
                                (0xFFFFFFFF - timeState.ntpCheckStartTime + currentMillis);
    if (checkElapsed >= NTP_CHECK_TIMEOUT) {
      LOG_WARNING("NTP check timeout, resetting in-progress flag");
      timeState.ntpCheckInProgress = false;
      timeState.ntpCheckStartTime = 0;
    } else {
      LOG_DEBUG("NTP check already in progress");
      return false;
    }
  }

  // 防止频繁检查NTP，设置冷却时间（除非强制检查）
  if (!forceCheck) {
    // 使用溢出安全的时间比较
    unsigned long ntpCheckElapsed = (currentMillis >= timeState.lastNtpCheckAttempt) ?
                                   (currentMillis - timeState.lastNtpCheckAttempt) :
                                   (0xFFFFFFFF - timeState.lastNtpCheckAttempt + currentMillis);
    if (ntpCheckElapsed < NTP_CHECK_COOLDOWN) {
      LOG_DEBUG("NTP check in cooldown period");
      return false;
    }
  }

  timeState.lastNtpCheckAttempt = currentMillis;

  // 确保NTP客户端已正确初始化
  ensureNtpClientInitialized();

  // 标记正在检查并记录开始时间
  timeState.ntpCheckInProgress = true;
  timeState.ntpCheckStartTime = currentMillis;

  // 尝试获取NTP时间
  bool success = false;

  // 使用当前NTP服务器（先尝试当前，失败时下次会轮换）
  strncpy_P(timeState.currentNtpServer, (const char*)pgm_read_ptr(&NTP_SERVERS[timeState.currentNtpServerIndex]), sizeof(timeState.currentNtpServer) - 1);
  timeState.currentNtpServer[sizeof(timeState.currentNtpServer) - 1] = '\0';
  timeClient.setPoolServerName(timeState.currentNtpServer);

  LOG_DEBUG("Trying NTP server: %s", timeState.currentNtpServer);

  // 使用非阻塞方式更新NTP时间，避免长时间阻塞
  // 虽然这里已经在checkNtpConnection函数中，但仍需要确保不会造成额外阻塞
  timeClient.update();
  // 在NTP检查后调用yield()，确保按键响应
  yield();

  // 检查NTP时间是否有效
  if (timeClient.isTimeSet()) {
    time_t ntpTime = timeClient.getEpochTime();
    LOG_DEBUG(" -> Got timestamp: %lu", ntpTime);

    // 验证时间戳的合理性
    if (ntpTime < 1577836800UL || ntpTime > 2524608000UL) { // 2020-01-01 到 2050-01-01
      LOG_DEBUG("NTP time out of range");
      timeState.ntpCheckInProgress = false;
      timeState.ntpCheckStartTime = 0;
      return false;
    }

    success = true;
  } else {
    LOG_DEBUG(" -> Failed");
    // 记录NTP连接失败，并轮换到下一个服务器供下次尝试
    timeState.ntpFailCount++;
    timeState.currentNtpServerIndex = (timeState.currentNtpServerIndex + 1) % NTP_SERVER_COUNT;
    static char ntpErrorMsg[60];
    snprintf(ntpErrorMsg, sizeof(ntpErrorMsg), "NTP连接失败，服务器: %s", timeState.currentNtpServer);
    if (timeState.ntpFailCount >= NTP_SERVER_COUNT) {
      handleError(ERROR_NTP_CONNECTION_FAILED, ERROR_LEVEL_WARNING, "所有NTP服务器均失败");
      timeState.ntpFailCount = 0;
    } else {
      handleError(ERROR_NTP_CONNECTION_FAILED, ERROR_LEVEL_WARNING, ntpErrorMsg);
    }
  }

  timeState.ntpCheckInProgress = false;
  timeState.ntpCheckStartTime = 0;

  // 在NTP检查后调用yield()，确保按键响应
  yield();

  return success;
}

void setupTimeSources() {
  // 智能时间源选择策略：RTC > NTP > 手动设置
  LOG_DEBUG("Setting up time sources with intelligent fallback...");
  
  // 第一优先级：尝试使用RTC
  if (systemState.rtcInitialized && systemState.rtcTimeValid) {
    switchTimeSource(TIME_SOURCE_RTC);
    LOG_DEBUG("✓ RTC available, using as primary time source");
    return;
  }
  
  // 第二优先级：RTC不可用，尝试使用NTP
  // 即使RTC初始化失败，也要设置NTP为时间源，因为网络可能随时恢复
  if (systemState.networkConnected) {
    LOG_DEBUG("RTC not available, trying NTP as backup...");
    
    // 确保NTP客户端已初始化（重新初始化以确保可靠性）
    timeClient.end();
    // 使用非阻塞延时替代delay(100)
    nonBlockingDelay(100);
    timeClient.begin();
    timeClient.setTimeOffset(8 * 3600);
    timeClient.setPoolServerName(timeState.currentNtpServer);
    LOG_DEBUG("NTP client initialized for backup time source");
    
    // 尝试获取NTP时间
    if (checkNtpConnection(false)) {
      // NTP连接成功，切换到NTP时间源
      switchTimeSource(TIME_SOURCE_NTP);
      LOG_DEBUG("✓ NTP available, using as time source");
      return;
    } else {
      LOG_DEBUG("✗ Failed to get NTP response");
    }
  } else {
    LOG_DEBUG("Network not available for NTP");
  }
  
  // 第三优先级：都不行，使用软件时钟（如果有效）或NTP（如果网络可用）
  if (timeState.softwareClockValid) {
    switchTimeSource(TIME_SOURCE_MANUAL);
    LOG_DEBUG("✓ Using software clock as fallback");
  } else if (systemState.networkConnected) {
    // 即使NTP获取失败，也尝试使用NTP（可能后续会成功）
    switchTimeSource(TIME_SOURCE_NTP);
    LOG_DEBUG("⚠ All time sources failed, trying NTP as fallback");
  } else if (systemState.rtcInitialized && systemState.rtcTimeValid) {
    switchTimeSource(TIME_SOURCE_RTC);
    LOG_DEBUG("⚠ All time sources failed, RTC as fallback");
  } else {
    // 即使当前没有可用的时间源，也要设置为NTP模式
    // 这样一旦网络恢复，系统可以自动获取时间
    switchTimeSource(TIME_SOURCE_NTP);
    LOG_DEBUG("⚠ No time sources available, defaulting to NTP mode");
  }
}

bool getCurrentTimeFromNtp(DateTime& now) {
  if (!systemState.networkConnected) {
    return false;
  }
  
  // 确保NTP客户端已正确初始化
  ensureNtpClientInitialized();
  
  // 检查NTP时间是否已设置（避免不必要的网络请求）
  if (timeClient.isTimeSet()) {
    time_t ntpTime = timeClient.getEpochTime();
    
    // 验证NTP时间有效性
    if (ntpTime > 1577836800UL && ntpTime < 2524608000UL) {
      struct tm *ntpTm = gmtime(&ntpTime);
      if (ntpTm != nullptr && ntpTm->tm_year >= 120) {
        // 创建NTP时间的DateTime对象
        now = DateTime(ntpTm->tm_year + 1900, ntpTm->tm_mon + 1, ntpTm->tm_mday,
                       ntpTm->tm_hour, ntpTm->tm_min, ntpTm->tm_sec);
        return true;
      }
    }
  }
  
  // 如果时间未设置，尝试更新（仅在必要时）
  // 使用非阻塞方式替代直接调用timeClient.update()
  if (!timeState.ntpCheckInProgress) {
    timeState.ntpCheckInProgress = true;
    timeClient.update();
    timeState.ntpCheckInProgress = false;
  }
  
  // 再次检查NTP时间是否有效
  if (timeClient.isTimeSet()) {
    time_t ntpTime = timeClient.getEpochTime();
    
    // 验证NTP时间有效性
    if (ntpTime > 1577836800UL && ntpTime < 2524608000UL) {
      struct tm *ntpTm = gmtime(&ntpTime);
      if (ntpTm != nullptr && ntpTm->tm_year >= 120) {
        // 创建NTP时间的DateTime对象
        now = DateTime(ntpTm->tm_year + 1900, ntpTm->tm_mon + 1, ntpTm->tm_mday,
                       ntpTm->tm_hour, ntpTm->tm_min, ntpTm->tm_sec);
        return true;
      }
    }
  }
  
  return false;
}

bool getCurrentTime(DateTime& now) {
  // 简化的时间获取逻辑，避免频繁重试和阻塞
  switch (timeState.currentTimeSource) {
    case TIME_SOURCE_NTP:
      return getCurrentTimeFromNtp(now);
      
    case TIME_SOURCE_RTC:
      if (systemState.rtcInitialized && systemState.rtcTimeValid) {
        now = rtc.now();
        return true;
      }
      return false;
      
    case TIME_SOURCE_MANUAL:
      if (timeState.softwareClockValid) {
        // 计算软件时钟当前时间（使用溢出安全的时间比较）
        unsigned long currentMillis = millis();
        unsigned long elapsed = (currentMillis >= timeState.softwareClockBase) ?
                              (currentMillis - timeState.softwareClockBase) :
                              (0xFFFFFFFF - timeState.softwareClockBase + currentMillis);
        // 转换为秒并加到基准时间上
        now = DateTime(timeState.softwareClockTime + elapsed / 1000);
        return true;
      }
      return false;
      
    case TIME_SOURCE_NONE:
    default:
      return false;
  }
}

void syncNtpToRtc() {
  if (!systemState.rtcInitialized) {
    LOG_DEBUG("RTC not initialized, cannot sync");
    handleError(ERROR_RTC_INIT_FAILED, ERROR_LEVEL_WARNING, "RTC not initialized, cannot sync to NTP");
    return;
  }

  if (!systemState.networkConnected) {
    LOG_DEBUG("Network not connected, cannot sync NTP");
    handleError(ERROR_NTP_CONNECTION_FAILED, ERROR_LEVEL_WARNING, "Network not connected, cannot sync NTP");
    return;
  }

  // 强制更新NTP时间（带超时保护）
  LOG_DEBUG("Syncing NTP time to RTC...");
  bool updateSuccess = false;
  if (!timeState.ntpCheckInProgress) {
    timeState.ntpCheckInProgress = true;

    // 设置超时时间（5秒）
    unsigned long timeoutStart = millis();
    const unsigned long SYNC_TIMEOUT = 5000;
    updateSuccess = false;

    // 尝试更新，但带超时检查
    while (!updateSuccess) {
      unsigned long elapsed = (millis() >= timeoutStart) ?
                            (millis() - timeoutStart) :
                            (0xFFFFFFFF - timeoutStart + millis());

      if (elapsed >= SYNC_TIMEOUT) {
        LOG_DEBUG("NTP sync timeout after %lu ms", elapsed);
        break;
      }

      // 尝试更新
      updateSuccess = timeClient.forceUpdate();
      yield(); // 让出控制权

      // 如果失败，短暂延迟后重试
      if (!updateSuccess) {
        nonBlockingDelay(100);

        // 检查网络连接是否仍然有效
        if (WiFi.status() != WL_CONNECTED) {
          LOG_DEBUG("Network disconnected during NTP sync");
          break;
        }
      }
    }

    timeState.ntpCheckInProgress = false;
    yield(); // 在NTP更新后让出控制权
  } else {
    LOG_DEBUG("NTP update in progress, skipping sync");
    return;
  }

  if (!updateSuccess) {
    LOG_DEBUG("Failed to update NTP time");
    handleError(ERROR_NTP_CONNECTION_FAILED, ERROR_LEVEL_WARNING, "Failed to update NTP time for sync");
    return;
  }
  
  // 获取NTP时间戳并验证有效性
  time_t ntpTime = timeClient.getEpochTime();
  
  // 检查时间戳有效性 (2020年到2050年之间)
  if (ntpTime < 1577836800UL || ntpTime > 2524608000UL) { // 2020-01-01 到 2050-01-01
    LOG_DEBUG("Invalid NTP timestamp: %lu", ntpTime);
    static char timeErrorMsg[50];
    snprintf(timeErrorMsg, sizeof(timeErrorMsg), "Invalid NTP timestamp: %lu", ntpTime);
    handleError(ERROR_TIME_SETTING_INVALID, ERROR_LEVEL_ERROR, timeErrorMsg);
    return;
  }
  
  // 使用gmtime替代localtime，避免时区和线程安全问题
  struct tm *ntpTm = gmtime(&ntpTime);
  
  if (ntpTm != nullptr && ntpTm->tm_year >= 120) { // tm_year从1900年开始，120表示2020年
    // 验证时间字段的合理性
    if (ntpTm->tm_mon >= 0 && ntpTm->tm_mon < 12 && 
        ntpTm->tm_mday >= 1 && ntpTm->tm_mday <= 31 &&
        ntpTm->tm_hour >= 0 && ntpTm->tm_hour < 24 &&
        ntpTm->tm_min >= 0 && ntpTm->tm_min < 60 &&
        ntpTm->tm_sec >= 0 && ntpTm->tm_sec < 60) {
      
      // 将NTP时间同步到DS1307
      DateTime rtcTime(ntpTm->tm_year + 1900, ntpTm->tm_mon + 1, ntpTm->tm_mday, 
                      ntpTm->tm_hour, ntpTm->tm_min, ntpTm->tm_sec);
      
      // 最后一次验证时间有效性
      if (isRtcTimeValid(rtcTime)) {
        // rtc.adjust()函数返回void，无法检查返回值
        rtc.adjust(rtcTime);
        systemState.rtcTimeValid = true;
        timeState.lastRtcSync = millis(); // 更新最后同步时间
        
        LOG_DEBUG("NTP time successfully synchronized to RTC: %04d/%02d/%02d %02d:%02d:%02d", 
               rtcTime.year(), rtcTime.month(), rtcTime.day(), 
               rtcTime.hour(), rtcTime.minute(), rtcTime.second());
      } else {
        LOG_DEBUG("RTC time validation failed after NTP sync");
        handleError(ERROR_RTC_TIME_INVALID, ERROR_LEVEL_ERROR, "RTC time validation failed after NTP sync");
      }
    } else {
      LOG_DEBUG("NTP time field validation failed");
      handleError(ERROR_TIME_SETTING_INVALID, ERROR_LEVEL_ERROR, "NTP time field validation failed");
    }
  } else {
    LOG_DEBUG("Failed to parse NTP time for RTC synchronization");
    handleError(ERROR_TIME_SETTING_INVALID, ERROR_LEVEL_ERROR, "Failed to parse NTP time for RTC synchronization");
  }
}

void switchTimeSource(TimeSource newSource) {
  if (timeState.currentTimeSource != newSource) {
    timeState.lastTimeSource = timeState.currentTimeSource;
    timeState.currentTimeSource = newSource;
    timeState.timeSourceChanged = true;
    timeState.lastTimeSourceSwitch = millis(); // 记录时间源切换时间
    
    // 根据新的时间源类型执行特定操作
    switch (newSource) {
      case TIME_SOURCE_NTP:
        // 确保NTP客户端已初始化
        ensureNtpClientInitialized();
        break;
        
      case TIME_SOURCE_RTC:
        // 确保RTC已初始化
        if (!systemState.rtcInitialized) {
          initializeRTC();
        }
        break;
        
      case TIME_SOURCE_MANUAL:
        // 软件时钟需要已设置
        if (!timeState.softwareClockValid) {
          // 如果软件时钟无效，尝试从其他源获取时间作为基准
          DateTime now;
          if (timeState.lastTimeSource != TIME_SOURCE_NONE && getCurrentTime(now)) {
            timeState.softwareClockTime = now.unixtime();
            timeState.softwareClockBase = millis();
            timeState.softwareClockValid = true;
          }
        }
        break;
        
      case TIME_SOURCE_NONE:
      default:
        break;
    }
    
    // 更新时间源状态显示
    const char* sourceName = getTimeSourceName(newSource);
    snprintf(displayState.timeSourceStatus, sizeof(displayState.timeSourceStatus), "时间源: %s", sourceName);
    
    LOG_DEBUG("Switched time source to: %s", sourceName);
  } else {
    timeState.timeSourceChanged = false;
  }
}

const char* getTimeSourceName(TimeSource source) {
  switch (source) {
    case TIME_SOURCE_NTP: return "NTP";
    case TIME_SOURCE_RTC: return "RTC";
    case TIME_SOURCE_MANUAL: return "CLK";
    case TIME_SOURCE_NONE: 
    default: return "NONE";
  }
}

// 时间源设置模式相关函数实现
void enterTimeSourceSettingMode() {
  settingState.timeSourceSettingMode = true;
  
  // 设置初始选择的时间源为当前时间源
  // 映射当前时间源到选择索引
  switch (timeState.currentTimeSource) {
    case TIME_SOURCE_NTP:
      settingState.selectedTimeSourceIndex = 0;
      break;
    case TIME_SOURCE_RTC:
      settingState.selectedTimeSourceIndex = 1;
      break;
    case TIME_SOURCE_MANUAL:
      settingState.selectedTimeSourceIndex = 2;
      break;
    default:
      settingState.selectedTimeSourceIndex = 0; // 默认选择NTP
      break;
  }
  
  LOG_DEBUG("Entered time source setting mode");
}

void exitTimeSourceSettingMode() {
  settingState.timeSourceSettingMode = false;
  // 应用选择的时间源
  // 正确映射选择索引到时间源枚举
  TimeSource selectedSource;
  switch (settingState.selectedTimeSourceIndex) {
    case 0: // NTP
      selectedSource = TIME_SOURCE_NTP;
      break;
    case 1: // RTC
      selectedSource = TIME_SOURCE_RTC;
      break;
    case 2: // CLK (软件时钟)
      selectedSource = TIME_SOURCE_MANUAL;
      break;
    default:
      selectedSource = TIME_SOURCE_NTP; // 默认选择NTP
      break;
  }
  
  LOG_DEBUG("Exiting time source setting mode, selected index: %d", settingState.selectedTimeSourceIndex);
  
  // 检查选择的时间源是否可用
  if (selectedSource == TIME_SOURCE_NTP && !systemState.networkConnected) {
    displayError("NTP不可用", "网络未连接", "请检查网络", nullptr);
    nonBlockingDelay(1000);  // 1秒足够阅读，减少阻塞时间
  } else if (selectedSource == TIME_SOURCE_RTC && !systemState.rtcInitialized) {
    displayError("RTC不可用", "硬件未连接", "请检查RTC", nullptr);
    nonBlockingDelay(1000);
  } else if (selectedSource == TIME_SOURCE_MANUAL && !timeState.softwareClockValid) {
    displayError("软件时钟", "未设置时间", "请先手动设置", nullptr);
    nonBlockingDelay(1000);
  } else {
    // 时间源可用，进行切换
    switchTimeSource(selectedSource);
    LOG_DEBUG("Switched to time source: %s", getTimeSourceName(timeState.currentTimeSource));
  }
}

void displayTimeSourceSettingScreen() {
  u8g2.clearBuffer();
  
  // 使用小字体
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  
  // 显示标题
  u8g2.drawUTF8(0, 12, "设置时间源");
  
  // 显示时间源选项
  const char* timeSources[] = {"NTP", "RTC", "CLK"};
  
  for (int i = 0; i < 3; i++) {
    int y = 25 + i * 14;
    if (i == settingState.selectedTimeSourceIndex) {
      u8g2.drawUTF8(2, y, "*");
      u8g2.drawUTF8(10, y, timeSources[i]);
    } else {
      u8g2.drawUTF8(10, y, timeSources[i]);
    }
  }
  
  u8g2.sendBuffer();
}

void selectNextTimeSource() {
  settingState.selectedTimeSourceIndex = (settingState.selectedTimeSourceIndex + 1) % 3; // 循环选择
  LOG_DEBUG("Selected time source index: %d", settingState.selectedTimeSourceIndex);
}
