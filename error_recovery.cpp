/**
 * @file error_recovery.cpp
 * @brief 错误恢复模块实现
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2026-02-04
 */

#include "error_recovery.h"
#include "system_manager.h"
#include "time_manager.h"
#include "button_handler.h"
#include "utils.h"
#include <ESP8266WiFi.h>

// 全局错误恢复配置
ErrorRecoveryConfig errorRecoveryConfig = {
    3,          // maxRetries
    1000,       // retryDelay (1秒)
    true,       // enableAutoRecovery
    true,       // enableFallback
    true        // enableResetOnCritical
};

// 全局错误恢复状态
ErrorRecoveryState errorRecoveryState = {
    0,          // retryCount
    0,          // lastErrorTime
    0,          // lastRecoveryTime
    false,      // recoveryInProgress
    false       // recoverySucceeded
};

// 错误恢复规则表
static const ErrorRecoveryRule recoveryRules[] = {
    // RTC错误
    {
        ERROR_RTC_INIT_FAILED,
        ERROR_LEVEL_ERROR,
        RECOVERY_STRATEGY_FALLBACK,
        3,
        2000
    },
    {
        ERROR_RTC_I2C_ERROR,
        ERROR_LEVEL_ERROR,
        RECOVERY_STRATEGY_RETRY,
        5,
        500
    },
    {
        ERROR_RTC_TIME_INVALID,
        ERROR_LEVEL_WARNING,
        RECOVERY_STRATEGY_FALLBACK,
        1,
        0
    },
    // WiFi错误
    {
        ERROR_WIFI_CONNECTION_FAILED,
        ERROR_LEVEL_WARNING,
        RECOVERY_STRATEGY_RETRY,
        10,
        5000
    },
    // NTP错误
    {
        ERROR_NTP_CONNECTION_FAILED,
        ERROR_LEVEL_WARNING,
        RECOVERY_STRATEGY_FALLBACK,
        3,
        3000
    },
    // 时间源错误
    {
        ERROR_TIME_SOURCE_UNAVAILABLE,
        ERROR_LEVEL_ERROR,
        RECOVERY_STRATEGY_FALLBACK,
        2,
        1000
    },
    {
        ERROR_TIME_SETTING_INVALID,
        ERROR_LEVEL_ERROR,
        RECOVERY_STRATEGY_NONE,
        0,
        0
    },
    // 系统错误
    {
        ERROR_SYSTEM_WATCHDOG_TIMEOUT,
        ERROR_LEVEL_CRITICAL,
        RECOVERY_STRATEGY_RESTART,
        0,
        0
    },
    {
        ERROR_BUTTON_STATE_INVALID,
        ERROR_LEVEL_WARNING,
        RECOVERY_STRATEGY_RESET,
        1,
        0
    }
};

// 恢复规则数量
static const int recoveryRuleCount = sizeof(recoveryRules) / sizeof(recoveryRules[0]);

/**
 * @brief 初始化错误恢复模块
 */
void initErrorRecovery() {
    errorRecoveryState.retryCount = 0;
    errorRecoveryState.lastErrorTime = 0;
    errorRecoveryState.lastRecoveryTime = 0;
    errorRecoveryState.recoveryInProgress = false;
    errorRecoveryState.recoverySucceeded = false;

    LOG_INFO("Error Recovery initialized");
    LOG_INFO("Auto recovery: %s", errorRecoveryConfig.enableAutoRecovery ? "enabled" : "disabled");
    LOG_INFO("Fallback: %s", errorRecoveryConfig.enableFallback ? "enabled" : "disabled");
}

/**
 * @brief 带恢复的错误处理
 * @param code 错误代码
 * @param level 错误级别
 * @param message 错误消息
 * @return true 恢复成功，false 恢复失败
 */
bool handleErrorWithRecovery(ErrorCode code, ErrorLevel level, const char* message) {
    // 记录错误
    handleError(code, level, message);

    // 如果禁用自动恢复，直接返回
    if (!errorRecoveryConfig.enableAutoRecovery) {
        return false;
    }

    // 更新错误状态
    errorRecoveryState.lastErrorTime = millis();

    // 尝试恢复
    return attemptRecovery(code, level);
}

/**
 * @brief 尝试恢复
 * @param code 错误代码
 * @param level 错误级别
 * @return true 恢复成功，false 恢复失败
 */
