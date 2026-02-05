#ifndef CONFIG_H
#define CONFIG_H

// 引脚定义 (ESP8266 GPIO引脚)
#define K1_PIN 0   // GPIO0 (D3 on NodeMCU)
#define K2_PIN 14  // GPIO14 (D5 on NodeMCU)
#define K3_PIN 12  // GPIO12 (D6 on NodeMCU)
#define K4_PIN 13  // GPIO13 (D7 on NodeMCU)

// 超时和间隔设置
#define WIFI_TIMEOUT 60
#define INIT_TIMEOUT 10 // 初始化超时时间（秒）

// 按键相关常量
const unsigned long DEBOUNCE_DELAY = 50;         // 按键防抖时间(毫秒)
const unsigned long LONG_PRESS_TIME = 500;       // 按键长按判定时间(毫秒)
const unsigned long DOUBLE_CLICK_TIME = 500;     // 按键双击最大间隔时间(毫秒)
const unsigned long BUTTON_RESET_TIME = 2000;    // 按键状态重置时间(毫秒)

// 系统监控相关常量
const unsigned long WATCHDOG_INTERVAL = 30000;   // 系统状态恢复检查周期(30秒)
const unsigned long NETWORK_CHECK_INTERVAL = 3000; // 网络状态监控间隔(3秒)
const unsigned long NTP_SYNC_INTERVAL = 60000;   // NTP同步间隔(1分钟)
const unsigned long RTC_SYNC_INTERVAL = 1800000; // RTC同步间隔(30分钟)
const unsigned long SETTING_MODE_TIMEOUT = 15000; // 设置模式超时(15秒)

// 时间相关常量
const unsigned long SECS_PER_DAY = 86400;        // 一天的秒数
const unsigned long HOUR_IN_MILLIS = 3600000;    // 1小时的毫秒数
const int NTP_RETRY_DELAY = 300;                 // NTP服务器重试延迟(毫秒)

// WiFi相关常量
const unsigned long WIFI_RECONNECT_INTERVAL = 15000; // WiFi重连间隔15秒

// NTP相关常量
const int NTP_TIMEOUT = 2000;                    // NTP单次请求超时时间(毫秒)
const unsigned long NTP_CHECK_COOLDOWN = 30000;  // NTP检查冷却时间(30秒)
const unsigned long NTP_CHECK_TIMEOUT = 10000;   // NTP检查超时时间(10秒)，防止标志永久卡住

// =============================================================================
// 错误处理系统
// =============================================================================

/**
 * @brief 错误代码枚举
 * 
 * 定义系统中可能发生的各种错误类型，用于统一的错误报告和处理
 */
enum ErrorCode {
  ERROR_NONE = 0,                    ///< 无错误
  ERROR_RTC_INIT_FAILED = 1,        ///< RTC初始化失败
  ERROR_RTC_I2C_ERROR = 2,          ///< RTC I2C通信错误
  ERROR_RTC_TIME_INVALID = 3,       ///< RTC时间无效
  ERROR_WIFI_CONNECTION_FAILED = 4,  ///< WiFi连接失败
  ERROR_NTP_CONNECTION_FAILED = 5,   ///< NTP连接失败
  ERROR_TIME_SOURCE_UNAVAILABLE = 6, ///< 时间源不可用
  ERROR_TIME_SETTING_INVALID = 7,    ///< 时间设置无效
  ERROR_SYSTEM_WATCHDOG_TIMEOUT = 8, ///< 系统看门狗超时
  ERROR_BUTTON_STATE_INVALID = 9    ///< 按键状态无效
};

/**
 * @brief 错误级别枚举
 * 
 * 定义错误的严重程度，决定处理策略
 */
enum ErrorLevel {
  ERROR_LEVEL_INFO = 0,     ///< 信息级别，仅记录日志
  ERROR_LEVEL_WARNING = 1,  ///< 警告级别，可能需要用户注意
  ERROR_LEVEL_ERROR = 2,    ///< 错误级别，影响功能但可恢复
  ERROR_LEVEL_CRITICAL = 3  ///< 严重级别，可能导致系统重启
};

#endif