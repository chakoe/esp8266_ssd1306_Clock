/**
 * @file logger.cpp
 * @brief 分级日志系统实现
 * 
 * 实现分级的日志输出功能
 * 
 * @author ESP8266 SSD1306 Clock Project
 * @version 2.0
 * @date 2025-12-23
 */

#include "logger.h"
#include "production_config.h"
#include <stdarg.h>

// 日志配置定义
LogConfig logConfig = {
    .currentLevel = DEFAULT_LOG_LEVEL,  // 使用production_config.h中的默认级别
    .enabled = true,
    .timestampEnabled = ENABLE_TIMESTAMP,  // 使用production_config.h中的设置
    .useFlashStrings = true
};

// 日志级别名称（使用Flash字符串优化）
const char LOG_LEVEL_NAME_ERROR[] PROGMEM = "ERROR";
const char LOG_LEVEL_NAME_WARNING[] PROGMEM = "WARN";
const char LOG_LEVEL_NAME_INFO[] PROGMEM = "INFO";
const char LOG_LEVEL_NAME_DEBUG[] PROGMEM = "DEBUG";
const char LOG_LEVEL_NAME_VERBOSE[] PROGMEM = "VERBOSE";

const char* const LOG_LEVEL_NAMES[] PROGMEM = {
    LOG_LEVEL_NAME_ERROR,
    LOG_LEVEL_NAME_WARNING,
    LOG_LEVEL_NAME_INFO,
    LOG_LEVEL_NAME_DEBUG,
    LOG_LEVEL_NAME_VERBOSE
};

void initLogger() {
    Serial.begin(115200);

    // 等待串口初始化完成 - 使用非阻塞方式（带溢出保护）
    unsigned long startTime = millis();
    while (!Serial) {
        unsigned long currentMillis = millis();
        unsigned long elapsed = (currentMillis >= startTime) ?
                              (currentMillis - startTime) :
                              (0xFFFFFFFF - startTime + currentMillis);
        if (elapsed >= 5000) break;  // 超时5秒

        yield(); // 让出控制权
    }

    LOG_INFO("Logger initialized with level: %s", getLogLevelName(logConfig.currentLevel));
}

void logMessage(LogLevel level, const char* format, ...) {
    if (!logConfig.enabled || level > logConfig.currentLevel) {
        return;
    }

    // 输出时间戳
    if (logConfig.timestampEnabled) {
        unsigned long currentMillis = millis();
        Serial.printf("[%08lu] ", currentMillis);
    }

    // 输出日志级别（使用栈分配的缓冲区，避免重入问题）
    char levelName[10];
    int safeLevel = (level >= LOG_LEVEL_ERROR && level <= LOG_LEVEL_VERBOSE) ? level : LOG_LEVEL_INFO;
    strncpy_P(levelName, (const char*)pgm_read_ptr(&LOG_LEVEL_NAMES[safeLevel]), sizeof(levelName) - 1);
    levelName[sizeof(levelName) - 1] = '\0';
    Serial.printf("[%s] ", levelName);

    // 输出日志消息，使用栈分配的缓冲区避免重入问题
    va_list args;
    va_start(args, format);

    // 使用栈分配的缓冲区，避免多线程/中断环境下的静态缓冲区冲突
    char tempBuffer[200]; // 限制消息长度以避免缓冲区溢出
    memset(tempBuffer, 0, sizeof(tempBuffer)); // 确保缓冲区初始化
    int len = vsnprintf(tempBuffer, sizeof(tempBuffer), format, args);

    if (len > 0) {
        // vsnprintf已确保字符串以null结尾，但仍需检查
        if (len >= (int)sizeof(tempBuffer)) {
            tempBuffer[sizeof(tempBuffer) - 1] = '\0';
        }
        Serial.print(tempBuffer);
    }

    va_end(args);

    Serial.println();
}

