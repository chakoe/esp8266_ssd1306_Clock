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
#include "version.h"

// UI 布局常量
const int PADDING_X = 4;
const int PADDING_Y = 2;
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;

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





// 辅助函数：显示日期（复古风格 - 顶部居中，带装饰线）
void displayDate(const DateTime& now) {
  // 绘制顶部装饰线
  u8g2.drawLine(PADDING_X, PADDING_Y + 1, 30, PADDING_Y + 1);
  u8g2.drawLine(SCREEN_WIDTH - 30, PADDING_Y + 1, SCREEN_WIDTH - PADDING_X - 1, PADDING_Y + 1);

  char dateStr[30];
  int result = snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d",
                       now.year(), now.month(), now.day());

  if (result < 0 || result >= (int)sizeof(dateStr)) {
    strncpy(dateStr, "日期格式错误", sizeof(dateStr) - 1);
    dateStr[sizeof(dateStr) - 1] = '\0';
  }

  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  int16_t dateW = u8g2.getUTF8Width(dateStr);
  int dateX = (SCREEN_WIDTH - dateW) / 2;
  const int dateY = PADDING_Y + 12;
  u8g2.drawUTF8(dateX, dateY, dateStr);
}

// 辅助函数：显示时间（复古风格 - 核心区域，带分隔线）
void displayTimeValue(const DateTime& now) {
  int hour = now.hour();
  char timeStr[20];

  int result = snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
                       hour, now.minute(), now.second());

  if (result < 0 || result >= (int)sizeof(timeStr)) {
    strncpy(timeStr, "时间格式错误", sizeof(timeStr) - 1);
    timeStr[sizeof(timeStr) - 1] = '\0';
  }

  // 绘制时间区域的上下分隔线（下方线居中，不贯穿两边）
  u8g2.drawLine(PADDING_X, 18, SCREEN_WIDTH - PADDING_X - 1, 18);
  
  int lineWidth = 60; // 分隔线宽度
  int lineStartX = (SCREEN_WIDTH - lineWidth) / 2;
  u8g2.drawLine(lineStartX, 54, lineStartX + lineWidth, 54);

  if (displayState.largeFont) {
    u8g2.setFont(u8g2_font_logisoso26_tr);
  } else {
    u8g2.setFont(u8g2_font_logisoso18_tr);
  }
  
  // 固定时间显示位置，防止非等宽字体导致的跳动
  // 针对不同字号使用不同的固定起始坐标，以确保视觉居中
  int fixedX = displayState.largeFont ? 2 : 22; 
  int fixedY = displayState.largeFont ? 48 : 42;
  
  u8g2.drawUTF8(fixedX, fixedY, timeStr);
}

// 辅助函数：显示市场日和星期（复古风格 - 底部区域，带底线）
void displayMarketDayAndWeekday(const DateTime& now) {
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);

  tm timeInfo = {};
  timeInfo.tm_year = now.year() - 1900;
  timeInfo.tm_mon = now.month() - 1;
  timeInfo.tm_mday = now.day();
  timeInfo.tm_hour = 0;
  timeInfo.tm_min = 0;
  timeInfo.tm_sec = 0;
  time_t currentTime = mktime(&timeInfo);

  if (currentTime == (time_t)-1) {
    currentTime = now.unixtime();
  }

  int marketIndex;
  calculateMarketDay(currentTime, marketIndex);
  if (marketIndex < 0 || marketIndex >= 3) {
    marketIndex = 0;
  }
  
  // 绘制底部装饰底线
  u8g2.drawLine(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);

  const int infoY = SCREEN_HEIGHT - 4;
  drawProgmemString((const char*)pgm_read_ptr(&MARKET_DAYS[marketIndex]), PADDING_X, infoY);

  int weekdayIndex = now.dayOfTheWeek();
  if (weekdayIndex < 0 || weekdayIndex >= 7) weekdayIndex = 0;

  char weekBuffer[10];
  strcpy_P(weekBuffer, (const char*)pgm_read_ptr(&CN_WEEKDAYS[weekdayIndex]));
  int16_t ww = u8g2.getUTF8Width(weekBuffer);
  u8g2.drawUTF8(SCREEN_WIDTH - ww - PADDING_X, infoY, weekBuffer);
}

