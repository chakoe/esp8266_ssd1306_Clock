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
#include "button_handler.h"

/**
 * @brief 非阻塞延时函数
 *
 * 在延时期间执行其他任务，如喂狗和让出控制权
 *
 * @param delayMs 延时时间（毫秒）
 */
void ICACHE_FLASH_ATTR nonBlockingDelay(unsigned long delayMs) {
    unsigned long startTime = millis();
    // 递归保护：按键事件可能再次调用nonBlockingDelay（如resetToAP），
    // 嵌套调用中只执行yield和喂狗，不再轮询按键，避免栈溢出
    static bool nested = false;
    bool shouldPollButtons = !nested;
    nested = true;
    while (true) {
        unsigned long currentMillis = millis();
        unsigned long elapsed = (currentMillis >= startTime) ?
                              (currentMillis - startTime) :
                              (0xFFFFFFFF - startTime + currentMillis);
        if (elapsed >= delayMs) break;

        yield(); // 让出控制权
        ESP.wdtFeed(); // 喂看门狗
        if (shouldPollButtons) {
            updateButtonStates(); // 轮询按键状态，避免延时期间按键事件丢失
        }
    }
    nested = false;
}