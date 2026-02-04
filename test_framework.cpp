/**
 * @file test_framework.cpp
 * @brief 轻量级单元测试框架实现
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2026-02-04
 */

#include "test_framework.h"
#include "test_suites.h"
#include "logger.h"

// 全局测试统计
TestStats g_testStats = {
    0,      // totalTests
    0,      // passedTests
    0,      // failedTests
    ""      // currentSuite
};

/**
 * @brief 初始化测试框架
 */
void initTestFramework() {
    g_testStats.totalTests = 0;
    g_testStats.passedTests = 0;
    g_testStats.failedTests = 0;
    g_testStats.currentSuite = "";

    LOG_INFO("========================================");
    LOG_INFO("  Unit Test Framework Initialized");
    LOG_INFO("========================================");
    LOG_DEBUG("");
}

/**
 * @brief 打印测试总结
 */
void printTestSummary() {
    LOG_INFO("========================================");
    LOG_INFO("  Test Summary");
    LOG_INFO("========================================");
    LOG_INFO("Total Tests:  %d", g_testStats.totalTests);
    LOG_INFO("Passed:       %d", g_testStats.passedTests);
    LOG_INFO("Failed:       %d", g_testStats.failedTests);

    if (g_testStats.failedTests == 0) {
        LOG_INFO("Result:       ✓ ALL TESTS PASSED");
    } else {
        float passRate = (float)g_testStats.passedTests / g_testStats.totalTests * 100.0f;
        LOG_INFO("Pass Rate:    %.1f%%", passRate);
        LOG_INFO("Result:       ✗ SOME TESTS FAILED");
        LOG_ERROR("Check the logs above for detailed failure information");
    }
    LOG_INFO("========================================");
}

/**
 * @brief 运行所有测试套件
 */
void runAllTests() {
    initTestFramework();

    LOG_DEBUG("");
    Serial.flush();

    // 测试日志系统
    LOG_ERROR("=== Testing Log System ===");
    Serial.flush();
    LOG_ERROR("This is an ERROR message (should always appear)");
    Serial.flush();
    LOG_WARNING("This is a WARNING message (should appear)");
    Serial.flush();
    LOG_INFO("This is an INFO message (should appear)");
    Serial.flush();
    LOG_DEBUG("This is a DEBUG message (should appear)");
    Serial.flush();
    LOG_ERROR("=== Log System Test Complete ===");
    Serial.flush();

    // 运行所有测试套件
    LOG_INFO("Running EEPROM test suite...");
    Serial.flush();
    runTestSuite_eeprom();
    Serial.flush();
    LOG_DEBUG("");
    Serial.flush();

    // 打印中间结果
    LOG_ERROR("=== Intermediate Results ===");
    LOG_ERROR("Total: %d, Passed: %d, Failed: %d", g_testStats.totalTests, g_testStats.passedTests, g_testStats.failedTests);
    Serial.flush();

    LOG_INFO("Running Utils test suite...");
    Serial.flush();
    runTestSuite_utils();
    Serial.flush();
    LOG_DEBUG("");
    Serial.flush();

    // 打印中间结果
    LOG_ERROR("=== Intermediate Results ===");
    LOG_ERROR("Total: %d, Passed: %d, Failed: %d", g_testStats.totalTests, g_testStats.passedTests, g_testStats.failedTests);
    Serial.flush();

    LOG_INFO("Running Time test suite...");
    Serial.flush();
    runTestSuite_time();
    Serial.flush();
    LOG_DEBUG("");
    Serial.flush();

    // 打印中间结果
    LOG_ERROR("=== Intermediate Results ===");
    LOG_ERROR("Total: %d, Passed: %d, Failed: %d", g_testStats.totalTests, g_testStats.passedTests, g_testStats.failedTests);
    Serial.flush();

    // runTestSuite_encryption(); // 加密测试套件暂未实现，暂时注释

    LOG_DEBUG("");
    Serial.flush();
    printTestSummary();
}