// 辅助函数：显示时间源图标（已禁用）
void displayTimeSourceIcon() {
  // 不再显示时间源图标，改为后台自动切换
  // u8g2.setFont(u8g2_font_6x10_tf);
  // 
  // // 根据当前时间源显示对应图标
  // if (timeState.currentTimeSource == TIME_SOURCE_NTP) {
  //   u8g2.drawStr(120, 10, "*"); // *表示NTP时间
  // } else if (timeState.currentTimeSource == TIME_SOURCE_RTC) {
  //   u8g2.drawStr(120, 10, "R"); // R表示RTC时间
  // } else if (timeState.currentTimeSource == TIME_SOURCE_MANUAL) {
  //   u8g2.drawStr(120, 10, "S"); // S表示软件时钟
  // } else {
  //   u8g2.drawStr(120, 10, "!"); // !表示时间错误
  // }
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
        (!justSwitchedToNtp || timeSinceSwitch >= 3000)) {
      // 临时使用RTC时间显示，但保持时间源为NTP
      DateTime rtcNow = rtc.now();
      now = rtcNow;  // 使用拷贝构造而非赋值操作符
      if (!isRtcTimeValid(now)) {
        // RTC时间也无效，显示错误
        if (!systemState.forceDisplayTimeError && !justSwitchedToNtp) {
          displayErrorScreen("时间获取失败", "请检查系统状态");
        } else if (justSwitchedToNtp) {
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_wqy12_t_gb2312);
          u8g2.drawUTF8(0, 20, "正在获取网络时间");
          u8g2.drawUTF8(0, 35, "请稍候...");
          // displayTimeSourceIcon(); // 已禁用时间源图标显示
          u8g2.sendBuffer();
        }
        return;
      }
      // RTC时间有效，继续使用它显示
    } else {
      // 无可用时间源
      if (!systemState.forceDisplayTimeError && !justSwitchedToNtp) {
        displayErrorScreen("时间获取失败", "请检查系统状态");
      } else if (justSwitchedToNtp) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_wqy12_t_gb2312);
        u8g2.drawUTF8(0, 20, "正在获取网络时间");
        u8g2.drawUTF8(0, 35, "请稍候...");
        // displayTimeSourceIcon(); // 已禁用时间源图标显示
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
  
  // 时间已恢复有效，清除错误显示标志
  bool wasInErrorState = systemState.forceDisplayTimeError;
  if (wasInErrorState) {
    systemState.forceDisplayTimeError = false;
  }
  
  // 检查是否需要更新显示（秒数变化、日期变化或强制刷新）
  bool shouldRefresh = (now.second() != displayState.lastDisplayedSecond) ||
                      (wasInErrorState != systemState.lastForceDisplayTimeError) ||
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
    // displayTimeSourceIcon(); // 已禁用时间源图标显示
    
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

// 显示OTA更新中界面
void displayOtaUpdating() {
  u8g2.clearBuffer();

  // 显示"OTA更新中"标题 - 顶部显示
  u8g2.setFont(u8g2_font_wqy16_t_gb2312);
  u8g2.drawUTF8(0, 16, "OTA更新中");

  // 绘制进度条外框
  int barX = 10;
  int barY = 26;
  int barWidth = 108;
  int barHeight = 10;
  u8g2.drawFrame(barX, barY, barWidth, barHeight);

  // 显示进度提示文字 - 居中显示
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);

  // 第一行提示
  const char* prompt1 = "请勿断电...";
  int textWidth1 = u8g2.getUTF8Width(prompt1);
  int textX1 = (128 - textWidth1) / 2;
  u8g2.drawUTF8(textX1, 50, prompt1);

  // 第二行提示
  const char* prompt2 = "正在上传固件";
  int textWidth2 = u8g2.getUTF8Width(prompt2);
  int textX2 = (128 - textWidth2) / 2;
  u8g2.drawUTF8(textX2, 62, prompt2);

  u8g2.sendBuffer();
}

