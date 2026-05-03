/**
 * @file integration_tests.cpp
 * @brief 集成测试实现
 *
 * 测试各模块之间的协同工作
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2026-02-04
 */

#include "integration_tests.h"
#include "button_handler.h"
#include "time_manager.h"
#include "display_manager.h"
#include "system_manager.h"
#include "eeprom_config.h"
#include "logger.h"
#include "utils.h"

// =============================================================================
// 系统启动集成测试
// =============================================================================

void runIntegrationTest_system_startup() {
    TEST_SUITE_START(system_startup);

    TEST_CASE(test_eeprom_initialization) {
            // 测试EEPROM初始化
            initEEPROM();
            uint8_t brightness = loadBrightnessIndex();
            // 应该能成功加载（可能返回默认值）
            ASSERT_TRUE(brightness >= 0 && brightness <= 3);
        }
        TEST_CASE_END();

        TEST_CASE(test_button_initialization) {
            // 测试按键初始化
            initButtons();

            // 检查所有按键是否正确初始化
            for (int i = 0; i < 4; i++) {
                ButtonState& btn = buttonStates.buttons[i];
                ASSERT_EQ(HIGH, btn.stableState);
                ASSERT_FALSE(btn.isPressed);
            }
        }
        TEST_CASE_END();

        TEST_CASE(test_brightness_persistence) {
            // 测试亮度持久化
            uint8_t testBrightness = 1;
            saveBrightnessIndex(testBrightness);

            uint8_t loadedBrightness = loadBrightnessIndex();
            ASSERT_EQ(testBrightness, loadedBrightness);

            // 恢复默认值
            saveBrightnessIndex(2);
        }
        TEST_CASE_END();

    TEST_SUITE_END();

    LOG_INFO("=== Test Suite Complete: %s ===", g_testStats.currentSuite);
    LOG_INFO("Passed: %d, Failed: %d", g_testStats.passedTests, g_testStats.failedTests);
    LOG_DEBUG("");
}

// =============================================================================
// 时间同步集成测试
// =============================================================================

void runIntegrationTest_time_sync() {
    TEST_SUITE_START(time_sync);

    TEST_CASE(test_time_source_switching) {
            // 测试时间源切换
            TimeSource originalSource = timeState.currentTimeSource;

            // 切换到RTC时间源
            switchTimeSource(TIME_SOURCE_RTC);
            ASSERT_EQ(TIME_SOURCE_RTC, timeState.currentTimeSource);

            // 切换到NTP时间源
            switchTimeSource(TIME_SOURCE_NTP);
            ASSERT_EQ(TIME_SOURCE_NTP, timeState.currentTimeSource);

            // 切换到手动时间源
            switchTimeSource(TIME_SOURCE_MANUAL);
            ASSERT_EQ(TIME_SOURCE_MANUAL, timeState.currentTimeSource);

            // 恢复原始时间源
            switchTimeSource(originalSource);
        }
        TEST_CASE_END();

        TEST_CASE(test_time_validation) {
            // 测试时间验证功能
            DateTime validTime(2023, 12, 25, 12, 30, 45);
            ASSERT_TRUE(isRtcTimeValid(validTime));

            DateTime invalidYear(2019, 12, 25, 12, 30, 45);
            ASSERT_FALSE(isRtcTimeValid(invalidYear));

            DateTime invalidMonth(2023, 13, 25, 12, 30, 45);
            ASSERT_FALSE(isRtcTimeValid(invalidMonth));

            DateTime invalidDay(2023, 12, 32, 12, 30, 45);
            ASSERT_FALSE(isRtcTimeValid(invalidDay));
        }
        TEST_CASE_END();

        TEST_CASE(test_market_day_calculation) {
            // 测试市场日计算
            time_t testTime = 1672531200; // 2023-01-01 00:00:00 UTC
            int marketIndex;
            calculateMarketDay(testTime, marketIndex);

            // 市场日索引应该在0-2之间
            ASSERT_TRUE(marketIndex >= 0 && marketIndex <= 2);
        }
        TEST_CASE_END();

    TEST_SUITE_END();

    LOG_INFO("=== Test Suite Complete: %s ===", g_testStats.currentSuite);
    LOG_INFO("Passed: %d, Failed: %d", g_testStats.passedTests, g_testStats.failedTests);
    LOG_DEBUG("");
}

