/**
 * @file production_config_complete.h
 * @brief 完整生产环境配置文件
 * 
 * 包含所有生产环境需要的配置参数和监控设置
 * 
 * @author ESP8266 SSD1306 Clock Project
 * @version 2.0 - Production
 * @date 2025-12-23
 */

#ifndef PRODUCTION_CONFIG_COMPLETE_H
#define PRODUCTION_CONFIG_COMPLETE_H

// =============================================================================
// 生产环境主配置
// =============================================================================

// 启用生产模式（禁用调试输出）
// 注意：如果未通过编译标志定义，则在此处定义
#ifndef PRODUCTION_MODE
    #define PRODUCTION_MODE
#endif

// 生产环境标识
#define PRODUCTION_VERSION "2.0"
#define PRODUCTION_BUILD_DATE "2025-12-23"

// =============================================================================
// 系统性能配置
// =============================================================================

// CPU频率设置
#define PRODUCTION_CPU_FREQ 160  // MHz - 最高性能模式

// 系统看门狗配置
#define SYSTEM_WATCHDOG_TIMEOUT_MS 8000      // 8秒系统看门狗
#define HARDWARE_WATCHDOG_TIMEOUT_MS 6000    // 6秒硬件看门狗

// 内存管理配置
#define FREE_MEMORY_CHECK_INTERVAL 300000     // 5分钟检查一次内存
#define MINIMUM_FREE_MEMORY 4000             // 最小可用内存阈值
#define MEMORY_WARNING_THRESHOLD 8000         // 内存警告阈值

// =============================================================================
// 网络配置
// =============================================================================

// WiFi连接配置
#define WIFI_CONNECTION_TIMEOUT 20000         // 20秒连接超时
#define WIFI_RECONNECT_INTERVAL 15000        // 15秒重连间隔
#define MAX_WIFI_RETRY_ATTEMPTS 5            // 最大重试次数
#define WIFI_PERSISTENT_CONFIG true           // 保存WiFi配置

// DHCP配置
#define USE_DHCP true                        // 使用DHCP
#define DHCP_TIMEOUT 10000                   // DHCP超时时间

// 静态IP配置（如使用静态IP）
// #define USE_STATIC_IP true
// #define STATIC_IP "192.168.1.100"
// #define STATIC_GATEWAY "192.168.1.1"
// #define STATIC_SUBNET "255.255.255.0"
// #define STATIC_DNS "8.8.8.8"

// =============================================================================
// 时间同步配置
// =============================================================================

// NTP配置
#define NTP_UPDATE_INTERVAL 3600000           // 1小时更新一次
#define NTP_TIMEOUT 5000                     // 5秒NTP超时
#define NTP_SERVER_POOL_SIZE 4               // NTP服务器池大小
#define NTP_RETRY_DELAY 2000                 // NTP重试延迟

// RTC配置
#define RTC_SYNC_INTERVAL 1800000            // 30分钟同步一次RTC
#define RTC_ERROR_THRESHOLD 5                 // RTC错误阈值

// =============================================================================
// 显示配置
// =============================================================================

// 显示性能配置
#define DISPLAY_UPDATE_INTERVAL 1000          // 1秒刷新间隔
#define DISPLAY_PARTIAL_UPDATE_ENABLED true   // 启用部分更新
#define DISPLAY_BUFFER_SIZE 1024              // 显示缓冲区大小

// OLED配置
#define OLED_CONTRAST_DEFAULT 0xCF            // 默认对比度
#define OLED_BRIGHTNESS_LEVELS 4              // 亮度级别数量
#define OLED_POWER_SAVE_TIMEOUT 300000        // 5分钟后省电

// =============================================================================
// 电源管理配置
// =============================================================================

// 自动省电配置
#define AUTO_SLEEP_ENABLED true               // 启用自动睡眠
#define AUTO_SLEEP_TIMEOUT_MS 1800000         // 30分钟自动睡眠
#define AUTO_DIM_ENABLED true                  // 启用自动调暗
#define AUTO_DIM_TIMEOUT_MS 300000            // 5分钟自动调暗

// 夜间模式配置
#define NIGHT_MODE_ENABLED true                // 启用夜间模式
#define NIGHT_START_HOUR 22                   // 夜间开始时间
#define NIGHT_END_HOUR 7                     // 夜间结束时间
#define NIGHT_BRIGHTNESS_LEVEL 1              // 夜间亮度级别

// =============================================================================
// 按键和交互配置
// =============================================================================

// 按键防抖配置
#define BUTTON_DEBOUNCE_MS 50                // 按键防抖时间
#define BUTTON_LONG_PRESS_MS 500              // 长按判定时间
#define BUTTON_DOUBLE_CLICK_MS 300            // 双击判定时间

// 按键看门狗
#define BUTTON_WATCHDOG_TIMEOUT 5000          // 按键看门狗超时

// =============================================================================
// 错误处理和恢复配置
// =============================================================================

// 错误恢复策略
#define AUTO_ERROR_RECOVERY true              // 自动错误恢复
#define MAX_ERROR_COUNT 10                   // 最大错误次数
#define ERROR_RESET_THRESHOLD 5               // 错误重启阈值