// 显示OTA更新完成界面
void displayOtaComplete() {
  u8g2.clearBuffer();

  // 显示"更新完成"标题 - 居中显示
  u8g2.setFont(u8g2_font_wqy16_t_gb2312);
  const char* title = "更新完成";
  int titleWidth = u8g2.getUTF8Width(title);
  int titleX = (128 - titleWidth) / 2;
  u8g2.drawUTF8(titleX, 16, title);

  // 显示"设备正在重启"提示 - 居中显示
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  const char* prompt = "设备正在重启...";
  int promptWidth = u8g2.getUTF8Width(prompt);
  int promptX = (128 - promptWidth) / 2;
  u8g2.drawUTF8(promptX, 36, prompt);

  // 显示成功图标 - 对勾 (居中显示,缩小版本)
  // 图标位置: x=58, y=48, 大小=12x12, 底部到60 (在64像素内)
  u8g2.drawFrame(58, 48, 12, 12);
  u8g2.drawLine(60, 54, 62, 56);
  u8g2.drawLine(62, 56, 68, 50);

  u8g2.sendBuffer();
}

// 显示OTA更新失败界面
void displayOtaFailed() {
  u8g2.clearBuffer();

  // 显示"更新失败"标题 - 居中显示
  u8g2.setFont(u8g2_font_wqy16_t_gb2312);
  const char* title = "更新失败";
  int titleWidth = u8g2.getUTF8Width(title);
  int titleX = (128 - titleWidth) / 2;
  u8g2.drawUTF8(titleX, 18, title);

  // 显示错误提示 - 居中显示
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  const char* prompt = "请检查固件文件";
  int promptWidth = u8g2.getUTF8Width(prompt);
  int promptX = (128 - promptWidth) / 2;
  u8g2.drawUTF8(promptX, 38, prompt);

  // 显示失败图标 - 叉号 (居中显示,缩小版本)
  // 图标位置: x=58, y=48, 大小=12x12, 底部到60 (在64像素内)
  u8g2.drawFrame(58, 48, 12, 12);
  u8g2.drawLine(60, 50, 68, 58);
  u8g2.drawLine(68, 50, 60, 58);

  u8g2.sendBuffer();
}

// 显示OTA更新进度界面
void displayOtaProgress(uint8_t progress, uint32_t uploadedSize, uint32_t totalSize) {
  u8g2.clearBuffer();

  // 标题 - 顶部显示
  u8g2.setFont(u8g2_font_wqy16_t_gb2312);
  u8g2.drawUTF8(0, 16, "OTA更新中");

  // 绘制进度条外框
  int barX = 10;
  int barY = 26;
  int barWidth = 108;
  int barHeight = 10;
  u8g2.drawFrame(barX, barY, barWidth, barHeight);

  // 绘制进度条填充 - 确保不超过外框宽度
  int filledWidth = (barWidth * progress) / 100;
  if (filledWidth > barWidth) filledWidth = barWidth;
  if (filledWidth > 0) {
    u8g2.drawBox(barX, barY, filledWidth, barHeight);
  }

  // 显示进度百分比 - 居中显示
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  char progressStr[20];
  snprintf(progressStr, sizeof(progressStr), "%d%%", progress);

  // 计算百分比文字的宽度以居中显示
  int textWidth = u8g2.getUTF8Width(progressStr);
  int textX = (128 - textWidth) / 2;  // 128是屏幕宽度
  u8g2.drawUTF8(textX, 50, progressStr);

  // 显示上传大小信息 - 简化显示以适应屏幕
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  char sizeStr[20];
  snprintf(sizeStr, sizeof(sizeStr), "%u/%uKB",
           (unsigned int)(uploadedSize / 1024),
           (unsigned int)(totalSize > 0 ? totalSize / 1024 : 0));
  int sizeTextWidth = u8g2.getUTF8Width(sizeStr);
  int sizeTextX = (128 - sizeTextWidth) / 2;
  u8g2.drawUTF8(sizeTextX, 62, sizeStr);

  u8g2.sendBuffer();
}

