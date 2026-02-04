/**
 * @file logger.h
 * @brief 分级日志系统
 * 
 * 提供分级的日志输出功能，支持生产环境调试控制
 * 
 * @author ESP8266 SSD1306 Clock Project
 * @version 2.0
 * @date 2025-12-23
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include "global_config.h"
#include "production_config.h"

// 日志级别枚举
enum LogLevel {
    LOG_LEVEL_ERROR = 0,    // 错误级别（必须输出）
    LOG_LEVEL_WARNING = 1,  // 警告级别
    LOG_LEVEL_INFO = 2,     // 信息级别
    LOG_LEVEL_DEBUG = 3,    // 调试级别（生产环境可关闭）
    LOG_LEVEL_VERBOSE = 4   // 详细级别（仅开发环境）
};

// 日志配置结构体
struct LogConfig {
    LogLevel currentLevel;      // 当前日志级别
    bool enabled;              // 是否启用日志
    bool timestampEnabled;     // 是否包含时间戳
    bool useFlashStrings;      // 是否使用Flash字符串
};

extern LogConfig logConfig;

// 宏定义，便于使用
#ifdef ENABLE_DEBUG_LOGS
    #define LOG_ERROR(msg, ...)     logMessage(LOG_LEVEL_ERROR, msg, ##__VA_ARGS__)
    #define LOG_WARNING(msg, ...)   logMessage(LOG_LEVEL_WARNING, msg, ##__VA_ARGS__)
    #define LOG_INFO(msg, ...)      logMessage(LOG_LEVEL_INFO, msg, ##__VA_ARGS__)
    #define LOG_DEBUG(msg, ...)     logMessage(LOG_LEVEL_DEBUG, msg, ##__VA_ARGS__)
    #define LOG_VERBOSE(msg, ...)   logMessage(LOG_LEVEL_VERBOSE, msg, ##__VA_ARGS__)
#else
    // 生产环境：仅保留错误和警告日志
    #define LOG_ERROR(msg, ...)     logMessage(LOG_LEVEL_ERROR, msg, ##__VA_ARGS__)
    #define LOG_WARNING(msg, ...)   logMessage(LOG_LEVEL_WARNING, msg, ##__VA_ARGS__)
    #define LOG_INFO(msg, ...)      // 生产环境不输出信息级别日志
    #define LOG_DEBUG(msg, ...)     // 生产环境不输出调试级别日志
    #define LOG_VERBOSE(msg, ...)   // 生产环境不输出详细级别日志
#endif

// 函数声明
void initLogger();
void logMessage(LogLevel level, const char* format, ...);
void logMessageP(LogLevel level, const char* format, ...); // 使用Flash字符串
void setLogLevel(LogLevel level);
void enableLogger(bool enable);
void enableTimestamp(bool enable);
const char* getLogLevelName(LogLevel level);
void adjustLogLevelForError(ErrorCode code); // 根据错误代码调整日志级别

#endif