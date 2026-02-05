#include "display_manager.h"
#include "time_manager.h"
#include "button_handler.h"
#include "system_manager.h"
#include "utils.h"
#include <math.h>
#include <RTClib.h>
#include <U8g2lib.h>
#include "logger.h"
#include "eeprom_config.h"

// 全局对象声明 - 现在统一在global_config.cpp中定义
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

// 外部变量声明 - 精简版本
extern SystemState systemState;
extern DisplayState displayState;
extern SettingState settingState;
extern TimeState timeState;
extern RTC_DS1307 rtc;
extern NTPClient timeClient;
extern const uint8_t BRIGHTNESS_LEVELS[];
extern const char* const BRIGHTNESS_LABELS[];
extern const char* const MARKET_DAYS[];
extern const char* const CN_WEEKDAYS[];
extern const unsigned long DISPLAY_UPDATE_INTERVAL;

// 全局常量声明 - 从global_config.h引入
extern const uint8_t BRIGHTNESS_LEVELS[];
extern const char* const BRIGHTNESS_LABELS[];
extern const char* const MARKET_DAYS[];
extern const char* const CN_WEEKDAYS[];


// 辅助函数：显示日期
void displayDate(const DateTime& now) {
  // 使用栈分配的局部缓冲区，避免线程安全问题
  char dateStr[30]; // 增加缓冲区大小以处理UTF-8中文字符

  // 使用snprintf自动处理字符串终止
  int result = snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d",
                       now.year(), now.month(), now.day());

  // 检查格式化是否成功
  if (result < 0 || result >= (int)sizeof(dateStr)) {
    // 格式化失败或缓冲区不足，使用错误提示
    strncpy(dateStr, "日期格式错误", sizeof(dateStr) - 1);
    dateStr[sizeof(dateStr) - 1] = '\0';
  }

  u8g2.setFont(u8g2_font_unifont_t_chinese3);
  int16_t dateW = u8g2.getUTF8Width(dateStr);
  int dateX = (128 - dateW) / 2;
  const int dateY = 10;
  u8g2.drawUTF8(dateX, dateY, dateStr);
}

// 辅助函数：显示时间
void displayTimeValue(const DateTime& now) {
  int hour = now.hour();
  // 使用栈分配的局部缓冲区，避免线程安全问题
  char timeStr[20]; // 增加缓冲区大小确保安全

  // 使用snprintf自动处理字符串终止
  int result = snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
                       hour, now.minute(), now.second());

  // 检查格式化是否成功
  if (result < 0 || result >= (int)sizeof(timeStr)) {
    // 格式化失败或缓冲区不足，使用错误提示
    strncpy(timeStr, "时间格式错误", sizeof(timeStr) - 1);
    timeStr[sizeof(timeStr) - 1] = '\0';
  }

  if (displayState.largeFont) {
    u8g2.setFont(u8g2_font_logisoso24_tr);
  } else {
    u8g2.setFont(u8g2_font_logisoso18_tr);
  }
  int16_t tw = u8g2.getUTF8Width(timeStr);
  int tx = (128 - tw) / 2;
  u8g2.drawUTF8(tx, displayState.largeFont ? 42 : 38, timeStr);
}

// 辅助函数：显示市场日和星期
void displayMarketDayAndWeekday(const DateTime& now) {
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);

  // 计算市场日（需要转换为time_t）
  tm timeInfo = {0};
  timeInfo.tm_year = now.year() - 1900;
  timeInfo.tm_mon = now.month() - 1;
  timeInfo.tm_mday = now.day();
  time_t currentTime = mktime(&timeInfo);

  // 确保currentTime有效
  if (currentTime == (time_t)-1) {
    currentTime = now.unixtime();
  }

  int marketIndex;
  calculateMarketDay(currentTime, marketIndex);
  // 边界检查：确保marketIndex在有效范围内（0-2）
  if (marketIndex < 0 || marketIndex >= 3) {
    marketIndex = 0; // 默认值
  }
  const int marketY = 62;
  drawProgmemString((const char*)pgm_read_ptr(&MARKET_DAYS[marketIndex]), 2, marketY);

  // 显示星期（使用dayOfTheWeek()方法，返回0-6）
  int weekdayIndex = now.dayOfTheWeek();
  if (weekdayIndex < 0 || weekdayIndex >= 7) weekdayIndex = 0; // 处理边界情况

  // 获取星期字符串并计算宽度（使用栈分配的局部缓冲区）
  char weekBuffer[10];
  strcpy_P(weekBuffer, (const char*)pgm_read_ptr(&CN_WEEKDAYS[weekdayIndex]));
  int16_t ww = u8g2.getUTF8Width(weekBuffer);
  u8g2.drawUTF8(128 - ww, marketY, weekBuffer);
}

