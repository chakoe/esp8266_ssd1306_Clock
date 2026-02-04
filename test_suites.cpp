/**
 * @file test_suites.cpp
 * @brief 单元测试套件实现
 *
 * 实现所有测试套件
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2026-02-04
 */

#include "test_suites.h"
#include "eeprom_config.h"
#include "utils.h"
#include "system_manager.h"
#include "time_manager.h"
#include "logger.h"

// =============================================================================
// EEPROM测试套件
// =============================================================================

void runTestSuite_eeprom() {
    TEST_SUITE_START(eeprom);

    TEST_CASE(test_clear_eeprom) {
            clearEEPROM();
            uint8_t brightness = loadBrightnessIndex();
            LOG_ERROR("    Clear EEPROM test: brightness = %d (expected: 2)", brightness);
            ASSERT_EQ(2, brightness); // 默认值应该是2
        }
        TEST_CASE_END();

        TEST_CASE(test_save_and_load_brightness_0) {
            clearEEPROM();
            bool result = saveBrightnessIndex(0);
            LOG_ERROR("    Save brightness 0: result = %d (expected: 1)", result);
            ASSERT_TRUE(result);

            uint8_t brightness = loadBrightnessIndex();
            LOG_ERROR("    Load brightness: %d (expected: 0)", brightness);
            ASSERT_EQ(0, brightness);
        }
        TEST_CASE_END();

        TEST_CASE(test_save_and_load_brightness_3) {
            clearEEPROM();
            bool result = saveBrightnessIndex(3);
            LOG_ERROR("    Save brightness 3: result = %d (expected: 1)", result);
            ASSERT_TRUE(result);

            uint8_t brightness = loadBrightnessIndex();
            LOG_ERROR("    Load brightness: %d (expected: 3)", brightness);
            ASSERT_EQ(3, brightness);
        }
        TEST_CASE_END();

        TEST_CASE(test_save_invalid_brightness) {
            clearEEPROM();
            bool result = saveBrightnessIndex(5); // 无效值
            LOG_ERROR("    Save invalid brightness 5: result = %d (expected: 0)", result);
            ASSERT_FALSE(result);

            uint8_t brightness = loadBrightnessIndex();
            LOG_ERROR("    Load brightness: %d (expected: 2)", brightness);
            ASSERT_EQ(2, brightness); // 应该保持默认值
        }
        TEST_CASE_END();

        TEST_CASE(test_save_negative_brightness) {
            clearEEPROM();
            bool result = saveBrightnessIndex(255); // 负数（作为无符号处理）
            LOG_ERROR("    Save negative brightness 255: result = %d (expected: 0)", result);
            ASSERT_FALSE(result);
        }
        TEST_CASE_END();

        TEST_CASE(test_multiple_saves) {
            clearEEPROM();
            saveBrightnessIndex(1);
            uint8_t b1 = loadBrightnessIndex();
            LOG_ERROR("    Save/Load brightness 1: %d (expected: 1)", b1);
            ASSERT_EQ(1, b1);

            saveBrightnessIndex(2);
            uint8_t b2 = loadBrightnessIndex();
            LOG_ERROR("    Save/Load brightness 2: %d (expected: 2)", b2);
            ASSERT_EQ(2, b2);

            saveBrightnessIndex(0);
            uint8_t b3 = loadBrightnessIndex();
            LOG_ERROR("    Save/Load brightness 0: %d (expected: 0)", b3);
            ASSERT_EQ(0, b3);
        }
        TEST_CASE_END();

    TEST_SUITE_END();

    LOG_DEBUG("=== Test Suite Complete: %s ===", g_testStats.currentSuite);
    LOG_DEBUG("");
}

// =============================================================================
// 工具函数测试套件
// =============================================================================

void runTestSuite_utils() {
    TEST_SUITE_START(utils);

    TEST_CASE(test_non_blocking_delay_short) {
            unsigned long start = millis();
            nonBlockingDelay(100);
            unsigned long elapsed = millis() - start;

            LOG_ERROR("    Short delay test: elapsed = %lu ms (expected: 80-120 ms)", elapsed);
            // 允许20ms的误差（更加宽松）
            ASSERT_TRUE(elapsed >= 80 && elapsed <= 120);
        }
        TEST_CASE_END();

        TEST_CASE(test_non_blocking_delay_medium) {
            unsigned long start = millis();
            nonBlockingDelay(500);
            unsigned long elapsed = millis() - start;

            LOG_ERROR("    Medium delay test: elapsed = %lu ms (expected: 460-540 ms)", elapsed);
            // 允许40ms的误差（更加宽松）
            ASSERT_TRUE(elapsed >= 460 && elapsed <= 540);
        }
        TEST_CASE_END();

    TEST_SUITE_END();

    LOG_DEBUG("=== Test Suite Complete: %s ===", g_testStats.currentSuite);
    LOG_DEBUG("");
}

