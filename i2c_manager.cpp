/**
 * @file i2c_manager.cpp
 * @brief I2C通信管理模块实现
 * 
 * 实现健壮的I2C通信错误处理和恢复机制
 * 
 * @author ESP8266 SSD1306 Clock Project
 * @version 2.0
 * @date 2025-12-23
 */

#include "i2c_manager.h"
#include "utils.h"

// I2C管理器配置定义
I2CConfig i2cConfig = {
    .rtcStatus = {
        .initialized = false,
        .connected = false,
        .lastCheck = 0,
        .errorCount = 0,
        .maxRetries = 3
    },
    .oledStatus = {
        .initialized = false,
        .connected = false,
        .lastCheck = 0,
        .errorCount = 0,
        .maxRetries = 3
    },
    .checkInterval = 30000,        // 30秒检查一次
    .maxConsecutiveErrors = 5,     // 最大连续错误次数
    .autoRecoveryEnabled = true    // 启用自动恢复
};

// I2C错误描述（使用Flash字符串优化）
const char I2C_ERROR_NONE_STR[] PROGMEM = "No error";
const char I2C_ERROR_BUS_BUSY_STR[] PROGMEM = "Bus busy";
const char I2C_ERROR_ADDRESS_NACK_STR[] PROGMEM = "Address NACK";
const char I2C_ERROR_DATA_NACK_STR[] PROGMEM = "Data NACK";
const char I2C_ERROR_ARBITRATION_LOST_STR[] PROGMEM = "Arbitration lost";
const char I2C_ERROR_TIMEOUT_STR[] PROGMEM = "Timeout";
const char I2C_ERROR_UNKNOWN_STR[] PROGMEM = "Unknown error";

const char* const I2C_ERROR_STRINGS[] PROGMEM = {
    I2C_ERROR_NONE_STR,
    I2C_ERROR_BUS_BUSY_STR,
    I2C_ERROR_ADDRESS_NACK_STR,
    I2C_ERROR_DATA_NACK_STR,
    I2C_ERROR_ARBITRATION_LOST_STR,
    I2C_ERROR_TIMEOUT_STR,
    I2C_ERROR_UNKNOWN_STR
};

bool initI2CManager() {
    LOG_INFO("Initializing I2C manager...");
    
    // 初始化I2C总线
    Wire.begin();
    Wire.setClock(100000); // 100kHz标准速度
    
    // 检查I2C设备
    checkI2CDevice(I2C_ADDRESS_RTC, &i2cConfig.rtcStatus);
    checkI2CDevice(I2C_ADDRESS_OLED, &i2cConfig.oledStatus);
    
    LOG_INFO("I2C manager initialized");
    LOG_INFO("RTC: %s", i2cConfig.rtcStatus.connected ? "Connected" : "Disconnected");
    LOG_INFO("OLED: %s", i2cConfig.oledStatus.connected ? "Connected" : "Disconnected");
    
    return i2cConfig.rtcStatus.connected && i2cConfig.oledStatus.connected;
}

bool checkI2CDevice(uint8_t address, I2CDeviceStatus* status) {
    unsigned long currentMillis = millis();
    
    // 检查是否需要检查设备
    if (currentMillis - status->lastCheck < i2cConfig.checkInterval && 
        status->lastCheck != 0) {
        return status->connected;
    }
    
    status->lastCheck = currentMillis;
    
    // 尝试与设备通信
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    
    I2CErrorCode i2cError = getI2CError(error);
    
    if (i2cError == I2C_ERROR_NONE) {
        status->connected = true;
        status->errorCount = 0;
        if (!status->initialized) {
            status->initialized = true;
            LOG_INFO("I2C device 0x%02X initialized", address);
        }
    } else {
        status->connected = false;
        status->errorCount++;
        
        LOG_WARNING("I2C device 0x%02X error: %s (count: %d)", 
                   address, getI2CErrorString(i2cError), status->errorCount);
        
        // 如果错误次数过多，尝试恢复
        if (status->errorCount >= i2cConfig.maxConsecutiveErrors && 
            i2cConfig.autoRecoveryEnabled) {
            LOG_WARNING("Attempting to recover I2C device 0x%02X", address);
            if (recoverI2CDevice(address)) {
                status->connected = true;
                status->errorCount = 0;
                LOG_INFO("I2C device 0x%02X recovered", address);
            }
        }
    }
    
    return status->connected;
}

