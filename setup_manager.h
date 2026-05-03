/**
 * @file setup_manager.h
 * @brief 系统初始化管理模块
 *
 * 负责管理系统的初始化流程，将setup()函数拆分为多个模块化的初始化函数
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2026-02-05
 */

#ifndef SETUP_MANAGER_H
#define SETUP_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "global_config.h"

// 初始化函数声明

/**
 * @brief 初始化基础系统
 * 设置串口、硬件看门狗、WiFi持久化配置等
 */
void initBasicSystem();

/**
 * @brief 初始化硬件外设
 * 初始化按键、Web OTA管理器、EEPROM、I2C总线、OLED显示器
 */
void initHardwarePeripherals();

/**
 * @brief 初始化RTC并显示启动画面
 * @return true RTC初始化成功，false RTC初始化失败
 */
bool initRTCAndBootScreen();

/**
 * @brief 检测K4按键长按，决定是否进入配网模式
 * @return true 检测到长按，需要进入配网模式；false 未检测到长按
 */
bool checkK4LongPress();

/**
 * @brief 连接WiFi并初始化NTP
 * @param enterAPMode 是否进入AP模式（配网模式）
 */
void connectWiFiAndInitNTP(bool enterAPMode);

/**
 * @brief 初始化系统状态变量
 * 设置时间戳、网络检查间隔等系统状态
 */
void initSystemState();

/**
 * @brief 完整的系统初始化流程
 * 调用所有初始化函数，替代原setup()函数的内容
 */
void systemSetup();

/**
 * @brief 测试密码加密功能
 * 在DEBUG模式下测试AES和XOR加密/解密功能
 */
void testPasswordEncryption();

#endif // SETUP_MANAGER_H