// =============================================================================
// 时间管理测试套件
// =============================================================================

void runTestSuite_time() {
    TEST_SUITE_START(time);

    TEST_CASE(test_time_source_names) {
            const char* ntpName = getTimeSourceName(TIME_SOURCE_NTP);
            LOG_ERROR("    NTP name: %s (expected: NTP)", ntpName);
            ASSERT_STR_EQ("NTP", ntpName);

            const char* rtcName = getTimeSourceName(TIME_SOURCE_RTC);
            LOG_ERROR("    RTC name: %s (expected: RTC)", rtcName);
            ASSERT_STR_EQ("RTC", rtcName);

            const char* manualName = getTimeSourceName(TIME_SOURCE_MANUAL);
            LOG_ERROR("    Manual name: %s (expected: CLK)", manualName);
            ASSERT_STR_EQ("CLK", manualName);

            const char* noneName = getTimeSourceName(TIME_SOURCE_NONE);
            LOG_ERROR("    None name: %s (expected: NONE)", noneName);
            ASSERT_STR_EQ("NONE", noneName);
        }
        TEST_CASE_END();

        TEST_CASE(test_market_offset_calculation) {
            int offset = getCorrectOffset();
            LOG_ERROR("    getCorrectOffset() returned: %d (expected: 0-2)", offset);
            // 偏移量应该在0-2之间
            ASSERT_TRUE(offset >= 0 && offset <= 2);
        }
        TEST_CASE_END();

    TEST_SUITE_END();

    LOG_DEBUG("=== Test Suite Complete: %s ===", g_testStats.currentSuite);
    LOG_DEBUG("");
}

// =============================================================================
// 加密测试套件
// =============================================================================

void runTestSuite_encryption() {
    TEST_SUITE_START(encryption);

    TEST_CASE(test_xor_encrypt_decrypt) {
            String original = "TestPassword123";
            String encrypted = encryptPassword(original);
            String decrypted = decryptPassword(encrypted);

            ASSERT_STR_EQ(original.c_str(), decrypted.c_str());
        }
        TEST_CASE_END();

        TEST_CASE(test_xor_empty_password) {
            String original = "";
            String encrypted = encryptPassword(original);
            String decrypted = decryptPassword(encrypted);

            ASSERT_STR_EQ(original.c_str(), decrypted.c_str());
        }
        TEST_CASE_END();

        TEST_CASE(test_xor_special_chars) {
            String original = "P@ssw0rd!#$%";
            String encrypted = encryptPassword(original);
            String decrypted = decryptPassword(encrypted);

            ASSERT_STR_EQ(original.c_str(), decrypted.c_str());
        }
        TEST_CASE_END();

        TEST_CASE(test_aes_encrypt_decrypt) {
            String original = "TestPassword123";
            uint8_t aesKey[AES_KEY_SIZE];
            generateAESKey(aesKey);

            String encrypted = encryptPasswordAES(original, aesKey);
            String decrypted = decryptPasswordAES(encrypted, aesKey);

            ASSERT_STR_EQ(original.c_str(), decrypted.c_str());
        }
        TEST_CASE_END();

        TEST_CASE(test_aes_empty_password) {
            String original = "";
            uint8_t aesKey[AES_KEY_SIZE];
            generateAESKey(aesKey);

            String encrypted = encryptPasswordAES(original, aesKey);
            String decrypted = decryptPasswordAES(encrypted, aesKey);

            ASSERT_EQ(0, original.length());
            ASSERT_EQ(0, decrypted.length());
        }
        TEST_CASE_END();

        TEST_CASE(test_aes_wrong_password) {
            String original = "TestPassword123";
            uint8_t aesKey[AES_KEY_SIZE];
            generateAESKey(aesKey);

            String encrypted = encryptPasswordAES(original, aesKey);

            // 使用不同的密钥解密
            uint8_t wrongKey[AES_KEY_SIZE];
            generateAESKey(wrongKey);
            String decrypted = decryptPasswordAES(encrypted, wrongKey);

            // 解密应该失败（返回空字符串）
            ASSERT_TRUE(decrypted.length() == 0 || decrypted != original);
        }
        TEST_CASE_END();

    TEST_SUITE_END();

    LOG_DEBUG("=== Test Suite Complete: %s ===", g_testStats.currentSuite);
    LOG_DEBUG("");
}
