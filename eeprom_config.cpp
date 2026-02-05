/**
 * @file eeprom_config.cpp
 * @brief EEPROM配置管理实现文件
 *
 * 实现使用EEPROM持久化存储的配置项
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 2.1
 * @date 2025-02-03
 */

#include "eeprom_config.h"
#include "logger.h"

/**
 * @brief CRC8校验算法（使用多项式0x07，与Dallas/Maxim格式兼容）
 * @param data 数据指针
 * @param len 数据长度
 * @return CRC8校验值
 */
static uint8_t crc8(const uint8_t* data, size_t len) {
    uint8_t crc = 0x00;
    const uint8_t polynomial = 0x07;

    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

/**
 * @brief 初始化EEPROM
 * @note 必须在setup()中调用
 */
void initEEPROM() {
    // ESP8266需要先调用EEPROM.begin()来初始化
    EEPROM.begin(EEPROM_SIZE);
    LOG_DEBUG("EEPROM initialized, size: %d bytes", EEPROM_SIZE);
}

/**
 * @brief 计算校验和（使用CRC8算法）
 * @param config 配置结构体指针
 * @return CRC8校验值
 */
uint8_t calculateChecksum(const EEPROMConfig* config) {
    if (config == nullptr) return 0;

    // 构建用于CRC计算的数据数组
    uint8_t data[3];
    data[0] = config->brightnessIndex;
    data[1] = config->magicNumber & 0xFF;
    data[2] = (config->magicNumber >> 8) & 0xFF;

    // 返回CRC8校验值
    return crc8(data, sizeof(data));
}

/**
 * @brief 验证EEPROM数据有效性
 * @return true 数据有效，false 数据无效
 */
bool validateEEPROM() {
    // 读取魔数
    uint16_t magicNumber = EEPROM.read(EEPROM_ADDR_MAGIC_NUMBER);
    magicNumber |= (EEPROM.read(EEPROM_ADDR_MAGIC_NUMBER + 1) << 8);

    // 读取校验和
    uint8_t storedChecksum = EEPROM.read(EEPROM_ADDR_CHECKSUM);

    // 读取亮度索引
    uint8_t brightnessIndex = EEPROM.read(EEPROM_ADDR_BRIGHTNESS_INDEX);

    // 构建配置结构体
    EEPROMConfig config;
    config.brightnessIndex = brightnessIndex;
    config.magicNumber = magicNumber;
    config.checksum = storedChecksum;

    // 验证魔数
    if (config.magicNumber != EEPROM_MAGIC_NUMBER) {
        LOG_DEBUG("EEPROM magic number mismatch: 0x%04X (expected: 0x%04X)",
                  config.magicNumber, EEPROM_MAGIC_NUMBER);
        return false;
    }

    // 验证亮度索引范围
    if (config.brightnessIndex > 3) {
        LOG_DEBUG("EEPROM brightness index out of range: %d", config.brightnessIndex);
        return false;
    }

    // 计算并验证CRC8校验和
    uint8_t calculatedChecksum = calculateChecksum(&config);
    if (config.checksum != calculatedChecksum) {
        LOG_DEBUG("EEPROM CRC8 checksum mismatch: stored=0x%02X, calculated=0x%02X",
                  config.checksum, calculatedChecksum);
        return false;
    }

    LOG_DEBUG("EEPROM data validated successfully");
    return true;
}

/**
 * @brief 保存亮度索引到EEPROM
 * @param brightnessIndex 亮度索引 (0-3)
 * @return true 保存成功，false 保存失败
 */
bool saveBrightnessIndex(uint8_t brightnessIndex) {
    // 验证亮度索引范围
    if (brightnessIndex > 3) {
        LOG_WARNING("Invalid brightness index: %d", brightnessIndex);
        return false;
    }

    // 构建配置结构体
    EEPROMConfig config;
    config.brightnessIndex = brightnessIndex;
    config.magicNumber = EEPROM_MAGIC_NUMBER;
    config.checksum = 0;

    // 计算CRC8校验和
    config.checksum = calculateChecksum(&config);

    // 写入EEPROM
    EEPROM.write(EEPROM_ADDR_BRIGHTNESS_INDEX, config.brightnessIndex);
    EEPROM.write(EEPROM_ADDR_MAGIC_NUMBER, config.magicNumber & 0xFF);
    EEPROM.write(EEPROM_ADDR_MAGIC_NUMBER + 1, (config.magicNumber >> 8) & 0xFF);
    EEPROM.write(EEPROM_ADDR_CHECKSUM, config.checksum);

    // 提交更改到Flash
    bool success = EEPROM.commit();

    if (success) {
        LOG_DEBUG("Brightness index saved to EEPROM: %d (CRC8: 0x%02X)", brightnessIndex, config.checksum);
    } else {
        LOG_WARNING("Failed to save brightness index to EEPROM");
    }

    return success;
}

/**
 * @brief 从EEPROM加载亮度索引
 * @return 亮度索引 (0-3)，如果EEPROM无效则返回默认值2
 */
uint8_t loadBrightnessIndex() {
    // 验证EEPROM数据有效性
    if (!validateEEPROM()) {
        LOG_DEBUG("EEPROM data invalid, using default brightness index: 2");
        return 2;  // 返回默认亮度索引
    }

    // 读取亮度索引
    uint8_t brightnessIndex = EEPROM.read(EEPROM_ADDR_BRIGHTNESS_INDEX);

    LOG_DEBUG("Brightness index loaded from EEPROM: %d", brightnessIndex);
    return brightnessIndex;
}

/**
 * @brief 清除EEPROM中的所有数据
 * @note 将所有字节设置为0xFF
 */
void clearEEPROM() {
    for (int i = 0; i < EEPROM_SIZE; i++) {
        EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();
    LOG_DEBUG("EEPROM cleared");
}
