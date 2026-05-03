/**
 * @file config_manager.cpp
 * @brief 配置管理模块实现
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2026-02-04
 */

#include "config_manager.h"
#include "logger.h"
#include <EEPROM.h>

// 全局配置管理状态
ConfigManagerState configManagerState = {
    false,                          // initialized
    1,                              // configVersion
    0                               // checksum
};

// 配置项表
static const ConfigItem configItems[] = {
    // 亮度索引
    {
        CONFIG_BRIGHTNESS_INDEX,
        CONFIG_TYPE_UINT8,
        EEPROM_ADDR_BRIGHTNESS_INDEX,
        1,
        "brightness_index",
        "Display brightness level (0-3)",
        (void*)2  // 默认值：中等亮度
    },
    // 时间源
    {
        CONFIG_TIME_SOURCE,
        CONFIG_TYPE_UINT8,
        EEPROM_ADDR_CHECKSUM + 1,  // 紧接在基础配置之后
        1,
        "time_source",
        "Time source (0:NONE, 1:RTC, 2:NTP, 3:MANUAL)",
        (void*)2  // 默认值：NTP
    },
    // WiFi SSID
    {
        CONFIG_WIFI_SSID,
        CONFIG_TYPE_STRING,
        EEPROM_ADDR_CHECKSUM + 2,
        32,
        "wifi_ssid",
        "WiFi network SSID",
        nullptr  // 默认值：空字符串
    },
    // 时区偏移
    {
        CONFIG_TIMEZONE_OFFSET,
        CONFIG_TYPE_INT,
        EEPROM_ADDR_CHECKSUM + 34,
        4,
        "timezone_offset",
        "Timezone offset in seconds",
        (void*)28800  // 默认值：东八区（8小时）
    },
    // 自动更新启用
    {
        CONFIG_AUTO_UPDATE_ENABLED,
        CONFIG_TYPE_BOOL,
        EEPROM_ADDR_CHECKSUM + 38,
        1,
        "auto_update_enabled",
        "Enable automatic OTA updates",
        (void*)false  // 默认值：禁用
    },
    // 调试模式
    {
        CONFIG_DEBUG_MODE,
        CONFIG_TYPE_BOOL,
        EEPROM_ADDR_CHECKSUM + 39,
        1,
        "debug_mode",
        "Enable debug logging",
        (void*)false  // 默认值：禁用
    }
};

// 配置项数量
static const int configItemCount = sizeof(configItems) / sizeof(configItems[0]);

/**
 * @brief 初始化配置管理器
 */
void initConfigManager() {
    EEPROM.begin(512);  // 使用512字节EEPROM空间

    // 读取配置版本
    uint16_t storedVersion = EEPROM.read(EEPROM_ADDR_MAGIC_NUMBER + 2);
    storedVersion |= (EEPROM.read(EEPROM_ADDR_MAGIC_NUMBER + 3) << 8);

    if (storedVersion != configManagerState.configVersion) {
        // 配置版本不匹配，重置配置
        LOG_WARNING("Config version mismatch: stored=%d, current=%d",
                   storedVersion, configManagerState.configVersion);
        resetAllConfigs();
    } else {
        // 验证配置校验和
        if (!verifyConfigChecksum()) {
            LOG_WARNING("Config checksum invalid, resetting");
            resetAllConfigs();
        }
    }

    configManagerState.initialized = true;
    configManagerState.checksum = calculateConfigChecksum();

    LOG_INFO("Config Manager initialized (version %d)", configManagerState.configVersion);
}

/**
 * @brief 加载配置
 * @param id 配置ID
 * @param value 值指针
 * @param size 值大小
 * @return true 成功，false 失败
 */
bool loadConfig(ConfigId id, void* value, size_t size) {
    if (!configManagerState.initialized) {
        LOG_WARNING("Config manager not initialized");
        return false;
    }

    if (id < 0 || id >= CONFIG_COUNT) {
        LOG_WARNING("Invalid config ID: %d", id);
        return false;
    }

    const ConfigItem* item = &configItems[id];

    if (size > item->maxSize) {
        LOG_WARNING("Config size too large: %d > %d", size, item->maxSize);
        return false;
    }

    // 从EEPROM读取
    for (size_t i = 0; i < size; i++) {
        ((uint8_t*)value)[i] = EEPROM.read(item->eepromAddress + i);
    }

    // 如果值为空或无效，使用默认值
    if (item->type == CONFIG_TYPE_STRING && strlen((char*)value) == 0) {
        if (item->defaultValue != nullptr) {
            strcpy((char*)value, (char*)item->defaultValue);
        }
    }

    LOG_DEBUG("Loaded config: %s", item->name);
    return true;
}