// 辅助函数：显示时间源图标
void displayTimeSourceIcon() {
  u8g2.setFont(u8g2_font_6x10_tf);
  
  // 根据当前时间源显示对应图标
  if (timeState.currentTimeSource == TIME_SOURCE_NTP) {
    u8g2.drawStr(120, 10, "*"); // 星号表示NTP时间
  } else if (timeState.currentTimeSource == TIME_SOURCE_RTC) {
    u8g2.drawStr(120, 10, "R"); // R表示RTC时间
  } else if (timeState.currentTimeSource == TIME_SOURCE_MANUAL) {
    u8g2.drawStr(120, 10, "S"); // S表示软件时钟
  } else {
    u8g2.drawStr(120, 10, "!"); // 感叹号表示时间错误
  }
}

// 优化的显示刷新策略
void displayTime() {
  DateTime now;
  if (!getCurrentTime(now)) {
    // 检查是否刚切换到NTP时间源（在10秒内），如果是，则暂时不显示错误信息
    unsigned long currentTime = millis();
    unsigned long timeSinceSwitch = (currentTime >= timeState.lastTimeSourceSwitch) ?
        (currentTime - timeState.lastTimeSourceSwitch) :
        (0xFFFFFFFF - timeState.lastTimeSourceSwitch + currentTime);
    bool justSwitchedToNtp = (timeState.currentTimeSource == TIME_SOURCE_NTP) && 
                             (timeSinceSwitch < 10000) && // 10秒内
                             (systemState.networkConnected);
    
    // 如果当前是NTP时间源但无法获取时间，尝试回退到RTC时间源
    if (timeState.currentTimeSource == TIME_SOURCE_NTP && 
        systemState.rtcInitialized && systemState.rtcTimeValid && 
        (!justSwitchedToNtp || timeSinceSwitch >= 3000)) { // 如果已超过3秒仍未获取到NTP时间
      // 临时使用RTC时间显示，但保持时间源为NTP
      now = rtc.now();
      if (isRtcTimeValid(now)) {
        // 继续使用RTC时间显示，不返回
      } else {
        if (!systemState.forceDisplayTimeError && !justSwitchedToNtp) {
          displayErrorScreen("时间获取失败", "请检查系统状态");
        } else if (justSwitchedToNtp) {
          // 刚切换到NTP，显示提示信息而不是错误
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_wqy12_t_gb2312);
          u8g2.drawUTF8(0, 20, "正在获取网络时间");
          u8g2.drawUTF8(0, 35, "请稍候...");
          
          // 显示时间源图标
          displayTimeSourceIcon();
          
          u8g2.sendBuffer();
        }
        return;
      }
    } else {
      if (!systemState.forceDisplayTimeError && !justSwitchedToNtp) {
        displayErrorScreen("时间获取失败", "请检查系统状态");
      } else if (justSwitchedToNtp) {
        // 刚切换到NTP，显示提示信息而不是错误
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_wqy12_t_gb2312);
        u8g2.drawUTF8(0, 20, "正在获取网络时间");
        u8g2.drawUTF8(0, 35, "请稍候...");
        
        // 显示时间源图标
        displayTimeSourceIcon();
        
        u8g2.sendBuffer();
      }
      return;
    }
  }
  
  // 验证时间有效性
  if (!isRtcTimeValid(now)) {
    if (!systemState.forceDisplayTimeError) {
      displayErrorScreen("时间无效", "时间不在有效范围内");
    }
    return;
  }
  
  // 检查是否需要更新显示（秒数变化、日期变化或强制刷新）
  bool shouldRefresh = (now.second() != displayState.lastDisplayedSecond) ||
                      (systemState.forceDisplayTimeError != systemState.lastForceDisplayTimeError) ||
                      systemState.needsRefresh;

  // 如果是新的一分钟的开始，强制刷新以更新日期和星期
  static int lastDisplayedMinute = -1;
  bool minuteChanged = (lastDisplayedMinute == -1) || (now.minute() != lastDisplayedMinute);
  if (minuteChanged) {
    shouldRefresh = true;
    lastDisplayedMinute = now.minute();
  }
  
  if (shouldRefresh) {
    displayState.lastDisplayedSecond = now.second();
    systemState.lastForceDisplayTimeError = systemState.forceDisplayTimeError;
    
    u8g2.clearBuffer();
    
    // 调用辅助函数来显示各个部分
    displayDate(now);
    displayTimeValue(now);
    displayMarketDayAndWeekday(now);
    displayTimeSourceIcon();
    
    u8g2.sendBuffer();
  }
}