bool attemptRecovery(ErrorCode code, ErrorLevel level) {
    // 查找恢复规则
    const ErrorRecoveryRule* rule = nullptr;
    for (int i = 0; i < recoveryRuleCount; i++) {
        if (recoveryRules[i].code == code) {
            rule = &recoveryRules[i];
            break;
        }
    }

    if (rule == nullptr) {
        LOG_DEBUG("No recovery rule for error code: %d", code);
        return false;
    }

    // 检查是否已经在恢复中
    if (errorRecoveryState.recoveryInProgress) {
        LOG_DEBUG("Recovery already in progress");
        return false;
    }

    errorRecoveryState.recoveryInProgress = true;
    errorRecoveryState.recoverySucceeded = false;

    LOG_INFO("Attempting recovery for: %s (strategy: %s)",
             getErrorDescription(code),
             getRecoveryStrategyString(rule->strategy));

    bool success = false;

    switch (rule->strategy) {
        case RECOVERY_STRATEGY_RETRY:
            success = retryOperation(code, nullptr);
            break;

        case RECOVERY_STRATEGY_FALLBACK:
            success = fallbackToAlternative(code);
            break;

        case RECOVERY_STRATEGY_RESET:
            LOG_INFO("Resetting system state");
            resetErrorRecoveryState();
            success = true;
            break;

        case RECOVERY_STRATEGY_RESTART:
            LOG_WARNING("Critical error, restarting system");
            nonBlockingDelay(1000);  // 给日志时间输出
            ESP.restart();
            break;

        default:
            LOG_DEBUG("No recovery strategy for error: %d", code);
            success = false;
            break;
    }

    errorRecoveryState.lastRecoveryTime = millis();
    errorRecoveryState.recoveryInProgress = false;
    errorRecoveryState.recoverySucceeded = success;

    if (success) {
        LOG_INFO("Recovery successful for: %s", getErrorDescription(code));
    } else {
        LOG_WARNING("Recovery failed for: %s", getErrorDescription(code));
    }

    return success;
}

/**
 * @brief 重试操作
 * @param code 错误代码
 * @param operation 操作函数
 * @return true 成功，false 失败
 */
bool retryOperation(ErrorCode code, bool (*operation)(void)) {
    // 查找重试配置
    uint8_t maxRetries = errorRecoveryConfig.maxRetries;
    unsigned long retryDelay = errorRecoveryConfig.retryDelay;

    for (int i = 0; i < recoveryRuleCount; i++) {
        if (recoveryRules[i].code == code) {
            maxRetries = recoveryRules[i].maxRetries;
            retryDelay = recoveryRules[i].retryDelay;
            break;
        }
    }

    errorRecoveryState.retryCount = 0;

    while (errorRecoveryState.retryCount < maxRetries) {
        errorRecoveryState.retryCount++;
        LOG_DEBUG("Retry attempt %d/%d for error: %d",
                 errorRecoveryState.retryCount, maxRetries, code);

        // 如果提供了操作函数，执行它
        if (operation != nullptr) {
            if (operation()) {
                return true;
            }
        }

        // 根据错误类型执行特定的重试逻辑
        bool success = false;
        switch (code) {
            case ERROR_RTC_I2C_ERROR:
                // 重试RTC初始化
                success = initializeRTC();
                break;

            case ERROR_WIFI_CONNECTION_FAILED:
                // 重试WiFi连接
                success = (WiFi.status() == WL_CONNECTED);
                break;

            case ERROR_NTP_CONNECTION_FAILED:
                // 重试NTP连接
                success = checkNtpConnection(true);
                break;

            default:
                break;
        }

        if (success) {
            return true;
        }

        // 等待重试延迟
        if (errorRecoveryState.retryCount < maxRetries) {
            nonBlockingDelay(retryDelay);
        }
    }

    return false;
}

/**
 * @brief 降级到替代方案
 * @param code 错误代码
 * @return true 成功，false 失败
 */