void logMessageP(LogLevel level, const char* format, ...) {
    if (!logConfig.enabled || level > logConfig.currentLevel) {
        return;
    }

    // 输出时间戳
    if (logConfig.timestampEnabled) {
        unsigned long currentMillis = millis();
        Serial.printf("[%08lu] ", currentMillis);
    }

    // 输出日志级别（使用栈分配的缓冲区，避免重入问题）
    char levelName[10];
    int safeLevel = (level >= LOG_LEVEL_ERROR && level <= LOG_LEVEL_VERBOSE) ? level : LOG_LEVEL_INFO;
    strncpy_P(levelName, (const char*)pgm_read_ptr(&LOG_LEVEL_NAMES[safeLevel]), sizeof(levelName) - 1);
    levelName[sizeof(levelName) - 1] = '\0';
    Serial.printf("[%s] ", levelName);

    // 从Flash读取格式化字符串（使用栈分配的缓冲区）
    char formatBuffer[100];
    memset(formatBuffer, 0, sizeof(formatBuffer));
    strncpy_P(formatBuffer, format, sizeof(formatBuffer) - 1);
    formatBuffer[sizeof(formatBuffer) - 1] = '\0';

    // 输出日志消息，使用栈分配的缓冲区避免重入问题
    va_list args;
    va_start(args, format);

    // 使用栈分配的缓冲区，避免多线程/中断环境下的静态缓冲区冲突
    char tempBuffer[200]; // 限制消息长度以避免缓冲区溢出
    memset(tempBuffer, 0, sizeof(tempBuffer)); // 确保缓冲区初始化
    int len = vsnprintf(tempBuffer, sizeof(tempBuffer), formatBuffer, args);

    if (len > 0) {
        // vsnprintf已确保字符串以null结尾，但仍需检查
        if (len >= (int)sizeof(tempBuffer)) {
            tempBuffer[sizeof(tempBuffer) - 1] = '\0';
        }
        Serial.print(tempBuffer);
    }

    va_end(args);

    Serial.println();
}

void setLogLevel(LogLevel level) {
    if (level >= LOG_LEVEL_ERROR && level <= LOG_LEVEL_VERBOSE) {
        logConfig.currentLevel = level;
        LOG_INFO("Log level set to: %s", getLogLevelName(level));
    }
}

void enableLogger(bool enable) {
    logConfig.enabled = enable;
    LOG_INFO("Logger %s", enable ? "enabled" : "disabled");
}

void enableTimestamp(bool enable) {
    logConfig.timestampEnabled = enable;
    LOG_INFO("Timestamp %s", enable ? "enabled" : "disabled");
}

const char* getLogLevelName(LogLevel level) {
    static char buffer[10];
    int safeLevel = (level >= LOG_LEVEL_ERROR && level <= LOG_LEVEL_VERBOSE) ? level : LOG_LEVEL_INFO;
    strncpy_P(buffer, (const char*)pgm_read_ptr(&LOG_LEVEL_NAMES[safeLevel]), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    return buffer;
}

void adjustLogLevelForError(ErrorCode code) {
    // 根据错误代码自动调整日志级别
    switch (code) {
        case ERROR_NONE:
        case ERROR_RTC_TIME_INVALID:
        case ERROR_TIME_SOURCE_UNAVAILABLE:
            // 这些错误可能不需要提高日志级别
            break;
        case ERROR_RTC_INIT_FAILED:
        case ERROR_RTC_I2C_ERROR:
        case ERROR_WIFI_CONNECTION_FAILED:
        case ERROR_NTP_CONNECTION_FAILED:
            // 重要错误，可以考虑提高日志级别
            if (logConfig.currentLevel < LOG_LEVEL_INFO) {
                logConfig.currentLevel = LOG_LEVEL_INFO;
            }
            break;
        case ERROR_SYSTEM_WATCHDOG_TIMEOUT:
        case ERROR_TIME_SETTING_INVALID:
            // 严重错误，提高日志级别
            logConfig.currentLevel = LOG_LEVEL_INFO;
            break;
        default:
            break;
    }
}