/**
 * @file eeprom_config_test.cpp
 * @brief EEPROM配置测试程序
 *
 * 用于测试EEPROM配置管理功能
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 2.0
 * @date 2025-02-03
 */

#include "eeprom_config.h"
#include "logger.h"

/**
 * @brief 测试EEPROM功能
 * @note 此函数可用于调试和验证EEPROM功能
 */
void testEEPROMFunction() {
    LOG_DEBUG("=== Testing EEPROM Function ===");

    // 测试1: 清除EEPROM
    LOG_DEBUG("Test 1: Clear EEPROM");
    clearEEPROM();

    // 测试2: 加载默认值（应该返回2）
    LOG_DEBUG("Test 2: Load default brightness index");
    uint8_t brightness = loadBrightnessIndex();
    LOG_DEBUG("Loaded brightness index (should be 2): %d", brightness);

    // 测试3: 保存不同的亮度值
    LOG_DEBUG("Test 3: Save brightness index 0");
    if (saveBrightnessIndex(0)) {
        LOG_DEBUG("Saved brightness index 0 successfully");
    } else {
        LOG_WARNING("Failed to save brightness index 0");
    }

    // 测试4: 加载保存的值
    LOG_DEBUG("Test 4: Load saved brightness index");
    brightness = loadBrightnessIndex();
    LOG_DEBUG("Loaded brightness index (should be 0): %d", brightness);

    // 测试5: 保存最大亮度值
    LOG_DEBUG("Test 5: Save brightness index 3");
    if (saveBrightnessIndex(3)) {
        LOG_DEBUG("Saved brightness index 3 successfully");
    } else {
        LOG_WARNING("Failed to save brightness index 3");
    }

    // 测试6: 加载保存的值
    LOG_DEBUG("Test 6: Load saved brightness index");
    brightness = loadBrightnessIndex();
    LOG_DEBUG("Loaded brightness index (should be 3): %d", brightness);

    // 测试7: 尝试保存无效值（应该失败）
    LOG_DEBUG("Test 7: Try to save invalid brightness index 5");
    if (saveBrightnessIndex(5)) {
        LOG_WARNING("Saved invalid brightness index 5 (should have failed)");
    } else {
        LOG_DEBUG("Correctly rejected invalid brightness index 5");
    }

    // 测试8: 验证之前的值仍然有效
    LOG_DEBUG("Test 8: Verify previous brightness index is still valid");
    brightness = loadBrightnessIndex();
    LOG_DEBUG("Loaded brightness index (should be 3): %d", brightness);

    // 测试9: 重启模拟（清除并重新加载）
    LOG_DEBUG("Test 9: Simulate restart (clear and reload)");
    clearEEPROM();
    brightness = loadBrightnessIndex();
    LOG_DEBUG("Loaded brightness index after clear (should be 2): %d", brightness);

    LOG_DEBUG("=== EEPROM Test Complete ===");
}