void displayStatusOverlay() {
  IPAddress ip = WiFi.localIP();
  // 使用栈分配的局部缓冲区，避免线程安全问题
  char line2[40];
  char wifiStatus[40];
  if (WiFi.status() == WL_CONNECTED) {
    snprintf(line2, sizeof(line2), "IP: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    snprintf(wifiStatus, sizeof(wifiStatus), "WiFi: %s", WiFi.SSID().c_str());
  } else {
    strcpy(line2, "IP: 未连接");
    strcpy(wifiStatus, "WiFi: 未连接");
  }

  // 显示当前时间源
  char timeSourceStr[40];
  snprintf(timeSourceStr, sizeof(timeSourceStr), "时间源: %s", getTimeSourceName(timeState.currentTimeSource));

  // 显示信号强度
  int rssi = WiFi.RSSI();
  char signalStr[30];
  if (WiFi.status() == WL_CONNECTED) {
    snprintf(signalStr, sizeof(signalStr), "信号: %ddBm", rssi);
  } else {
    strcpy(signalStr, "信号: 未连接");
  }

  oledShowLinesSmall(wifiStatus, line2, timeSourceStr, signalStr);
}

// 显示OTA模式界面
void displayOtaMode() {
  IPAddress ip = WiFi.localIP();
  // 使用栈分配的局部缓冲区，避免线程安全问题
  char line1[40];
  char line2[40];
  char line3[40];
  char line4[40];

  // 第一行：OTA模式标题
  strcpy(line1, "OTA模式");
  
  // 第二行：IP地址
  if (WiFi.status() == WL_CONNECTED) {
    snprintf(line2, sizeof(line2), "IP: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  } else {
    strcpy(line2, "IP: 未连接");
  }
  
  // 第三行：访问地址
  snprintf(line3, sizeof(line3), "http://%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  
  // 第四行：状态提示
  strcpy(line4, "等待固件上传...");

  oledShowLinesSmall(line1, line2, line3, line4);
}

// 辅助：从年月日计算自2023-01-01以来的天数（不依赖mktime，避免时区问题）
static long getDaysSince2023_01_01(int year, int month, int day) {
  auto isLeap = [](int y) { return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0); };
  auto daysInMonth = [&isLeap](int m, int y) {
    if (m == 2) return isLeap(y) ? 29 : 28;
    if (m == 4 || m == 6 || m == 9 || m == 11) return 30;
    return 31;
  };
  long days = 0;
  for (int y = 2023; y < year; y++) days += isLeap(y) ? 366 : 365;
  for (int m = 1; m < month; m++) days += daysInMonth(m, year);
  return days + (day - 1);
}

void calculateMarketDay(time_t currentTime, int& marketIndex) {
  // 确保currentTime有效（time_t是无符号类型，检查是否为-1）
  if (currentTime == (time_t)-1) {
    marketIndex = 0;
    return;
  }

  // 检查时间是否在合理范围内（2020年到2050年）
  if (currentTime < 1577836800UL || currentTime > 2524608000UL) {
    marketIndex = 0;
    return;
  }

  // 使用localtime_r获取本地日期（与显示时间一致），避免gmtime_r与mktime混用导致的时区问题
  struct tm currentTm;
  if (localtime_r(&currentTime, &currentTm) == nullptr) {
    marketIndex = 0;
    return;
  }

  int year = currentTm.tm_year + 1900;
  int month = currentTm.tm_mon + 1;
  int day = currentTm.tm_mday;

  long daysDiff = getDaysSince2023_01_01(year, month, day);
  int correctOffset = getCorrectOffset();
  marketIndex = (daysDiff + correctOffset) % 3;
  if (marketIndex < 0) marketIndex += 3;
}

void oledShowLines(const char* l1, const char* l2, const char* l3, const char* l4) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_chinese3);
  int y = 14;
  if (l1) { u8g2.drawUTF8(0, y, l1); y += 14; }
  if (l2) { u8g2.drawUTF8(0, y, l2); y += 14; }
  if (l3) { u8g2.drawUTF8(0, y, l3); y += 14; }
  if (l4) { u8g2.drawUTF8(0, y, l4); }
  u8g2.sendBuffer();
}