bool fallbackToAlternative(ErrorCode code) {
    if (!errorRecoveryConfig.enableFallback) {
        return false;
    }

    LOG_INFO("Falling back to alternative for: %s", getErrorDescription(code));

    bool success = false;

    switch (code) {
        case ERROR_RTC_INIT_FAILED:
        case ERROR_RTC_TIME_INVALID:
            // 降级到NTP时间源
            if (systemState.networkConnected) {
                switchTimeSource(TIME_SOURCE_NTP);
                success = true;
                LOG_INFO("Fallback to NTP time source");
            } else if (timeState.softwareClockValid) {
                switchTimeSource(TIME_SOURCE_MANUAL);
                success = true;
                LOG_INFO("Fallback to software clock");
            }
            break;

        case ERROR_NTP_CONNECTION_FAILED:
            // 降级到RTC时间源
            if (systemState.rtcInitialized && systemState.rtcTimeValid) {
                switchTimeSource(TIME_SOURCE_RTC);
                success = true;
                LOG_INFO("Fallback to RTC time source");
            } else if (timeState.softwareClockValid) {
                switchTimeSource(TIME_SOURCE_MANUAL);
                success = true;
                LOG_INFO("Fallback to software clock");
            }
            break;

        case ERROR_TIME_SOURCE_UNAVAILABLE:
            // 尝试所有可用时间源
            if (systemState.rtcInitialized && systemState.rtcTimeValid) {
                switchTimeSource(TIME_SOURCE_RTC);
                success = true;
            } else if (systemState.networkConnected) {
                switchTimeSource(TIME_SOURCE_NTP);
                success = true;
            } else if (timeState.softwareClockValid) {
                switchTimeSource(TIME_SOURCE_MANUAL);
                success = true;
            }
            break;

        default:
            LOG_DEBUG("No fallback available for: %s", getErrorDescription(code));
            break;
    }

    return success;
}

/**
 * @brief 重置错误恢复状态
 */
void resetErrorRecoveryState() {
    errorRecoveryState.retryCount = 0;
    errorRecoveryState.recoveryInProgress = false;
    errorRecoveryState.recoverySucceeded = false;

    // 重置按键状态
    initButtons();

    LOG_DEBUG("Error recovery state reset");
}

/**
 * @brief 设置错误恢复配置
 * @param maxRetries 最大重试次数
 * @param retryDelay 重试延迟
 * @param enableAutoRecovery 启用自动恢复
 * @param enableFallback 启用降级
 * @param enableResetOnCritical 严重错误时重置
 */
void setErrorRecoveryConfig(uint8_t maxRetries, unsigned long retryDelay,
                            bool enableAutoRecovery, bool enableFallback,
                            bool enableResetOnCritical) {
    errorRecoveryConfig.maxRetries = maxRetries;
    errorRecoveryConfig.retryDelay = retryDelay;
    errorRecoveryConfig.enableAutoRecovery = enableAutoRecovery;
    errorRecoveryConfig.enableFallback = enableFallback;
    errorRecoveryConfig.enableResetOnCritical = enableResetOnCritical;

    LOG_INFO("Error recovery config updated");
    LOG_INFO("  Max retries: %d", maxRetries);
    LOG_INFO("  Retry delay: %lu ms", retryDelay);
    LOG_INFO("  Auto recovery: %s", enableAutoRecovery ? "enabled" : "disabled");
    LOG_INFO("  Fallback: %s", enableFallback ? "enabled" : "disabled");
}

/**
 * @brief 打印错误恢复统计
 */
void printErrorRecoveryStats() {
    LOG_INFO("========================================");
    LOG_INFO("  Error Recovery Statistics");
    LOG_INFO("========================================");
    LOG_INFO("Retry count: %d", errorRecoveryState.retryCount);
    LOG_INFO("Last error time: %lu ms", errorRecoveryState.lastErrorTime);
    LOG_INFO("Last recovery time: %lu ms", errorRecoveryState.lastRecoveryTime);
    LOG_INFO("Recovery in progress: %s", errorRecoveryState.recoveryInProgress ? "yes" : "no");
    LOG_INFO("Last recovery: %s", errorRecoveryState.recoverySucceeded ? "success" : "failed");
    LOG_INFO("========================================");
}

/**
 * @brief 获取恢复策略字符串
 * @param strategy 恢复策略
 * @return 策略字符串
 */
const char* getRecoveryStrategyString(RecoveryStrategy strategy) {
    switch (strategy) {
        case RECOVERY_STRATEGY_NONE: return "None";
        case RECOVERY_STRATEGY_RETRY: return "Retry";
        case RECOVERY_STRATEGY_FALLBACK: return "Fallback";
        case RECOVERY_STRATEGY_RESET: return "Reset";
        case RECOVERY_STRATEGY_RESTART: return "Restart";
        default: return "Unknown";
    }
}