/**
 * @brief 保存配置
 * @param id 配置ID
 * @param value 值指针
 * @param size 值大小
 * @return true 成功，false 失败
 */
bool saveConfig(ConfigId id, const void* value, size_t size) {
    if (!configManagerState.initialized) {
        LOG_WARNING("Config manager not initialized");
        return false;
    }

    if (id < 0 || id >= CONFIG_COUNT) {
        LOG_WARNING("Invalid config ID: %d", id);
        return false;
    }

    const ConfigItem* item = &configItems[id];

    if (size > item->maxSize) {
        LOG_WARNING("Config size too large: %d > %d", size, item->maxSize);
        return false;
    }

    // 验证配置值
    if (!validateConfig(id, value)) {
        LOG_WARNING("Config validation failed: %s", item->name);
        return false;
    }

    // 写入EEPROM
    for (size_t i = 0; i < size; i++) {
        EEPROM.write(item->eepromAddress + i, ((const uint8_t*)value)[i]);
    }

    // 更新校验和
    configManagerState.checksum = calculateConfigChecksum();

    // 提交更改
    bool success = EEPROM.commit();

    if (success) {
        LOG_DEBUG("Saved config: %s", item->name);
    } else {
        LOG_WARNING("Failed to save config: %s", item->name);
    }

    return success;
}

/**
 * @brief 重置单个配置
 * @param id 配置ID
 * @return true 成功，false 失败
 */
bool resetConfig(ConfigId id) {
    if (id < 0 || id >= CONFIG_COUNT) {
        LOG_WARNING("Invalid config ID: %d", id);
        return false;
    }

    const ConfigItem* item = &configItems[id];

    if (item->defaultValue == nullptr) {
        // 无默认值，清零
        uint8_t buffer[32] = {0};
        return saveConfig(id, buffer, item->maxSize);
    } else {
        // 使用默认值
        if (item->type == CONFIG_TYPE_STRING) {
            return saveConfig(id, item->defaultValue, strlen((char*)item->defaultValue) + 1);
        } else {
            return saveConfig(id, &item->defaultValue, item->maxSize);
        }
    }
}

/**
 * @brief 重置所有配置
 * @return true 成功，false 失败
 */
bool resetAllConfigs() {
    LOG_INFO("Resetting all configs...");

    for (int i = 0; i < configItemCount; i++) {
        if (!resetConfig((ConfigId)i)) {
            LOG_WARNING("Failed to reset config: %s", configItems[i].name);
        }
    }

    // 重置配置版本
    EEPROM.write(EEPROM_ADDR_MAGIC_NUMBER + 2, configManagerState.configVersion & 0xFF);
    EEPROM.write(EEPROM_ADDR_MAGIC_NUMBER + 3, (configManagerState.configVersion >> 8) & 0xFF);

    configManagerState.checksum = calculateConfigChecksum();
    bool success = EEPROM.commit();

    if (success) {
        LOG_INFO("All configs reset successfully");
    } else {
        LOG_WARNING("Failed to reset all configs");
    }

    return success;
}

/**
 * @brief 验证配置
 * @param id 配置ID
 * @param value 值指针
 * @return true 有效，false 无效
 */
bool validateConfig(ConfigId id, const void* value) {
    if (id < 0 || id >= CONFIG_COUNT) {
        return false;
    }

    const ConfigItem* item = &configItems[id];

    switch (id) {
        case CONFIG_BRIGHTNESS_INDEX:
            return (*(uint8_t*)value >= 0 && *(uint8_t*)value <= 3);

        case CONFIG_TIME_SOURCE:
            return (*(uint8_t*)value >= 0 && *(uint8_t*)value <= 3);

        case CONFIG_TIMEZONE_OFFSET:
            return *(int*)value >= -43200 && *(int*)value <= 50400;  // -12到+14小时

        default:
            return true;
    }
}

/**
 * @brief 获取配置名称
 * @param id 配置ID
 * @return 配置名称
 */
