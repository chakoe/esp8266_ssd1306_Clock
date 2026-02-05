/**
 * @file integration_tests.h
 * @brief 集成测试声明
 *
 * 声明所有集成测试
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2026-02-04
 */

#ifndef INTEGRATION_TESTS_H
#define INTEGRATION_TESTS_H

#include "test_framework.h"

// 集成测试函数声明
void runIntegrationTest_system_startup();
void runIntegrationTest_time_sync();
void runIntegrationTest_button_response();
void runIntegrationTest_display_update();
void runAllIntegrationTests();

#endif // INTEGRATION_TESTS_H