// 辅助：判断是否为闰年
static bool isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// 辅助：获取指定月份的天数
static int getDaysInMonth(int month, int year) {
  if (month == 2) return isLeapYear(year) ? 29 : 28;
  if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
  return 31;
}

// 辅助：计算从基准日期到指定日期的天数差
static long getDaysSinceBaseDate(int year, int month, int day) {
  // 使用配置的基准日期
  int baseYear = MarketConfig::BASE_YEAR;
  int baseMonth = MarketConfig::BASE_MONTH;
  int baseDay = MarketConfig::BASE_DAY;
  
  // 计算目标日期相对于基准日期的天数
  long days = 0;
  
  if (year > baseYear || (year == baseYear && month > baseMonth) || 
      (year == baseYear && month == baseMonth && day >= baseDay)) {
    // 目标日期在基准日期之后
    for (int y = baseYear; y < year; y++) {
      days += isLeapYear(y) ? 366 : 365;
    }
    for (int m = 1; m < month; m++) {
      days += getDaysInMonth(m, year);
    }
    days += (day - 1);
    
    // 减去基准日期之前的天数
    for (int m = 1; m < baseMonth; m++) {
      days -= getDaysInMonth(m, baseYear);
    }
    days -= (baseDay - 1);
  } else {
    // 目标日期在基准日期之前（返回负数）
    for (int y = year; y < baseYear; y++) {
      days -= isLeapYear(y) ? 366 : 365;
    }
    for (int m = 1; m < baseMonth; m++) {
      days -= getDaysInMonth(m, baseYear);
    }
    days -= (baseDay - 1);
    
    // 加上目标日期之前的天数
    for (int m = 1; m < month; m++) {
      days += getDaysInMonth(m, year);
    }
    days += (day - 1);
  }
  
  return days;
}