const char* getConfigName(ConfigId id) {
    if (id < 0 || id >= CONFIG_COUNT) {
        return "Unknown";
    }
    return configItems[id].name;
}

/**
 * @brief 获取配置描述
 * @param id 配置ID
 * @return 配置描述
 */
const char* getConfigDescription(ConfigId id) {
    if (id < 0 || id >= CONFIG_COUNT) {
        return "Unknown config";
    }
    return configItems[id].description;
}

/**
 * @brief 获取配置类型
 * @param id 配置ID
 * @return 配置类型
 */
ConfigType getConfigType(ConfigId id) {
    if (id < 0 || id >= CONFIG_COUNT) {
        return CONFIG_TYPE_UINT8;
    }
    return configItems[id].type;
}

/**
 * @brief 打印所有配置
 */
void printAllConfigs() {
    LOG_INFO("========================================");
    LOG_INFO("  Current Configuration");
    LOG_INFO("========================================");

    for (int i = 0; i < configItemCount; i++) {
        const ConfigItem* item = &configItems[i];
        LOG_INFO("%s (%s):", item->name, item->description);

        // 读取并显示配置值
        switch (item->type) {
            case CONFIG_TYPE_UINT8: {
                uint8_t value;
                loadConfig((ConfigId)i, &value, sizeof(value));
                LOG_INFO("  Value: %u", value);
                break;
            }
            case CONFIG_TYPE_UINT16: {
                uint16_t value;
                loadConfig((ConfigId)i, &value, sizeof(value));
                LOG_INFO("  Value: %u", value);
                break;
            }
            case CONFIG_TYPE_UINT32: {
                uint32_t value;
                loadConfig((ConfigId)i, &value, sizeof(value));
                LOG_INFO("  Value: %lu", value);
                break;
            }
            case CONFIG_TYPE_INT: {
                int value;
                loadConfig((ConfigId)i, &value, sizeof(value));
                LOG_INFO("  Value: %d", value);
                break;
            }
            case CONFIG_TYPE_STRING: {
                char value[32];
                loadConfig((ConfigId)i, value, sizeof(value));
                LOG_INFO("  Value: %s", value);
                break;
            }
            case CONFIG_TYPE_BOOL: {
                bool value;
                loadConfig((ConfigId)i, &value, sizeof(value));
                LOG_INFO("  Value: %s", value ? "true" : "false");
                break;
            }
        }
    }

    LOG_INFO("========================================");
}

/**
 * @brief 获取配置版本
 * @return 配置版本号
 */
uint16_t getConfigVersion() {
    return configManagerState.configVersion;
}

/**
 * @brief 设置配置版本
 * @param version 版本号
 * @return true 成功，false 失败
 */
bool setConfigVersion(uint16_t version) {
    configManagerState.configVersion = version;
    EEPROM.write(EEPROM_ADDR_MAGIC_NUMBER + 2, version & 0xFF);
    EEPROM.write(EEPROM_ADDR_MAGIC_NUMBER + 3, (version >> 8) & 0xFF);
    return EEPROM.commit();
}

/**
 * @brief 计算配置校验和
 * @return 校验和
 */
uint32_t calculateConfigChecksum() {
    uint32_t checksum = 0;

    // 计算所有配置项的校验和
    for (int i = 0; i < configItemCount; i++) {
        const ConfigItem* item = &configItems[i];
        for (uint16_t j = 0; j < item->maxSize; j++) {
            checksum ^= EEPROM.read(item->eepromAddress + j);
            checksum = (checksum << 1) | (checksum >> 31);
        }
    }

    return checksum;
}

/**
 * @brief 验证配置校验和
 * @return true 有效，false 无效
 */
bool verifyConfigChecksum() {
    uint32_t storedChecksum = 0;
    uint16_t checksumAddr = EEPROM_ADDR_CHECKSUM + 40;  // 配置校验和地址

    for (int i = 0; i < 4; i++) {
        storedChecksum |= (EEPROM.read(checksumAddr + i) << (i * 8));
    }

    uint32_t calculatedChecksum = calculateConfigChecksum();

    if (storedChecksum != calculatedChecksum) {
        LOG_WARNING("Checksum mismatch: stored=%lu, calculated=%lu",
                   storedChecksum, calculatedChecksum);
        return false;
    }

    return true;
}