I2CErrorCode getI2CError(byte errorCode) {
    switch (errorCode) {
        case 0: return I2C_ERROR_NONE;
        case 1: return I2C_ERROR_BUS_BUSY;
        case 2: return I2C_ERROR_ADDRESS_NACK;
        case 3: return I2C_ERROR_DATA_NACK;
        case 4: return I2C_ERROR_ARBITRATION_LOST;
        case 5: return I2C_ERROR_TIMEOUT;
        default: return I2C_ERROR_UNKNOWN;
    }
}

const char* getI2CErrorString(I2CErrorCode error) {
    static char buffer[30];
    if (error >= I2C_ERROR_NONE && error <= I2C_ERROR_UNKNOWN) {
        strcpy_P(buffer, (const char*)pgm_read_ptr(&I2C_ERROR_STRINGS[error]));
        return buffer;
    }
    return "Invalid error code";
}

bool resetI2CBus() {
    LOG_WARNING("Resetting I2C bus...");
    
    // ESP8266的Wire库没有end()方法，直接重新初始化
    nonBlockingDelay(100);
    Wire.begin();
    Wire.setClock(100000);
    
    LOG_INFO("I2C bus reset completed");
    return true;
}

bool recoverI2CDevice(uint8_t address) {
    LOG_WARNING("Recovering I2C device 0x%02X...", address);
    
    // 尝试重置I2C总线
    if (!resetI2CBus()) {
        LOG_ERROR("Failed to reset I2C bus");
        return false;
    }
    
    // 多次尝试连接设备
    for (int attempt = 0; attempt < 3; attempt++) {
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();
        
        if (error == 0) {
            LOG_INFO("I2C device 0x%02X recovery successful (attempt %d)", address, attempt + 1);
            return true;
        }
        
        nonBlockingDelay(100);
    }
    
    LOG_ERROR("I2C device 0x%02X recovery failed", address);
    return false;
}

void updateI2CDeviceStatus() {
    unsigned long currentMillis = millis();
    
    // 定期检查设备状态
    if (currentMillis - i2cConfig.rtcStatus.lastCheck > i2cConfig.checkInterval) {
        checkI2CDevice(I2C_ADDRESS_RTC, &i2cConfig.rtcStatus);
    }
    
    if (currentMillis - i2cConfig.oledStatus.lastCheck > i2cConfig.checkInterval) {
        checkI2CDevice(I2C_ADDRESS_OLED, &i2cConfig.oledStatus);
    }
}

bool isI2CDeviceAvailable(uint8_t address) {
    if (address == I2C_ADDRESS_RTC) {
        return i2cConfig.rtcStatus.connected;
    } else if (address == I2C_ADDRESS_OLED) {
        return i2cConfig.oledStatus.connected;
    }
    
    // 对于未知设备，直接检查
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    return error == 0;
}

bool writeI2CRegister(uint8_t address, uint8_t reg, uint8_t value) {
    if (!isI2CDeviceAvailable(address)) {
        LOG_ERROR("I2C device 0x%02X not available for write", address);
        return false;
    }
    
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(value);
    byte error = Wire.endTransmission();
    
    if (error != 0) {
        I2CErrorCode i2cError = getI2CError(error);
        LOG_ERROR("Failed to write I2C register 0x%02X on device 0x%02X: %s", 
                 reg, address, getI2CErrorString(i2cError));
        
        // 根据错误类型进行处理
        if (i2cError == I2C_ERROR_ADDRESS_NACK || i2cError == I2C_ERROR_BUS_BUSY) {
            // 如果是地址错误或总线忙，尝试恢复设备
            recoverI2CDevice(address);
        }
        
        return false;
    }
    
    return true;
}

bool readI2CRegister(uint8_t address, uint8_t reg, uint8_t* value) {
    if (!isI2CDeviceAvailable(address)) {
        LOG_ERROR("I2C device 0x%02X not available for read", address);
        return false;
    }
    
    Wire.beginTransmission(address);
    Wire.write(reg);
    byte error = Wire.endTransmission();
    
    if (error != 0) {
        I2CErrorCode i2cError = getI2CError(error);
        LOG_ERROR("Failed to set I2C register 0x%02X on device 0x%02X: %s", 
                 reg, address, getI2CErrorString(i2cError));
        
        // 根据错误类型进行处理
        if (i2cError == I2C_ERROR_ADDRESS_NACK || i2cError == I2C_ERROR_BUS_BUSY) {
            // 如果是地址错误或总线忙，尝试恢复设备
            recoverI2CDevice(address);
        }
        
        return false;
    }
    
    Wire.requestFrom(address, (uint8_t)1);
    if (Wire.available()) {
        *value = Wire.read();
        return true;
    }
    
    LOG_ERROR("No data available from I2C device 0x%02X", address);
    
    // 如果没有数据可用，也尝试恢复设备
    recoverI2CDevice(address);
    
    return false;
}