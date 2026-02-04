/**
 * @file ota_manager.cpp
 * @brief OTA升级管理模块实现
 *
 * @author ESP8266 SSD1306 Clock Project
 * @version 1.0
 * @date 2026-02-04
 */

#include "ota_manager.h"
#include "global_config.h"
#include "utils.h"
#include <ESP8266HTTPClient.h>

// 全局OTA配置
OtaConfig otaConfig = {
    "",                            // updateServerUrl
    "1.0.0",                       // currentVersion
    "",                            // latestVersion
    false,                         // autoUpdateEnabled
    0,                             // lastCheckTime
    86400000                       // checkInterval (24小时)
};

// 全局OTA状态
OtaState otaState = {
    OTA_STATUS_IDLE,               // status
    0,                             // progress
    "",                            // error
    0,                             // startTime
    0                              // endTime
};

/**
 * @brief OTA进度回调函数
 */
void otaProgressCallback(int progress, int total) {
    otaState.progress = (progress * 100) / total;

    if (otaState.progress % 10 == 0) {
        LOG_DEBUG("OTA Update Progress: %d%%", otaState.progress);
    }
}

/**
 * @brief 初始化OTA管理器
 */
void initOtaManager() {
    otaState.status = OTA_STATUS_IDLE;
    otaState.progress = 0;
    otaState.error[0] = '\0';

    setOtaProgressCallback();

    LOG_INFO("OTA Manager initialized");
    LOG_INFO("Current version: %s", otaConfig.currentVersion);
}

/**
 * @brief 设置OTA进度回调
 */
void setOtaProgressCallback() {
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
    ESPhttpUpdate.onStart([]() {
        otaState.status = OTA_STATUS_UPDATING;
        otaState.startTime = millis();
        otaState.progress = 0;
        LOG_INFO("OTA Update started");
    });

    ESPhttpUpdate.onEnd([]() {
        otaState.status = OTA_STATUS_SUCCESS;
        otaState.endTime = millis();
        otaState.progress = 100;
        LOG_INFO("OTA Update successful");
    });

    ESPhttpUpdate.onProgress(otaProgressCallback);

    ESPhttpUpdate.onError([](int error) {
        otaState.status = OTA_STATUS_FAILED;
        otaState.endTime = millis();

        switch (error) {
            case HTTP_UPDATE_FAILED:
                snprintf(otaState.error, sizeof(otaState.error),
                        "HTTP Update Failed: %d", ESPhttpUpdate.getLastError());
                break;
            case HTTP_UPDATE_NO_UPDATES:
                snprintf(otaState.error, sizeof(otaState.error), "No updates available");
                break;
            default:
                snprintf(otaState.error, sizeof(otaState.error),
                        "Unknown error: %d", error);
                break;
        }

        LOG_WARNING("OTA Update failed: %s", otaState.error);
    });
}

/**
 * @brief 检查是否有更新
 * @return true 有更新，false 无更新
 */
bool checkForUpdates() {
    if (otaState.status != OTA_STATUS_IDLE) {
        LOG_DEBUG("OTA busy, skipping check");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        LOG_DEBUG("WiFi not connected, skipping OTA check");
        return false;
    }

    unsigned long currentMillis = millis();
    unsigned long elapsed = (currentMillis >= otaConfig.lastCheckTime) ?
                          (currentMillis - otaConfig.lastCheckTime) :
                          (0xFFFFFFFF - otaConfig.lastCheckTime + currentMillis);

    if (elapsed < otaConfig.checkInterval) {
        LOG_DEBUG("OTA check interval not reached");
        return false;
    }

    otaState.status = OTA_STATUS_CHECKING;
    otaConfig.lastCheckTime = currentMillis;

    LOG_INFO("Checking for updates...");

    // TODO: 实现版本检查逻辑
    // 这里应该从服务器获取最新版本信息
    // 目前模拟检查
    otaState.status = OTA_STATUS_IDLE;

    // 模拟：假设没有更新
    LOG_INFO("No updates available");

    return false;
}

/**
 * @brief 开始OTA升级
 * @param firmwareUrl 固件URL
 * @return true 成功开始升级，false 失败
 */