// I2C错误处理
#define I2C_ERROR_RETRY_COUNT 3              // I2C重试次数
#define I2C_BUS_RESET_ENABLED true           // 启用总线重置
#define I2C_ERROR_RECOVERY_DELAY 100         // I2C恢复延迟

// =============================================================================
// 监控和诊断配置
// =============================================================================

// 系统监控
#define SYSTEM_MONITOR_ENABLED true            // 启用系统监控
#define MONITOR_INTERVAL 60000                // 1分钟监控间隔
#define PERFORMANCE_LOGGING_ENABLED false      // 生产环境关闭性能日志

// 健康检查
#define HEALTH_CHECK_ENABLED true              // 启用健康检查
#define HEALTH_CHECK_INTERVAL 300000           // 5分钟健康检查
#define CRITICAL_ERROR_ALERT_ENABLED false     // 生产环境关闭错误警告

// =============================================================================
// 安全配置
// =============================================================================

// 加密和安全
#define WIFI_ENCRYPTION_ENABLED true          // 启用WiFi密码加密
#define AES_ENCRYPTION_KEY_SIZE 16           // AES密钥长度
#define PASSWORD_STORAGE_SECURE true           // 安全密码存储

// 访问控制
#define ADMIN_ACCESS_ENABLED true             // 启用管理员访问
#define CONFIG_PASSWORD_PROTECTED true        // 配置密码保护

// =============================================================================
// 日志配置
// =============================================================================

// 日志级别设置
#ifdef PRODUCTION_MODE
    #define DEFAULT_LOG_LEVEL LOG_LEVEL_WARNING  // 生产环境：仅警告和错误
    #define ENABLE_TIMESTAMP false               // 生产环境：禁用时间戳
    #define ENABLE_DEBUG_LOGS false              // 生产环境：禁用调试日志
    #define ENABLE_PERFORMANCE_LOGS false        // 生产环境：禁用性能日志
#else
    #define DEFAULT_LOG_LEVEL LOG_LEVEL_INFO     // 开发环境：信息级别
    #define ENABLE_TIMESTAMP true                // 开发环境：启用时间戳
    #define ENABLE_DEBUG_LOGS true               // 开发环境：启用调试日志
    #define ENABLE_PERFORMANCE_LOGS true         // 开发环境：启用性能日志
#endif

// 日志缓冲区配置
#define LOG_BUFFER_SIZE 512                   // 日志缓冲区大小
#define LOG_FLUSH_INTERVAL 10000              // 日志刷新间隔

// =============================================================================
// OTA更新配置
// =============================================================================

#define OTA_UPDATE_ENABLED true               // 启用OTA更新
#define OTA_PASSWORD_PROTECTED true           // OTA密码保护
#define OTA_UPDATE_CHECK_INTERVAL 86400000    // 24小时检查一次更新

// =============================================================================
// 远程监控配置
// =============================================================================

#define REMOTE_MONITORING_ENABLED false        // 生产环境默认关闭远程监控
#define TELEMETRY_INTERVAL 300000            // 5分钟遥测间隔
#define METRICS_COLLECTION_ENABLED false       // 指标收集（可选）

// =============================================================================
// 兼容性和向后兼容配置
// =============================================================================

// 向后兼容性
#define BACKWARD_COMPATIBILITY_ENABLED true    // 启用向后兼容
#define LEGACY_CONFIG_SUPPORT true            // 支持旧版配置

// 配置版本控制
#define CONFIG_VERSION 2                     // 配置文件版本
#define CONFIG_MIGRATION_ENABLED true         // 启用配置迁移

// =============================================================================
// 开发和调试配置（仅在非生产模式启用）
// =============================================================================

#ifndef PRODUCTION_MODE
    // 调试选项
    #define DEBUG_MEMORY_LEAKS true           // 内存泄漏检测
    #define DEBUG_PERFORMANCE_COUNTERS true   // 性能计数器
    #define DEBUG_STATE_MACHINE true          // 状态机调试
    
    // 测试选项
    #define UNIT_TESTS_ENABLED true            // 单元测试
    #define INTEGRATION_TESTS_ENABLED true    // 集成测试
#endif

// =============================================================================
// 验证宏定义
// =============================================================================

// 配置验证
#if !defined(PRODUCTION_MODE) && !defined(DEBUG_MODE)
    #error "必须定义 PRODUCTION_MODE 或 DEBUG_MODE"
#endif

#if defined(PRODUCTION_MODE) && defined(DEBUG_MODE)
    #error "不能同时定义 PRODUCTION_MODE 和 DEBUG_MODE"
#endif

// 硬件资源验证
#if FREE_MEMORY_CHECK_INTERVAL < 60000
    #error "内存检查间隔不能小于60秒"
#endif

#if WIFI_CONNECTION_TIMEOUT < 5000
    #error "WiFi连接超时不能小于5秒"
#endif

// 安全性验证
#if WIFI_ENCRYPTION_ENABLED && AES_ENCRYPTION_KEY_SIZE != 16
    #error "AES密钥长度必须为16字节"
#endif

#endif // PRODUCTION_CONFIG_COMPLETE_H