/**
 * @file test_framework.h
 * @brief 轻量级单元测试框架
 *
 * 为ESP8266项目提供简单的单元测试功能
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2026-02-04
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <Arduino.h>
#include "logger.h"

// 测试统计结构
typedef struct {
    int totalTests;
    int passedTests;
    int failedTests;
    const char* currentSuite;
} TestStats;

// 全局测试统计
extern TestStats g_testStats;

// 测试套件开始宏
#define TEST_SUITE_START(name) \
    do { \
        g_testStats.currentSuite = #name; \
        LOG_INFO("=== Running Test Suite: %s ===", #name); \
        LOG_DEBUG(""); \
    } while(0)

#define TEST_SUITE_END()

// 测试用例宏
#define TEST_CASE(name) \
    do { \
        g_testStats.totalTests++; \
        LOG_DEBUG("  [TEST] %s", #name); \
        bool testPassed = true; \
        do {

#define TEST_CASE_END() \
            if (testPassed) { \
                g_testStats.passedTests++; \
                LOG_DEBUG("    ✓ PASSED"); \
            } else { \
                g_testStats.failedTests++; \
                LOG_ERROR("    ✗ FAILED"); \
            } \
        } while(0); \
    } while(0)

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            testPassed = false; \
            LOG_ERROR("    Assertion failed: %s", #condition); \
            LOG_ERROR("    Expected: true, Got: false"); \
            LOG_ERROR("    File: %s, Line: %d", __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            testPassed = false; \
            LOG_ERROR("    Assertion failed: !%s", #condition); \
            LOG_ERROR("    Expected: false, Got: true"); \
            LOG_ERROR("    File: %s, Line: %d", __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            testPassed = false; \
            LOG_ERROR("    Assertion failed: expected %d, got %d", (int)(expected), (int)(actual)); \
            LOG_ERROR("    File: %s, Line: %d", __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_NE(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            testPassed = false; \
            LOG_WARNING("    Assertion failed: expected %d to not equal %d", (int)(expected), (int)(actual)); \
            LOG_WARNING("    File: %s, Line: %d", __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_LT(a, b) \
    do { \
        if (!((a) < (b))) { \
            testPassed = false; \
            LOG_WARNING("    Assertion failed: %d < %d", (int)(a), (int)(b)); \
            LOG_WARNING("    File: %s, Line: %d", __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_GT(a, b) \
    do { \
        if (!((a) > (b))) { \
            testPassed = false; \
            LOG_WARNING("    Assertion failed: %d > %d", (int)(a), (int)(b)); \
            LOG_WARNING("    File: %s, Line: %d", __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_STR_EQ(expected, actual) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            testPassed = false; \
            LOG_ERROR("    Assertion failed: expected '%s', got '%s'", (expected), (actual)); \
            LOG_ERROR("    File: %s, Line: %d", __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != nullptr) { \
            testPassed = false; \
            LOG_WARNING("    Assertion failed: expected nullptr, got %p", (ptr)); \
            LOG_WARNING("    File: %s, Line: %d", __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == nullptr) { \
            testPassed = false; \
            LOG_WARNING("    Assertion failed: expected non-null, got nullptr"); \
            LOG_WARNING("    File: %s, Line: %d", __FILE__, __LINE__); \
        } \
    } while(0)

// 测试框架函数声明
void initTestFramework();
void runAllTests();
void printTestSummary();

#endif // TEST_FRAMEWORK_H
