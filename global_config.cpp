/**
 * @file global_config.cpp
 * @brief 全局配置变量定义文件
 * 
 * 定义所有全局变量和配置常量
 * 
 * @author ESP8266 SSD1306 Clock Project
 * @version 2.0
 * @date 2025-12-23
 */

#include "global_config.h"
#include <U8g2lib.h>
#include <NTPClient.h>
#include <RTClib.h>
#include <WiFiUdp.h>

// 测试模式标志
bool g_testMode = false;

// 全局对象定义
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 8 * 3600, 60000);
RTC_DS1307 rtc;

// 按钮状态定义
ButtonStateArray buttonStates;

// 系统状态定义
SystemState systemState = {
    false,  // wifiConfigured
    false,  // rtcInitialized
    false,  // rtcTimeValid
    false,  // networkConnected
    0,      // lastWatchdogCheck
    0,      // lastDisplayUpdate
    0,      // lastMainLoopTime
    {0},    // lastButtonPressTime
    0,      // lastForcedRefresh
    0,      // lastNetworkCheck
    0,      // wifiReconnectAttempt
    false,  // forceDisplayTimeError
    false,  // lastForceDisplayTimeError
    false,  // needsRefresh
    {0}     // encryptedWifiPassword
};

// 显示状态定义
DisplayState displayState = {
    -1,     // lastDisplayedSecond
    0,      // statusOverlayUntil
    true,   // largeFont
    2,      // brightnessIndex
    false,  // showStatus
    ""      // timeSourceStatus
};

// 设置状态定义
SettingState settingState = {
    false,  // settingMode
    0,      // settingField
    {0, 0, 0, 0, 0, 0},  // settingValues
    {2099, 12, 31, 23, 59, 59},  // settingMaxValues
    {2020, 1, 1, 0, 0, 0},  // settingMinValues
    false,  // brightnessSettingMode
    false,  // timeSourceSettingMode
    0       // selectedTimeSourceIndex
};

// 时间状态定义
TimeState timeState = {
    TIME_SOURCE_NONE,  // currentTimeSource
    TIME_SOURCE_NONE,  // lastTimeSource
    false,  // softwareClockValid
    0,      // softwareClockBase
    0,      // softwareClockTime
    "pool.ntp.org",  // currentNtpServer
    0,      // ntpFailCount
    0,      // lastRtcSync
    false,  // timeSourceChanged
    false,  // ntpCheckInProgress
    0,      // lastNtpCheckAttempt
    0,      // ntpCheckStartTime
    0,      // currentNtpServerIndex
    0       // lastTimeSourceSwitch
};

// 全局常量定义 - 使用PROGMEM优化内存
const uint8_t BRIGHTNESS_LEVELS[] = {0x10, 0x2F, 0x7F, 0xFF};

// 使用PROGMEM存储常量字符串，节省RAM空间
const char BRIGHTNESS_LABEL_0[] PROGMEM = "低亮";
const char BRIGHTNESS_LABEL_1[] PROGMEM = "中亮";
const char BRIGHTNESS_LABEL_2[] PROGMEM = "高亮";
const char BRIGHTNESS_LABEL_3[] PROGMEM = "最亮";
const char* const BRIGHTNESS_LABELS[] PROGMEM = {
    BRIGHTNESS_LABEL_0, BRIGHTNESS_LABEL_1, BRIGHTNESS_LABEL_2, BRIGHTNESS_LABEL_3
};

const char MARKET_DAY_0[] PROGMEM = "太守";
const char MARKET_DAY_1[] PROGMEM = "新桥";
const char MARKET_DAY_2[] PROGMEM = "芦圩";
const char* const MARKET_DAYS[] PROGMEM = {
    MARKET_DAY_0, MARKET_DAY_1, MARKET_DAY_2
};

const char WEEKDAY_0[] PROGMEM = "周日";
const char WEEKDAY_1[] PROGMEM = "周一";
const char WEEKDAY_2[] PROGMEM = "周二";
const char WEEKDAY_3[] PROGMEM = "周三";
const char WEEKDAY_4[] PROGMEM = "周四";
const char WEEKDAY_5[] PROGMEM = "周五";
const char WEEKDAY_6[] PROGMEM = "周六";
const char* const CN_WEEKDAYS[] PROGMEM = {
    WEEKDAY_0, WEEKDAY_1, WEEKDAY_2, WEEKDAY_3, WEEKDAY_4, WEEKDAY_5, WEEKDAY_6
};

// NTP服务器列表，用于故障转移 - 使用PROGMEM优化
const char NTP_SERVER_0[] PROGMEM = "pool.ntp.org";
const char NTP_SERVER_1[] PROGMEM = "cn.pool.ntp.org";
const char NTP_SERVER_2[] PROGMEM = "ntp.aliyun.com";
const char NTP_SERVER_3[] PROGMEM = "time.windows.com";
const char* const NTP_SERVERS[] PROGMEM = {
    NTP_SERVER_0, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3
};

const int NTP_SERVER_COUNT = sizeof(NTP_SERVERS) / sizeof(NTP_SERVERS[0]);
const unsigned long DISPLAY_UPDATE_INTERVAL = 1000;