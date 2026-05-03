/**
 * @file error_recovery.h
 * @brief 错误恢复模块
 *
 * 提供增强的错误检测、恢复和降级机制
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2026-02-04
 */

#ifndef ERROR_RECOVERY_H
#define ERROR_RECOVERY_H

#include <Arduino.h>
#include "config.h"
#include "logger.h"

// 错误恢复策略枚举
typedef enum {
    RECOVERY_STRATEGY_NONE,        // 无恢复
    RECOVERY_STRATEGY_RETRY,       // 重试
    RECOVERY_STRATEGY_FALLBACK,    // 降级
    RECOVERY_STRATEGY_RESET,       // 重置
    RECOVERY_STRATEGY_RESTART      // 重启
} RecoveryStrategy;

// 错误恢复配置结构体
typedef struct {
    uint8_t maxRetries;            // 最大重试次数
    unsigned long retryDelay;      // 重试延迟（毫秒）
    bool enableAutoRecovery;       // 启用自动恢复
    bool enableFallback;           // 启用降级
    bool enableResetOnCritical;    // 严重错误时重置
} ErrorRecoveryConfig;

// 错误恢复状态结构体
typedef struct {
    uint8_t retryCount;            // 当前重试次数
    unsigned long lastErrorTime;   // 上次错误时间
    unsigned long lastRecoveryTime;// 上次恢复时间
    bool recoveryInProgress;       // 恢复进行中
    bool recoverySucceeded;        // 恢复成功
} ErrorRecoveryState;

// 错误恢复规则结构体
typedef struct {
    ErrorCode code;                // 错误代码
    ErrorLevel level;              // 错误级别
    RecoveryStrategy strategy;     // 恢复策略
    uint8_t maxRetries;            // 最大重试次数
    unsigned long retryDelay;      // 重试延迟
} ErrorRecoveryRule;

// 全局错误恢复配置和状态
extern ErrorRecoveryConfig errorRecoveryConfig;
extern ErrorRecoveryState errorRecoveryState;

// 函数声明
void initErrorRecovery();
bool handleErrorWithRecovery(ErrorCode code, ErrorLevel level, const char* message);
bool attemptRecovery(ErrorCode code, ErrorLevel level);
bool retryOperation(ErrorCode code, bool (*operation)(void));
bool fallbackToAlternative(ErrorCode code);
void resetErrorRecoveryState();
void setErrorRecoveryConfig(uint8_t maxRetries, unsigned long retryDelay,
                            bool enableAutoRecovery, bool enableFallback,
                            bool enableResetOnCritical);
void printErrorRecoveryStats();
const char* getRecoveryStrategyString(RecoveryStrategy strategy);

#endif // ERROR_RECOVERY_H