void oledShowLinesSmall(const char* l1, const char* l2, const char* l3, const char* l4) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  int y = 14;
  if (l1) { u8g2.drawUTF8(0, y, l1); y += 12; }
  if (l2) { u8g2.drawUTF8(0, y, l2); y += 12; }
  if (l3) { u8g2.drawUTF8(0, y, l3); y += 12; }
  if (l4) { u8g2.drawUTF8(0, y, l4); }
  u8g2.sendBuffer();
}

// 统一的错误显示函数，使用一致的字体大小
// 优化布局和间距，提高可读性和美观度
void displayError(const char* l1, const char* l2, const char* l3, const char* l4) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);  // 统一使用12号字体
  
  // 计算文本行数
  int lineCount = 0;
  if (l1) lineCount++;
  if (l2) lineCount++;
  if (l3) lineCount++;
  if (l4) lineCount++;
  
  // 计算起始Y坐标以实现垂直居中
  int lineHeight = 14;  // 行高
  int totalHeight = lineCount * lineHeight;
  int startY = (64 - totalHeight) / 2 + lineHeight;  // 垂直居中
  
  // 显示文本行
  int y = startY;
  if (l1) { 
    int16_t w = u8g2.getUTF8Width(l1);
    u8g2.drawUTF8((128 - w) / 2, y, l1);  // 水平居中
    y += lineHeight; 
  }
  if (l2) { 
    int16_t w = u8g2.getUTF8Width(l2);
    u8g2.drawUTF8((128 - w) / 2, y, l2);  // 水平居中
    y += lineHeight; 
  }
  if (l3) { 
    int16_t w = u8g2.getUTF8Width(l3);
    u8g2.drawUTF8((128 - w) / 2, y, l3);  // 水平居中
    y += lineHeight; 
  }
  if (l4) { 
    int16_t w = u8g2.getUTF8Width(l4);
    u8g2.drawUTF8((128 - w) / 2, y, l4);  // 水平居中
  }
  
  u8g2.sendBuffer();
}

void drawClockIcon() {
  u8g2.clearBuffer();
  int centerX = 64;
  int centerY = 32;
  int radius = 24;
  
  // 绘制时钟外圈
  u8g2.drawCircle(centerX, centerY, radius, U8G2_DRAW_ALL);
  
  // 绘制时钟刻度
  for (int i = 0; i < 12; i++) {
    float angle = (i * 30) * DEG_TO_RAD;
    int x1 = centerX + cos(angle) * radius;
    int y1 = centerY - sin(angle) * radius;
    int x2 = centerX + cos(angle) * (radius - 4);
    int y2 = centerY - sin(angle) * (radius - 4);
    u8g2.drawLine(x1, y1, x2, y2);
  }
  
  // 绘制时针（模拟9:15的位置）
  float hourAngle = (9 * 30 + 15 * 0.5) * DEG_TO_RAD;
  int hourLength = radius - 10;
  u8g2.drawLine(centerX, centerY, 
               centerX + cos(hourAngle) * hourLength,
               centerY - sin(hourAngle) * hourLength);
  
  // 绘制分针（指向15分位置）
  float minuteAngle = (15 * 6) * DEG_TO_RAD;
  int minuteLength = radius - 6;
  u8g2.drawLine(centerX, centerY, 
               centerX + cos(minuteAngle) * minuteLength,
               centerY - sin(minuteAngle) * minuteLength);
  
  // 绘制时钟中心
  u8g2.drawCircle(centerX, centerY, 2, U8G2_DRAW_ALL);
  
  // 显示"时钟"文本
  u8g2.setFont(u8g2_font_unifont_t_chinese3);
  u8g2.drawUTF8(centerX - 16, centerY + 4, "时钟");
  
  u8g2.sendBuffer();
}

