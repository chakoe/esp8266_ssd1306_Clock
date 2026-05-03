/**
 * @file test_suites.h
 * @brief 单元测试套件声明
 *
 * 声明所有测试套件
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2026-02-04
 */

#ifndef TEST_SUITES_H
#define TEST_SUITES_H

#include "test_framework.h"

// 测试套件函数声明
void runTestSuite_eeprom();
void runTestSuite_utils();
void runTestSuite_time();
void runTestSuite_encryption();

#endif // TEST_SUITES_H
