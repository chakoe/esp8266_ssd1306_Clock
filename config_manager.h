/**
 * @file config_manager.h
 * @brief 配置管理模块
 *
 * 提供统一的配置管理接口，支持配置的加载、保存和验证
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2026-02-04
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include "eeprom_config.h"

// 配置项ID枚举
typedef enum {
    CONFIG_BRIGHTNESS_INDEX = 0,     // 亮度索引
    CONFIG_TIME_SOURCE,              // 时间源
    CONFIG_WIFI_SSID,                // WiFi SSID
    CONFIG_WIFI_PASSWORD,            // WiFi密码（加密）
    CONFIG_NTP_SERVER,               // NTP服务器
    CONFIG_TIMEZONE_OFFSET,          // 时区偏移
    CONFIG_AUTO_UPDATE_ENABLED,      // 自动更新启用
    CONFIG_DEBUG_MODE,               // 调试模式
    CONFIG_COUNT                     // 配置项总数
} ConfigId;

// 配置项类型枚举
typedef enum {
    CONFIG_TYPE_UINT8,               // 8位无符号整数
    CONFIG_TYPE_UINT16,              // 16位无符号整数
    CONFIG_TYPE_UINT32,              // 32位无符号整数
    CONFIG_TYPE_INT,                 // 有符号整数
    CONFIG_TYPE_STRING,              // 字符串
    CONFIG_TYPE_BOOL                 // 布尔值
} ConfigType;

// 配置项结构体
typedef struct {
    ConfigId id;                     // 配置ID
    ConfigType type;                 // 配置类型
    uint16_t eepromAddress;          // EEPROM地址
    uint16_t maxSize;               // 最大大小
    const char* name;               // 配置名称
    const char* description;        // 配置描述
    void* defaultValue;             // 默认值
} ConfigItem;

// 配置验证函数类型
typedef bool (*ConfigValidator)(const void* value);

// 配置管理结构体
typedef struct {
    bool initialized;                // 是否初始化
    uint16_t configVersion;         // 配置版本
    uint32_t checksum;              // 校验和
} ConfigManagerState;

// 全局配置管理状态
extern ConfigManagerState configManagerState;

// 函数声明
void initConfigManager();
bool loadConfig(ConfigId id, void* value, size_t size);
bool saveConfig(ConfigId id, const void* value, size_t size);
bool resetConfig(ConfigId id);
bool resetAllConfigs();
bool validateConfig(ConfigId id, const void* value);
const char* getConfigName(ConfigId id);
const char* getConfigDescription(ConfigId id);
ConfigType getConfigType(ConfigId id);
void printAllConfigs();
uint16_t getConfigVersion();
bool setConfigVersion(uint16_t version);
uint32_t calculateConfigChecksum();
bool verifyConfigChecksum();

#endif // CONFIG_MANAGER_H