void displayErrorScreen(const char* errorMessage, const char* errorDetail) {
  // 测试模式下不显示错误信息
  if (g_testMode) {
    return;
  }

  systemState.forceDisplayTimeError = true;
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.drawUTF8(2, 12, errorMessage);
  if (errorDetail) {
    u8g2.drawUTF8(2, 26, errorDetail);
  }
  
  // 根据当前状态显示不同的解决方案
  if (!systemState.rtcInitialized && !systemState.networkConnected) {
    // 所有时间源都不可用
    u8g2.drawUTF8(2, 42, "请检查硬件连接");
    u8g2.drawUTF8(2, 56, "K4长按: 配网模式");
  } else if (!systemState.rtcInitialized && systemState.networkConnected) {
    // RTC故障，网络正常
    u8g2.drawUTF8(2, 42, "将使用网络时间");
    u8g2.drawUTF8(2, 56, "K3长按: 手动设置");
  } else if (systemState.rtcInitialized && !systemState.networkConnected) {
    // RTC正常，网络故障
    u8g2.drawUTF8(2, 42, "将使用RTC时间");
    u8g2.drawUTF8(2, 56, "K4长按: 重置WiFi");
  } else if (systemState.rtcInitialized && systemState.rtcTimeValid) {
    // RTC正常但时间可能不对
    u8g2.drawUTF8(2, 42, "K3长按: 手动设置时间");
    u8g2.drawUTF8(2, 56, "K4长按: WiFi配网");
  } else {
    // 通用解决方案
    u8g2.drawUTF8(2, 42, "K3长按: 手动设置");
    u8g2.drawUTF8(2, 56, "K4长按: 重置WiFi");
  }
  
  u8g2.sendBuffer();
}

// 设置模式相关函数
void enterSettingMode() {
  settingState.settingMode = true;
  
  // 获取当前时间作为初始设置值
  DateTime now;
  if (getCurrentTime(now)) {
    settingState.settingValues[0] = now.year();
    settingState.settingValues[1] = now.month();
    settingState.settingValues[2] = now.day();
    settingState.settingValues[3] = now.hour();
    settingState.settingValues[4] = now.minute();
    settingState.settingValues[5] = now.second();
  } else {
    // 如果无法获取当前时间，使用默认值
    settingState.settingValues[0] = 2023;
    settingState.settingValues[1] = 1;
    settingState.settingValues[2] = 1;
    settingState.settingValues[3] = 12;
    settingState.settingValues[4] = 0;
    settingState.settingValues[5] = 0;
  }
  
  settingState.settingField = 0; // 默认选择年份字段
  
  LOG_DEBUG("Entered setting mode");
}

void exitSettingMode() {
  settingState.settingMode = false;
  
  // 应用设置的时间
  DateTime newTime(settingState.settingValues[0], settingState.settingValues[1], settingState.settingValues[2],
                   settingState.settingValues[3], settingState.settingValues[4], settingState.settingValues[5]);
  
  // 验证时间有效性
  if (isRtcTimeValid(newTime)) {
    // 更新RTC时间
    if (systemState.rtcInitialized) {
      // rtc.adjust()函数返回void，无法检查返回值
      rtc.adjust(newTime);
      systemState.rtcTimeValid = true;
    }
    
    // 更新软件时钟
    timeState.softwareClockTime = newTime.unixtime();
    timeState.softwareClockBase = millis();
    timeState.softwareClockValid = true;
    
    // 切换到手动时间源
    switchTimeSource(TIME_SOURCE_MANUAL);
    
    LOG_DEBUG("Time settings applied");
  } else {
    LOG_DEBUG("Invalid time settings, not applied");
    static char timeErrorMsg[60];
    snprintf(timeErrorMsg, sizeof(timeErrorMsg), "时间设置无效: %04d-%02d-%02d %02d:%02d:%02d", 
             settingState.settingValues[0], settingState.settingValues[1], settingState.settingValues[2],
             settingState.settingValues[3], settingState.settingValues[4], settingState.settingValues[5]);
    handleError(ERROR_TIME_SETTING_INVALID, ERROR_LEVEL_ERROR, timeErrorMsg);
    displayError("时间设置无效", "请检查输入值", nullptr, nullptr);
    // 使用非阻塞延时替代delay(1000)
    nonBlockingDelay(1000);
  }
}

