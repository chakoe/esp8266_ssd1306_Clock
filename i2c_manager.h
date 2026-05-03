/**
 * @file i2c_manager.h
 * @brief I2C通信管理模块
 * 
 * 提供健壮的I2C通信错误处理和恢复机制
 * 
 * @author ESP8266 SSD1306 Clock Project
 * @version 2.0
 * @date 2025-12-23
 */

#ifndef I2C_MANAGER_H
#define I2C_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include "global_config.h"
#include "logger.h"

// I2C设备地址定义
#define I2C_ADDRESS_RTC 0x68      // DS1307 RTC地址
#define I2C_ADDRESS_OLED 0x3C     // SSD1306 OLED地址

// I2C错误代码
enum I2CErrorCode {
    I2C_ERROR_NONE = 0,
    I2C_ERROR_BUS_BUSY = 1,
    I2C_ERROR_ADDRESS_NACK = 2,
    I2C_ERROR_DATA_NACK = 3,
    I2C_ERROR_ARBITRATION_LOST = 4,
    I2C_ERROR_TIMEOUT = 5,
    I2C_ERROR_UNKNOWN = 6
};

// I2C设备状态结构体
struct I2CDeviceStatus {
    bool initialized;           // 设备是否初始化成功
    bool connected;            // 设备当前是否连接
    unsigned long lastCheck;   // 最后检查时间
    int errorCount;            // 错误计数
    int maxRetries;            // 最大重试次数
};

// I2C管理器配置
struct I2CConfig {
    I2CDeviceStatus rtcStatus;
    I2CDeviceStatus oledStatus;
    unsigned long checkInterval;    // 设备检查间隔
    int maxConsecutiveErrors;       // 最大连续错误次数
    bool autoRecoveryEnabled;       // 是否启用自动恢复
};

extern I2CConfig i2cConfig;

// 函数声明
bool initI2CManager();
bool checkI2CDevice(uint8_t address, I2CDeviceStatus* status);
I2CErrorCode getI2CError(byte errorCode);
const char* getI2CErrorString(I2CErrorCode error);
bool resetI2CBus();
bool recoverI2CDevice(uint8_t address);
void updateI2CDeviceStatus();
bool isI2CDeviceAvailable(uint8_t address);
bool writeI2CRegister(uint8_t address, uint8_t reg, uint8_t value);
bool readI2CRegister(uint8_t address, uint8_t reg, uint8_t* value);

#endif