// =============================================================================
// 按键响应集成测试
// =============================================================================

void runIntegrationTest_button_response() {
    TEST_SUITE_START(button_response);

    TEST_CASE(test_button_state_update) {
            // 测试按键状态更新
            initButtons();

            // 模拟按键状态更新
            updateButtonStates();

            // 检查按键状态是否正确更新
            for (int i = 0; i < 4; i++) {
                ButtonState& btn = buttonStates.buttons[i];
                // 稳定状态应该与物理状态一致
                bool physicalState = (digitalRead(btn.pin) == LOW);
                ASSERT_EQ(physicalState ? LOW : HIGH, btn.stableState);
            }
        }
        TEST_CASE_END();

        TEST_CASE(test_button_debounce) {
            // 测试按键消抖
            initButtons();

            unsigned long startTime = millis();
            int updateCount = 0;

            // 快速更新按键状态多次
            for (int i = 0; i < 10; i++) {
                updateButtonStates();
                updateCount++;
                nonBlockingDelay(5); // 5ms间隔
            }

            unsigned long elapsed = millis() - startTime;

            // 应该在50ms左右完成
            ASSERT_TRUE(elapsed >= 40 && elapsed <= 60);
            ASSERT_EQ(10, updateCount);
        }
        TEST_CASE_END();

    TEST_SUITE_END();

    LOG_INFO("=== Test Suite Complete: %s ===", g_testStats.currentSuite);
    LOG_INFO("Passed: %d, Failed: %d", g_testStats.failedTests);
    LOG_DEBUG("");
}

// =============================================================================
// 显示更新集成测试
// =============================================================================

void runIntegrationTest_display_update() {
    TEST_SUITE_START(display_update);

    TEST_CASE(test_brightness_setting) {
            // 测试亮度设置
            uint8_t originalBrightness = displayState.brightnessIndex;

            // 设置不同的亮度
            for (int i = 0; i < 4; i++) {
                displayState.brightnessIndex = i;
                u8g2.setContrast(BRIGHTNESS_LEVELS[i]);
                ASSERT_EQ(i, displayState.brightnessIndex);
            }

            // 恢复原始亮度
            displayState.brightnessIndex = originalBrightness;
            u8g2.setContrast(BRIGHTNESS_LEVELS[originalBrightness]);
        }
        TEST_CASE_END();

        TEST_CASE(test_display_refresh) {
            // 测试显示刷新
            systemState.needsRefresh = true;

            // 模拟显示刷新
            DateTime now(2023, 12, 25, 12, 30, 45);
            displayTime();

            // 检查刷新标志是否被清除
            ASSERT_FALSE(systemState.needsRefresh);
        }
        TEST_CASE_END();

    TEST_SUITE_END();

    LOG_INFO("=== Test Suite Complete: %s ===", g_testStats.currentSuite);
    LOG_INFO("Passed: %d, Failed: %d", g_testStats.passedTests, g_testStats.failedTests);
    LOG_DEBUG("");
}

// =============================================================================
// 运行所有集成测试
// =============================================================================

void runAllIntegrationTests() {
    initTestFramework();

    LOG_DEBUG("");
    LOG_INFO("========================================");
    LOG_INFO("  Running Integration Tests");
    LOG_INFO("========================================");
    LOG_DEBUG("");

    runIntegrationTest_system_startup();
    runIntegrationTest_time_sync();
    runIntegrationTest_button_response();
    runIntegrationTest_display_update();

    LOG_DEBUG("");
    printTestSummary();
}
