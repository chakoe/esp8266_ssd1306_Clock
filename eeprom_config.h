/**
 * @file eeprom_config.h
 * @brief EEPROM配置管理头文件
 *
 * 管理使用EEPROM持久化存储的配置项
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 2.0
 * @date 2025-02-03
 */

#ifndef EEPROM_CONFIG_H
#define EEPROM_CONFIG_H

#include <Arduino.h>
#include <EEPROM.h>

// EEPROM地址定义
#define EEPROM_ADDR_BRIGHTNESS_INDEX  0    // 亮度索引 (1字节)
#define EEPROM_ADDR_MAGIC_NUMBER      1    // 魔数标识 (2字节)
#define EEPROM_ADDR_CHECKSUM          3    // 校验和 (1字节)

// EEPROM大小
#define EEPROM_SIZE 64  // ESP8266默认EEPROM大小为512字节，这里只使用前64字节

// 魔数标识，用于验证EEPROM数据有效性
#define EEPROM_MAGIC_NUMBER 0xA5C3  // 随机选择的魔数

// 配置结构体
struct EEPROMConfig {
    uint8_t brightnessIndex;  // 亮度索引 (0-3)
    uint16_t magicNumber;     // 魔数标识
    uint8_t checksum;         // 校验和
};

// 函数声明
/**
 * @brief 初始化EEPROM
 * @note 必须在setup()中调用
 */
void initEEPROM();

/**
 * @brief 保存亮度索引到EEPROM
 * @param brightnessIndex 亮度索引 (0-3)
 * @return true 保存成功，false 保存失败
 */
bool saveBrightnessIndex(uint8_t brightnessIndex);

/**
 * @brief 从EEPROM加载亮度索引
 * @return 亮度索引 (0-3)，如果EEPROM无效则返回默认值2
 */
uint8_t loadBrightnessIndex();

/**
 * @brief 清除EEPROM中的所有数据
 * @note 将所有字节设置为0xFF
 */
void clearEEPROM();

/**
 * @brief 验证EEPROM数据有效性
 * @return true 数据有效，false 数据无效
 */
bool validateEEPROM();

/**
 * @brief 计算校验和
 * @param config 配置结构体指针
 * @return 校验和
 */
uint8_t calculateChecksum(const EEPROMConfig* config);

#endif
