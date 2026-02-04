/**
 * @file test_main.ino
 * @brief 测试运行器主程序
 *
 * 用于运行单元测试和集成测试
 *
 * 注意：此文件应单独编译，不要与 esp8266_ssd1306_Clock.ino 同时编译
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2026-02-04
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include "test_framework.h"
#include "test_suites.h"
#include "integration_tests.h"
#include "logger.h"
#include "config.h"
#include "global_config.h"
#include "button_handler.h"
#include "time_manager.h"
#include "display_manager.h"
#include "system_manager.h"
#include "utils.h"
#include "eeprom_config.h"
#include "ota_manager.h"
#include "runtime_monitor.h"
#include "config_manager.h"
#include "error_recovery.h"
#include "power_manager.h"

// 测试模式选择
#define RUN_UNIT_TESTS true
#define RUN_INTEGRATION_TESTS false
#define RUN_TEST_ON_STARTUP true
#define RUN_EEPROM_TESTS_ONLY false
#define RUN_UTILS_TESTS_ONLY false
#define RUN_TIME_TESTS_ONLY false

/**
 * @brief 测试程序设置函数
 */
void setup() {
    // 初始化串口
    Serial.begin(115200);

    // 等待串口初始化
    unsigned long startTime = millis();
    while (!Serial) {
        if (millis() - startTime > 5000) break;
        yield();
    }

    LOG_INFO("========================================");
    LOG_INFO("  ESP8266 SSD1306 Clock Test Runner");
    LOG_INFO("========================================");
    LOG_INFO("");

    // 设置测试模式标志
    g_testMode = true;

    // 启用调试日志
    setLogLevel(LOG_LEVEL_DEBUG);
    enableLogger(true);
    enableTimestamp(true);

    // 初始化所有模块（测试需要）
    initEEPROM();
    initLogger();
    initPowerManagement();
    initRuntimeMonitor();
    initConfigManager();
    initErrorRecovery();

    // 初始化硬件
    Wire.begin();
    u8g2.begin();
    u8g2.setPowerSave(false);

    // 初始化按键
    initButtons();

    // 初始化RTC（测试模式，不显示错误）
    bool rtcResult = initializeRTC();
    if (!rtcResult) {
        LOG_WARNING("RTC initialization failed (expected in test mode)");
    }

    LOG_INFO("All modules initialized for testing");
    LOG_INFO("");

    // 如果启用启动测试，立即运行
    if (RUN_TEST_ON_STARTUP) {
        LOG_INFO("Running tests on startup...");
        Serial.flush();

        if (RUN_EEPROM_TESTS_ONLY) {
            LOG_INFO("Running EEPROM tests only...");
            Serial.flush();
            runTestSuite_eeprom();
            Serial.flush();
        } else if (RUN_UTILS_TESTS_ONLY) {
            LOG_INFO("Running Utils tests only...");
            Serial.flush();
            runTestSuite_utils();
            Serial.flush();
        } else if (RUN_TIME_TESTS_ONLY) {
            LOG_INFO("Running Time tests only...");
            Serial.flush();
            runTestSuite_time();
            Serial.flush();
        } else {
            runAllTests();
            Serial.flush();
        }

        if (RUN_INTEGRATION_TESTS) {
            LOG_INFO("Running integration tests...");
            Serial.flush();
            runAllIntegrationTests();
            Serial.flush();
        }
    } else {
        LOG_INFO("Test mode: Manual");
        LOG_INFO("Current log level: %d", logConfig.currentLevel);
        LOG_INFO("Send 'u' to run unit tests");
        LOG_INFO("Send 'i' to run integration tests");
        LOG_INFO("Send 'a' to run all tests");
        LOG_INFO("Send 's' to show system stats");
        LOG_INFO("Send 'r' to show runtime stats");
        LOG_INFO("Send 'c' to show config");
        LOG_INFO("Send 'e' to show error recovery stats");
        LOG_INFO("");
    }
}

/**
 * @brief 测试程序主循环
 */
void loop() {
    if (RUN_TEST_ON_STARTUP) {
        // 测试模式：只运行一次
        while (true) {
            yield();
        }
    }

    // 手动测试模式：检查串口输入
    if (Serial.available() > 0) {
        char command = Serial.read();

        switch (command) {
            case 'u':
            case 'U':
                LOG_INFO("Running unit tests...");
                runAllTests();
                break;

            case 'i':
            case 'I':
                LOG_INFO("Running integration tests...");
                runAllIntegrationTests();
                break;

            case 'a':
            case 'A':
                LOG_INFO("Running all tests...");
                runAllTests();
                runAllIntegrationTests();
                break;

            case 's':
            case 'S':
                LOG_INFO("System Statistics:");
                LOG_INFO("  Free heap: %lu bytes", ESP.getFreeHeap());
                LOG_INFO("  Max free heap: %lu bytes", ESP.getMaxFreeBlockSize());
                LOG_INFO("  Heap fragmentation: %d%%", ESP.getHeapFragmentation());
                LOG_INFO("  Flash size: %lu bytes", ESP.getFlashChipSize());
                LOG_INFO("  CPU freq: %d MHz", ESP.getCpuFreqMHz());
                break;

            case 'r':
            case 'R':
                LOG_INFO("Runtime Statistics:");
                printRuntimeStats();
                break;

            case 'c':
            case 'C':
                LOG_INFO("Configuration:");
                printAllConfigs();
                break;

            case 'e':
            case 'E':
                LOG_INFO("Error Recovery Statistics:");
                printErrorRecoveryStats();
                break;

            case 'h':
            case 'H':
            case '?':
                LOG_INFO("Available commands:");
                LOG_INFO("  u - Run unit tests");
                LOG_INFO("  i - Run integration tests");
                LOG_INFO("  a - Run all tests");
                LOG_INFO("  s - Show system stats");
                LOG_INFO("  r - Show runtime stats");
                LOG_INFO("  c - Show configuration");
                LOG_INFO("  e - Show error recovery stats");
                LOG_INFO("  h - Show help");
                break;

            default:
                LOG_DEBUG("Unknown command: %c (send 'h' for help)", command);
                break;
        }

        LOG_INFO("");
        LOG_INFO("Test mode: Manual");
        LOG_INFO("Send 'h' for help");
        LOG_INFO("");
    }

    yield();
}
