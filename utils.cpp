/**
 * @file utils.cpp
 * @brief 实用工具函数实现
 * 
 * 包含各种实用工具函数的实现，如非阻塞延时等
 * 
 * @author ESP8266 SSD1306 Clock Project
 * @version 2.0
 * @date 2025-12-23
 */

#include "utils.h"

/**
 * @brief 非阻塞延时函数
 *
 * 在延时期间执行其他任务，如喂狗和让出控制权
 *
 * @param delayMs 延时时间（毫秒）
 */
void nonBlockingDelay(unsigned long delayMs) {
    unsigned long startTime = millis();
    while (true) {
        unsigned long currentMillis = millis();
        unsigned long elapsed = (currentMillis >= startTime) ?
                              (currentMillis - startTime) :
                              (0xFFFFFFFF - startTime + currentMillis);
        if (elapsed >= delayMs) break;

        yield(); // 让出控制权
        ESP.wdtFeed(); // 喂看门狗
        // 可以在这里添加其他需要执行的任务
    }
}