bool startOtaUpdate(const char* firmwareUrl) {
    if (otaState.status != OTA_STATUS_IDLE) {
        LOG_WARNING("OTA busy, cannot start update");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        LOG_WARNING("WiFi not connected, cannot start OTA update");
        return false;
    }

    if (firmwareUrl == nullptr || strlen(firmwareUrl) == 0) {
        LOG_WARNING("Invalid firmware URL");
        return false;
    }

    otaState.status = OTA_STATUS_DOWNLOADING;
    LOG_INFO("Starting OTA update from: %s", firmwareUrl);

    WiFiClient client;

    // 开始OTA更新
    t_httpUpdate_return ret = ESPhttpUpdate.update(client, firmwareUrl);

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            otaState.status = OTA_STATUS_FAILED;
            LOG_WARNING("HTTP_UPDATE_FAILED Error (%d): %s",
                       ESPhttpUpdate.getLastError(),
                       ESPhttpUpdate.getLastErrorString().c_str());
            return false;

        case HTTP_UPDATE_NO_UPDATES:
            otaState.status = OTA_STATUS_IDLE;
            LOG_INFO("HTTP_UPDATE_NO_UPDATES");
            return false;

        case HTTP_UPDATE_OK:
            otaState.status = OTA_STATUS_SUCCESS;
            LOG_INFO("HTTP_UPDATE_OK");
            return true;

        default:
            otaState.status = OTA_STATUS_ERROR;
            LOG_WARNING("Unknown OTA update result: %d", ret);
            return false;
    }
}

/**
 * @brief 处理OTA更新（在主循环中调用）
 */
void handleOtaUpdate() {
    // 如果启用了自动更新，定期检查
    if (otaConfig.autoUpdateEnabled) {
        checkForUpdates();
    }
}

/**
 * @brief 重置OTA状态
 */
void resetOtaState() {
    otaState.status = OTA_STATUS_IDLE;
    otaState.progress = 0;
    otaState.error[0] = '\0';
    otaState.startTime = 0;
    otaState.endTime = 0;
}

/**
 * @brief 获取OTA状态字符串
 * @param status OTA状态
 * @return 状态字符串
 */
const char* getOtaStatusString(OtaStatus status) {
    switch (status) {
        case OTA_STATUS_IDLE: return "Idle";
        case OTA_STATUS_CHECKING: return "Checking";
        case OTA_STATUS_DOWNLOADING: return "Downloading";
        case OTA_STATUS_UPDATING: return "Updating";
        case OTA_STATUS_SUCCESS: return "Success";
        case OTA_STATUS_FAILED: return "Failed";
        case OTA_STATUS_ERROR: return "Error";
        default: return "Unknown";
    }
}

/**
 * @brief 设置OTA版本号
 * @param version 版本号字符串
 */
void setOtaVersion(const char* version) {
    if (version != nullptr && strlen(version) > 0) {
        strncpy(otaConfig.currentVersion, version, sizeof(otaConfig.currentVersion) - 1);
        otaConfig.currentVersion[sizeof(otaConfig.currentVersion) - 1] = '\0';
        LOG_INFO("OTA version set to: %s", otaConfig.currentVersion);
    }
}

/**
 * @brief 从GitHub获取最新版本信息
 * @return 最新版本号字符串，失败返回空字符串
 */
String getLatestVersionFromGitHub() {
    if (WiFi.status() != WL_CONNECTED) {
        LOG_DEBUG("WiFi not connected, cannot check version");
        return "";
    }

    WiFiClientSecure client;
    HTTPClient http;

    // GitHub API获取最新Release
    // 注意：请确保仓库存在或修改为正确的仓库地址
    String url = "https://api.github.com/repos/chakoe/esp8266_ssd1306_Clock/releases/latest";

    LOG_DEBUG("Fetching latest version from GitHub...");
    LOG_DEBUG("Repository: chakoe/esp8266_ssd1306_Clock");

    // 配置HTTPS客户端（不验证证书以节省资源）
    client.setInsecure();
    client.setTimeout(15000); // 15秒超时

    // 重试机制
    const int maxRetries = 3;
    for (int retry = 0; retry < maxRetries; retry++) {
        if (retry > 0) {
            LOG_DEBUG("Retry %d/%d...", retry + 1, maxRetries);
            delay(1000); // 重试前等待1秒
        }

        if (http.begin(client, url)) {
            http.setUserAgent("ESP8266-Clock");
            http.setReuse(true);
            http.setTimeout(15000); // 15秒超时

            // 添加必要的请求头
            http.addHeader("Accept", "application/vnd.github.v3+json");

            int httpCode = http.GET();

            if (httpCode == HTTP_CODE_OK) {
                String payload = http.getString();

                // 解析JSON获取tag_name
                int tagIndex = payload.indexOf("\"tag_name\":");
                if (tagIndex > 0) {
                    int startIndex = payload.indexOf("\"", tagIndex + 11) + 1;
                    int endIndex = payload.indexOf("\"", startIndex);

                    if (startIndex > 0 && endIndex > startIndex) {
                        String tagName = payload.substring(startIndex, endIndex);
                        http.end();
                        client.stop();

                        LOG_INFO("Latest version from GitHub: %s", tagName.c_str());
                        return tagName;
                    }
                }

                http.end();
                client.stop();
                LOG_WARNING("Failed to parse version from GitHub response");
            } else {
                http.end();
                client.stop();

                if (httpCode == HTTP_CODE_TOO_MANY_REQUESTS) {
                    LOG_WARNING("GitHub API rate limit exceeded");
                    LOG_WARNING("Please try again later");
                    return "";
                } else if (httpCode == HTTP_CODE_NOT_FOUND) {
                    LOG_WARNING("========================================");
                    LOG_WARNING("  GitHub Repository Not Found");
                    LOG_WARNING("========================================");
                    LOG_WARNING("Repository: chakoe/esp8266_ssd1306_Clock");
                    LOG_WARNING("");
                    LOG_WARNING("Possible reasons:");
                    LOG_WARNING("1. Repository does not exist");
                    LOG_WARNING("2. Repository name is incorrect");
                    LOG_WARNING("3. Repository is private");
                    LOG_WARNING("");
                    LOG_WARNING("Solutions:");
                    LOG_WARNING("1. Create the repository on GitHub");
                    LOG_WARNING("2. Update the repository name in code");
                    LOG_WARNING("3. Make the repository public");
                    LOG_WARNING("4. Create a Release with a tag");
                    LOG_WARNING("");
                    LOG_WARNING("For now, OTA update is disabled.");
                    LOG_WARNING("You can still use manual update with 'u' command.");
                    LOG_WARNING("========================================");
                    return "";
                } else {
                    LOG_WARNING("GitHub API request failed, code: %d", httpCode);
                    // 继续重试
                }
            }
        } else {
            LOG_WARNING("Failed to connect to GitHub API");
            // 继续重试
        }
    }

    LOG_WARNING("Failed to get latest version after %d retries", maxRetries);
    return "";
}