void displaySettingScreen() {
  u8g2.clearBuffer();

  // 显示标题
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.drawUTF8(0, 10, "设置时间");

  // 显示当前设置的日期
  char dateStr[20];
  snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", settingState.settingValues[0], settingState.settingValues[1], settingState.settingValues[2]);
  u8g2.setFont(u8g2_font_unifont_t_chinese3);
  u8g2.drawUTF8(0, 28, dateStr);

  // 显示当前设置的时间
  char timeStr[16];
  // 固定使用24小时制
  int displayHour = settingState.settingValues[3];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", displayHour, settingState.settingValues[4], settingState.settingValues[5]);
  u8g2.drawUTF8(0, 48, timeStr);

  // 画下滑线高亮当前正在编辑的字段
  if (settingState.settingField < 3) {
    // 高亮日期字段
    int fieldStartX = 0;
    int fieldWidth = 0;

    if (settingState.settingField == 0) { // 年份
      fieldStartX = 0; // 年份显示位置
      fieldWidth = u8g2.getUTF8Width("0000"); // 年份字符宽度
    } else if (settingState.settingField == 1) { // 月份
      fieldStartX = u8g2.getUTF8Width("0000-"); // 月份显示位置
      fieldWidth = u8g2.getUTF8Width("00"); // 月份字符宽度
    } else if (settingState.settingField == 2) { // 日期
      fieldStartX = u8g2.getUTF8Width("0000-00-"); // 日期显示位置
      fieldWidth = u8g2.getUTF8Width("00"); // 日期字符宽度
    }

    // 绘制下滑线
    u8g2.drawHLine(fieldStartX, 30, fieldWidth);
  } else {
    // 高亮时间字段
    int fieldStartX = 0;
    int fieldWidth = 0;

    if (settingState.settingField == 3) { // 小时
      fieldStartX = 0; // 小时显示位置
      fieldWidth = u8g2.getUTF8Width("00"); // 小时字符宽度
    } else if (settingState.settingField == 4) { // 分钟
      fieldStartX = u8g2.getUTF8Width("00:"); // 分钟显示位置
      fieldWidth = u8g2.getUTF8Width("00"); // 分钟字符宽度
    } else if (settingState.settingField == 5) { // 秒
      fieldStartX = u8g2.getUTF8Width("00:00:"); // 秒显示位置
      fieldWidth = u8g2.getUTF8Width("00"); // 秒字符宽度
    }

    // 绘制下滑线
    u8g2.drawHLine(fieldStartX, 50, fieldWidth);
  }

  u8g2.sendBuffer();
}

void updateSettingValue(int direction) {
  // 验证字段索引的有效性
  if (settingState.settingField < 0 || settingState.settingField >= 6) {
    LOG_DEBUG("Invalid setting field index");
    return;
  }
  
  // 更新当前字段的值
  int newValue = settingState.settingValues[settingState.settingField] + direction;
  
  // 确保值在有效范围内
  if (newValue > settingState.settingMaxValues[settingState.settingField]) {
    newValue = settingState.settingMinValues[settingState.settingField];
  } else if (newValue < settingState.settingMinValues[settingState.settingField]) {
    newValue = settingState.settingMaxValues[settingState.settingField];
  }
  
  settingState.settingValues[settingState.settingField] = newValue;
  
  LOG_DEBUG("Updated setting field %d to %d", settingState.settingField, newValue);
}

// 亮度设置模式相关函数
void enterBrightnessSettingMode() {
  settingState.brightnessSettingMode = true;
  LOG_DEBUG("Entered brightness setting mode");
}

void exitBrightnessSettingMode() {
  // 添加初始化检查
  if (systemState.wifiConfigured == false && systemState.rtcInitialized == false) {
    LOG_WARNING("System not initialized, cannot apply brightness settings");
    return;
  }

  settingState.brightnessSettingMode = false;

  // 应用亮度设置
  u8g2.setContrast(BRIGHTNESS_LEVELS[displayState.brightnessIndex]);

  // 保存亮度设置到EEPROM
  if (saveBrightnessIndex(displayState.brightnessIndex)) {
    LOG_DEBUG("Brightness setting saved to EEPROM: %d", displayState.brightnessIndex);
  } else {
    LOG_WARNING("Failed to save brightness setting to EEPROM");
  }

  LOG_DEBUG("Brightness setting applied: %s (index: %d, contrast value: %d)", BRIGHTNESS_LABELS[displayState.brightnessIndex], displayState.brightnessIndex, BRIGHTNESS_LEVELS[displayState.brightnessIndex]);
}

