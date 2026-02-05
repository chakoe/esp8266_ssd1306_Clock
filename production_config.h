/**
 * @file production_config.h
 * @brief 生产环境配置文件
 *
 * 生产环境下的配置选项，禁用调试输出等
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 2.0
 * @date 2025-12-23
 */

#ifndef PRODUCTION_CONFIG_H
#define PRODUCTION_CONFIG_H

// 生产环境配置选项

// 启用生产模式（禁用调试日志）
// 注意：如果未通过编译标志定义，则在此处定义
#ifndef PRODUCTION_MODE
    #define PRODUCTION_MODE
#endif

// 日志级别设置
#ifdef PRODUCTION_MODE
    #define DEFAULT_LOG_LEVEL LOG_LEVEL_WARNING  // 生产环境：仅输出错误和警告
    #define ENABLE_TIMESTAMP false               // 生产环境：禁用时间戳
    #define ENABLE_DEBUG_LOGS false              // 生产环境：禁用调试日志
#else
    #define DEFAULT_LOG_LEVEL LOG_LEVEL_INFO     // 开发环境：输出信息级别及以上
    #define ENABLE_TIMESTAMP true                // 开发环境：启用时间戳
    #define ENABLE_DEBUG_LOGS true               // 开发环境：启用调试日志
#endif

// 系统配置
#define WATCHDOG_TIMEOUT_MS 8000                 // 硬件看门狗超时时间
#define WIFI_TIMEOUT_SECONDS 30                  // WiFi连接超时时间
#define NTP_SYNC_INTERVAL_MS 60000               // NTP同步间隔

// 电源管理配置
#define AUTO_DIM_ENABLED true                    // 启用自动调暗
#define AUTO_DIM_TIMEOUT_MS 300000               // 5分钟自动调暗
#define AUTO_SLEEP_TIMEOUT_MS 1800000            // 30分钟自动睡眠

// 错误处理配置
#define MAX_I2C_ERROR_COUNT 5                    // I2C最大错误次数
#define I2C_CHECK_INTERVAL_MS 30000              // I2C检查间隔

// WiFiManager配置
#define WIFI_MANAGER_AP_PASSWORD ""              // WiFiManager AP密码（空表示不设置密码，方便用户配置）
#define WIFI_MANAGER_AP_TIMEOUT 180              // WiFiManager AP超时时间（秒）
#define WIFI_MANAGER_CONNECT_TIMEOUT 30          // WiFiManager连接超时时间（秒）

// 内存优化配置
#define ENABLE_MEMORY_OPTIMIZATION true          // 启用内存优化
#define USE_PROGMEM_FOR_STRINGS true             // 使用PROGMEM存储字符串
#define OPTIMIZE_BUFFER_SIZES true               // 优化缓冲区大小

// 编译器优化配置
#ifdef PRODUCTION_MODE
    #define NDEBUG                              // 禁用assert
    #define F_CPU 80000000L                      // CPU频率80MHz（降低功耗）
#else
    #define DEBUG_MODE                           // 调试模式
#endif

#endif