void calculateMarketDay(time_t currentTime, int& marketIndex) {
  // 确保currentTime有效
  if (currentTime == (time_t)-1) {
    marketIndex = 0;
    return;
  }

  // 检查时间是否在合理范围内（2000年到2099年）
  if (currentTime < 946684800UL || currentTime > 4102444799UL) {
    marketIndex = 0;
    return;
  }

  // 使用localtime_r获取本地日期
  struct tm currentTm;
  if (localtime_r(&currentTime, &currentTm) == nullptr) {
    marketIndex = 0;
    return;
  }

  int year = currentTm.tm_year + 1900;
  int month = currentTm.tm_mon + 1;
  int day = currentTm.tm_mday;

  // 计算从基准日期到当前日期的天数差
  long daysDiff = getDaysSinceBaseDate(year, month, day);
  
  // 使用简化的公式计算市场索引：((daysDiff % 3) + 3) % 3
  // 这个公式直接处理负数情况，确保结果在0-2范围内
  marketIndex = ((daysDiff % MarketConfig::CYCLE_DAYS) + MarketConfig::CYCLE_DAYS) % MarketConfig::CYCLE_DAYS;
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
  settingState.settingModeEnterTime = millis();  // 记录进入时间
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
  settingState.settingField = 0;  // 清理残留状态
  settingState.settingModeEnterTime = 0;  // 清理残留时间戳
  
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

  // 绘制圆角外框
  u8g2.drawRFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 4);

  // 显示标题 - 居中
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  const char* title = "设置时间";
  int16_t titleW = u8g2.getUTF8Width(title);
  u8g2.drawUTF8((SCREEN_WIDTH - titleW) / 2, 12, title);

  // 显示当前设置的日期
  char dateStr[20];
  snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", settingState.settingValues[0], settingState.settingValues[1], settingState.settingValues[2]);
  u8g2.setFont(u8g2_font_unifont_t_chinese3);
  int16_t dateW = u8g2.getUTF8Width(dateStr);
  u8g2.drawUTF8((SCREEN_WIDTH - dateW) / 2, 30, dateStr);

  // 显示当前设置的时间
  char timeStr[16];
  int displayHour = settingState.settingValues[3];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", displayHour, settingState.settingValues[4], settingState.settingValues[5]);
  int16_t timeW = u8g2.getUTF8Width(timeStr);
  u8g2.drawUTF8((SCREEN_WIDTH - timeW) / 2, 50, timeStr);

  // 高亮逻辑优化：使用反色块或加粗下划线
  int fieldStartX = 0;
  int fieldWidth = 0;
  int highlightY = 0;

  if (settingState.settingField < 3) {
    highlightY = 32; // 日期行下方
    if (settingState.settingField == 0) { // 年份
      fieldStartX = (SCREEN_WIDTH - dateW) / 2;
      fieldWidth = u8g2.getUTF8Width("0000");
    } else if (settingState.settingField == 1) { // 月份
      fieldStartX = (SCREEN_WIDTH - dateW) / 2 + u8g2.getUTF8Width("0000-");
      fieldWidth = u8g2.getUTF8Width("00");
    } else if (settingState.settingField == 2) { // 日期
      fieldStartX = (SCREEN_WIDTH - dateW) / 2 + u8g2.getUTF8Width("0000-00-");
      fieldWidth = u8g2.getUTF8Width("00");
    }
  } else {
    highlightY = 52; // 时间行下方
    if (settingState.settingField == 3) { // 小时
      fieldStartX = (SCREEN_WIDTH - timeW) / 2;
      fieldWidth = u8g2.getUTF8Width("00");
    } else if (settingState.settingField == 4) { // 分钟
      fieldStartX = (SCREEN_WIDTH - timeW) / 2 + u8g2.getUTF8Width("00:");
      fieldWidth = u8g2.getUTF8Width("00");
    } else if (settingState.settingField == 5) { // 秒
      fieldStartX = (SCREEN_WIDTH - timeW) / 2 + u8g2.getUTF8Width("00:00:");
      fieldWidth = u8g2.getUTF8Width("00");
    }
  }

  // 绘制加粗高亮线（两条平行线）
  u8g2.drawHLine(fieldStartX, highlightY, fieldWidth);
  u8g2.drawHLine(fieldStartX, highlightY + 1, fieldWidth);

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
  settingState.settingModeEnterTime = millis();  // 记录进入时间
  settingState.brightnessSettingMode = true;
  LOG_DEBUG("Entered brightness setting mode");
}