/**
 * @brief 构建固件下载URL
 * @param version 版本标签
 * @return 完整的固件URL
 */
String buildFirmwareUrl(const char* version) {
    String url = "https://github.com/chakoe/esp8266_ssd1306_Clock/releases/download/";
    url += version;
    url += "/esp8266_ssd1306_Clock.ino.bin";
    return url;
}

/**
 * @brief 比较版本号，判断是否需要更新
 * @param latestVersion 最新版本号
 * @param currentVersion 当前版本号
 * @return true 需要更新，false 不需要更新
 */
bool isNewerVersion(const char* latestVersion, const char* currentVersion) {
    if (latestVersion == nullptr || currentVersion == nullptr) {
        return false;
    }

    // 简单字符串比较（适用于vX.Y.Z格式）
    // 如果版本号格式不同，返回false
    if (strcmp(latestVersion, currentVersion) == 0) {
        return false; // 版本相同
    }

    // 简化处理：如果最新版本不等于当前版本，则认为需要更新
    // 生产环境建议使用更精确的版本比较算法
    return true;
}

/**
 * @brief 检查并更新到最新版本
 * @return true 开始更新，false 无更新或失败
 */
bool checkAndUpdateToLatest() {
    if (otaState.status != OTA_STATUS_IDLE) {
        LOG_DEBUG("OTA busy, skipping version check");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        LOG_DEBUG("WiFi not connected, skipping version check");
        return false;
    }

    LOG_INFO("========================================");
    LOG_INFO("  Checking for OTA Updates");
    LOG_INFO("========================================");

    // 获取最新版本
    String latestVersion = getLatestVersionFromGitHub();

    if (latestVersion.length() == 0) {
        LOG_WARNING("Failed to get latest version from GitHub");
        LOG_INFO("========================================");
        return false;
    }

    LOG_INFO("Current version: %s", otaConfig.currentVersion);
    LOG_INFO("Latest version: %s", latestVersion.c_str());

    // 比较版本号
    if (!isNewerVersion(latestVersion.c_str(), otaConfig.currentVersion)) {
        LOG_INFO("Already up to date");
        LOG_INFO("========================================");
        return false;
    }

    // 有新版本可用
    LOG_INFO("New version available!");

    // 构建固件URL
    String firmwareUrl = buildFirmwareUrl(latestVersion.c_str());

    LOG_INFO("Firmware URL: %s", firmwareUrl.c_str());
    LOG_INFO("Starting OTA update...");
    LOG_INFO("========================================");

    // 更新配置中的最新版本号
    strncpy(otaConfig.latestVersion, latestVersion.c_str(),
            sizeof(otaConfig.latestVersion) - 1);
    otaConfig.latestVersion[sizeof(otaConfig.latestVersion) - 1] = '\0';

    // 开始更新
    return startOtaUpdate(firmwareUrl.c_str());
}