// 从PROGMEM读取字符串到临时缓冲区的辅助函数
// 使用静态缓冲区避免重复分配内存
void drawProgmemString(const char* progmemStr, int x, int y) {
  static char buffer[20]; // 使用静态缓冲区避免重复分配
  memset(buffer, 0, sizeof(buffer)); // 确保缓冲区初始化
  strncpy_P(buffer, progmemStr, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = '\0'; // 确保字符串结束
  u8g2.drawUTF8(x, y, buffer);
}

// 优化的时间格式化函数，避免重复分配
void formatTimeString(char* buffer, size_t size, int hour, int minute, int second) {
  if (buffer == nullptr || size == 0) return;
  snprintf(buffer, size, "%02d:%02d:%02d", hour, minute, second);
  buffer[size - 1] = '\0'; // 确保字符串终止
}

// 优化的日期格式化函数
void formatDateString(char* buffer, size_t size, int year, int month, int day) {
  if (buffer == nullptr || size == 0) return;
  snprintf(buffer, size, "%04d-%02d-%02d", year, month, day);
  buffer[size - 1] = '\0'; // 确保字符串终止
}

void displayBrightnessSettingScreen() {
  u8g2.clearBuffer();

  // 显示标题
  u8g2.setFont(u8g2_font_wqy16_t_gb2312);
  u8g2.drawUTF8(0, 16, "设置亮度");

  // 显示当前亮度等级
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.drawUTF8(0, 32, "当前亮度:");

  // 边界检查：确保brightnessIndex在有效范围内（0-3）
  int safeBrightnessIndex = displayState.brightnessIndex;
  if (safeBrightnessIndex < 0 || safeBrightnessIndex >= 4) {
    safeBrightnessIndex = 2; // 默认值
  }
  drawProgmemString((const char*)pgm_read_ptr(&BRIGHTNESS_LABELS[safeBrightnessIndex]), 0, 48);

  // 绘制亮度条
  int barX = 70;
  int barY = 40;
  int barWidth = 40;
  int barHeight = 8;

  // 绘制亮度条背景
  u8g2.drawFrame(barX, barY, barWidth, barHeight);

  // 绘制已填充部分
  int filledWidth = (barWidth * (safeBrightnessIndex + 1)) / 4;
  u8g2.drawBox(barX, barY, filledWidth, barHeight);

  // 绘制亮度等级指示器
  int indicatorX = barX + (barWidth * safeBrightnessIndex) / 4;
  u8g2.drawVLine(indicatorX, barY - 3, barHeight + 6);

  u8g2.sendBuffer();
}

void updateBrightnessSetting(int direction) {
  // 添加初始化检查
  if (systemState.wifiConfigured == false && systemState.rtcInitialized == false) {
    LOG_WARNING("System not initialized, cannot adjust brightness");
    return;
  }
  
  // 更新亮度索引
  int newBrightnessIndex = displayState.brightnessIndex + direction;
  
  // 边界检查 - 循环边界而不是限制在范围内
  if (newBrightnessIndex > 3) newBrightnessIndex = 0;
  else if (newBrightnessIndex < 0) newBrightnessIndex = 3;
  
  // 确保索引在有效范围内
  if (newBrightnessIndex >= 0 && newBrightnessIndex <= 3) {
    displayState.brightnessIndex = newBrightnessIndex;
    // 应用新的亮度设置
    u8g2.setContrast(BRIGHTNESS_LEVELS[displayState.brightnessIndex]);
    
    LOG_DEBUG("Brightness level: %s (index: %d, contrast value: %d)", BRIGHTNESS_LABELS[displayState.brightnessIndex], displayState.brightnessIndex, BRIGHTNESS_LEVELS[displayState.brightnessIndex]);
  } else {
    LOG_DEBUG("Invalid brightness index");
  }
}