void exitBrightnessSettingMode() {
  // 亮度调节和保存不依赖于WiFi或RTC状态，移除不必要的检查

  settingState.brightnessSettingMode = false;
  settingState.settingModeEnterTime = 0;  // 清理残留时间戳

  // 应用亮度设置
  u8g2.setContrast(BRIGHTNESS_LEVELS[displayState.brightnessIndex]);

  // 保存亮度设置到EEPROM
  if (saveBrightnessIndex(displayState.brightnessIndex)) {
    LOG_DEBUG("Brightness setting saved to EEPROM: %d", displayState.brightnessIndex);
  } else {
    LOG_WARNING("Failed to save brightness setting to EEPROM");
  }

  // 使用PROGMEM安全方式读取亮度标签用于日志输出
  static char labelBuf[20];
  strncpy_P(labelBuf, (const char*)pgm_read_ptr(&BRIGHTNESS_LABELS[displayState.brightnessIndex]), sizeof(labelBuf) - 1);
  labelBuf[sizeof(labelBuf) - 1] = '\0';
  LOG_DEBUG("Brightness setting applied: %s (index: %d, contrast value: %d)", labelBuf, displayState.brightnessIndex, BRIGHTNESS_LEVELS[displayState.brightnessIndex]);
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

  // 绘制圆角外框
  u8g2.drawRFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 4);

  // 显示标题 - 居中
  u8g2.setFont(u8g2_font_wqy16_t_gb2312);
  const char* title = "设置亮度";
  int16_t titleW = u8g2.getUTF8Width(title);
  u8g2.drawUTF8((SCREEN_WIDTH - titleW) / 2, 16, title);

  // 显示当前亮度等级 - 居中
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  const char* label = "当前亮度:";
  int16_t labelW = u8g2.getUTF8Width(label);
  u8g2.drawUTF8((SCREEN_WIDTH - labelW) / 2, 32, label);

  int safeBrightnessIndex = displayState.brightnessIndex;
  if (safeBrightnessIndex < 0 || safeBrightnessIndex >= 4) {
    safeBrightnessIndex = 2;
  }
  
  // 亮度值居中显示
  char buf[20];
  strncpy_P(buf, (const char*)pgm_read_ptr(&BRIGHTNESS_LABELS[safeBrightnessIndex]), sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  int16_t valW = u8g2.getUTF8Width(buf);
  u8g2.drawUTF8((SCREEN_WIDTH - valW) / 2, 46, buf);

  // 绘制拟物化进度条（带刻度）
  int barX = 20;
  int barY = 54;
  int barWidth = 88;
  int barHeight = 6;

  // 绘制背景槽
  u8g2.drawFrame(barX, barY, barWidth, barHeight);

  // 绘制刻度
  for (int i = 0; i <= 4; i++) {
    int tickX = barX + (barWidth * i) / 4;
    u8g2.drawVLine(tickX, barY - 2, barHeight + 4);
  }

  // 绘制填充部分
  int filledWidth = (barWidth * (safeBrightnessIndex + 1)) / 4;
  u8g2.drawBox(barX + 1, barY + 1, filledWidth - 2, barHeight - 2);

  u8g2.sendBuffer();
}

void updateBrightnessSetting(int direction) {
  // 亮度调节和保存不依赖于WiFi或RTC状态，移除不必要的检查
  
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

    // 使用PROGMEM安全方式读取亮度标签用于日志输出
    static char logLabelBuf[20];
    strncpy_P(logLabelBuf, (const char*)pgm_read_ptr(&BRIGHTNESS_LABELS[displayState.brightnessIndex]), sizeof(logLabelBuf) - 1);
    logLabelBuf[sizeof(logLabelBuf) - 1] = '\0';
    LOG_DEBUG("Brightness level: %s (index: %d, contrast value: %d)", logLabelBuf, displayState.brightnessIndex, BRIGHTNESS_LEVELS[displayState.brightnessIndex]);
  } else {
    LOG_DEBUG("Invalid brightness index");
  }
}

/**
 * @brief 显示版本信息
 */
void displayVersionInfo() {
  u8g2.clearBuffer();

  // 标题
  u8g2.setFont(u8g2_font_wqy16_t_gb2312);
  u8g2.drawUTF8(0, 16, "固件版本信息");

  // 版本号
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  char versionLine[30];
  snprintf(versionLine, sizeof(versionLine), "版本: %s", getVersionString());
  u8g2.drawUTF8(0, 32, versionLine);

  // 构建日期
  char dateLine[30];
  snprintf(dateLine, sizeof(dateLine), "日期: %s", getVersionInfo().date);
  u8g2.drawUTF8(0, 48, dateLine);

  // 芯片ID
  char chipIdLine[30];
  snprintf(chipIdLine, sizeof(chipIdLine), "芯片ID: %X", ESP.getChipId());
  u8g2.drawUTF8(0, 62, chipIdLine);

  u8g2.sendBuffer();
}
