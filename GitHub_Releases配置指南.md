# GitHub Releases 更新服务器配置指南

## 概述

本指南详细说明如何配置GitHub Releases作为ESP8266时钟的OTA更新服务器，以及如何处理固件地址变化的情况。

---

## 🎯 当前配置

### 固件URL格式
```
https://github.com/chakoe/esp8266_ssd1306_Clock/releases/download/v2.1.0/esp8266_ssd1306_Clock.ino.bin
```

### URL结构说明
```
https://github.com/[用户名]/[仓库名]/releases/download/[标签]/[文件名]
         └──────────┬─────────┘          └───┬───┘     └─┬─┘   └──┬──┘
                   │                         │         │       │
              GitHub基础URL            下载API     版本标签  文件名
```

---

## 📋 固件地址变化的场景

### 场景1：版本号变化（正常发布新版本）

**示例：**
- 旧版本：`v2.1.0`
- 新版本：`v2.2.0`

**处理方法：**
1. 在GitHub创建新的Release
2. 上传新的固件文件
3. 更新设备中的固件URL

### 场景2：文件名变化

**示例：**
- 旧文件名：`esp8266_ssd1306_Clock.ino.bin`
- 新文件名：`firmware_v2.2.0.bin`

**处理方法：**
1. 保持文件名一致（推荐）
2. 或更新设备中的固件URL

### 场景3：仓库迁移

**示例：**
- 旧仓库：`chakoe/esp8266_ssd1306_Clock`
- 新仓库：`newuser/esp8266_clock`

**处理方法：**
1. 更新所有设备的固件URL
2. 或使用重定向服务

### 场景4：使用自定义域名

**示例：**
- GitHub URL：`https://github.com/...`
- 自定义域名：`https://firmware.example.com/...`

**处理方法：**
1. 配置CDN或反向代理
2. 更新设备中的固件URL

---

## 🔧 解决方案

### 方案1：使用固定URL格式（推荐）

#### 1.1 统一文件命名规范

**推荐命名格式：**
```
esp8266_ssd1306_Clock.ino.bin
```

**始终使用相同的文件名**，只改变版本标签。

#### 1.2 版本标签规范

**推荐标签格式：**
```
vX.Y.Z
```

示例：
- `v2.1.0`
- `v2.2.0`
- `v3.0.0`

#### 1.3 动态版本检测

实现自动检测最新版本的功能：

```cpp
// 在 ota_manager.cpp 中添加

/**
 * @brief 从GitHub获取最新版本信息
 * @return 最新版本号字符串
 */
String getLatestVersionFromGitHub() {
    if (WiFi.status() != WL_CONNECTED) {
        return "";
    }

    WiFiClient client;
    HTTPClient http;

    // GitHub API获取最新Release
    String url = "https://api.github.com/repos/chakoe/esp8266_ssd1306_Clock/releases/latest";

    if (http.begin(client, url)) {
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();

            // 解析JSON获取tag_name
            // 简化版：查找"tag_name"字段
            int tagIndex = payload.indexOf("\"tag_name\":");
            if (tagIndex > 0) {
                int startIndex = payload.indexOf("\"", tagIndex + 11) + 1;
                int endIndex = payload.indexOf("\"", startIndex);

                if (startIndex > 0 && endIndex > startIndex) {
                    String tagName = payload.substring(startIndex, endIndex);
                    http.end();
                    return tagName;
                }
            }
        }

        http.end();
    }

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
 * @brief 检查并更新到最新版本
 * @return true 开始更新，false 无更新或失败
 */
bool checkAndUpdateToLatest() {
    LOG_INFO("Checking for latest version...");

    // 获取最新版本
    String latestVersion = getLatestVersionFromGitHub();

    if (latestVersion.length() == 0) {
        LOG_WARNING("Failed to get latest version");
        return false;
    }

    LOG_INFO("Latest version: %s", latestVersion.c_str());
    LOG_INFO("Current version: %s", otaConfig.currentVersion);

    // 比较版本号（简化版）
    if (latestVersion == String("v2.1.0")) {
        LOG_INFO("Already up to date");
        return false;
    }

    // 构建固件URL
    String firmwareUrl = buildFirmwareUrl(latestVersion.c_str());

    LOG_INFO("New version available: %s", latestVersion.c_str());
    LOG_INFO("Firmware URL: %s", firmwareUrl.c_str());

    // 开始更新
    return startOtaUpdate(firmwareUrl.c_str());
}
```

---

### 方案2：使用版本配置文件

#### 2.1 创建版本配置文件

在GitHub仓库中创建 `version.json` 文件：

```json
{
  "latest": "v2.2.0",
  "firmware_url": "https://github.com/chakoe/esp8266_ssd1306_Clock/releases/download/v2.2.0/esp8266_ssd1306_Clock.ino.bin",
  "release_notes": "修复了时间显示bug，添加了新功能",
  "min_version": "v2.0.0",
  "force_update": false
}
```

#### 2.2 实现版本检查

```cpp
/**
 * @brief 从配置文件获取版本信息
 */
bool getVersionConfig() {
    WiFiClient client;
    HTTPClient http;

    String url = "https://raw.githubusercontent.com/chakoe/esp8266_ssd1306_Clock/main/version.json";

    if (http.begin(client, url)) {
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();

            // 解析JSON（建议使用ArduinoJson库）
            // 这里简化处理
            int latestIndex = payload.indexOf("\"latest\":");
            if (latestIndex > 0) {
                // 提取latest版本
                // 提取firmware_url
                // ...
            }

            http.end();
            return true;
        }

        http.end();
    }

    return false;
}
```

---

### 方案3：使用CDN或镜像服务器

#### 3.1 配置CDN

使用jsDelivr CDN加速GitHub Releases下载：

```
原始URL:
https://github.com/chakoe/esp8266_ssd1306_Clock/releases/download/v2.1.0/esp8266_ssd1306_Clock.ino.bin

CDN URL:
https://cdn.jsdelivr.net/gh/chakoe/esp8266_ssd1306_Clock@v2.1.0/esp8266_ssd1306_Clock.ino.bin
```

#### 3.2 使用CDN的优势

- ✅ 下载速度更快
- ✅ 全球节点分布
- ✅ 自动缓存
- ✅ 免费使用

#### 3.3 配置CDN URL

```cpp
// 在setup()中配置CDN URL
strcpy(otaConfig.updateServerUrl,
       "https://cdn.jsdelivr.net/gh/chakoe/esp8266_ssd1306_Clock@v2.1.0/esp8266_ssd1306_Clock.ino.bin");
```

---

### 方案4：动态URL更新机制

#### 4.1 实现URL更新功能

```cpp
// 在主程序的串口命令处理中添加

// 'u' 命令：触发OTA更新
else if (command == 'u' || command == 'U') {
    if (otaState.status == OTA_STATUS_IDLE) {
        LOG_INFO("========================================");
        LOG_INFO("  Manual OTA Update Triggered");
        LOG_INFO("========================================");

        if (startOtaUpdate(otaConfig.updateServerUrl)) {
            LOG_INFO("OTA update started successfully");
            LOG_INFO("Please wait for the update to complete...");
            LOG_INFO("Device will restart automatically");
            LOG_INFO("========================================");
        } else {
            LOG_WARNING("OTA update failed: %s", otaState.error);
            LOG_INFO("========================================");
        }
    } else {
        LOG_WARNING("OTA is busy, current status: %s",
                   getOtaStatusString(otaState.status));
    }
}

// 'v' 命令：设置新的固件URL
else if (command == 'v' || command == 'V') {
    LOG_INFO("Enter new firmware URL (send when done):");
    LOG_INFO("Current URL: %s", otaConfig.updateServerUrl);
    LOG_INFO("Format: https://github.com/.../releases/download/.../filename.bin");
}

// 'c' 命令：检查最新版本
else if (command == 'c' || command == 'C') {
    LOG_INFO("Checking for latest version...");
    String latest = getLatestVersionFromGitHub();
    if (latest.length() > 0) {
        LOG_INFO("Latest version: %s", latest.c_str());
        LOG_INFO("Current version: %s", otaConfig.currentVersion);

        if (latest != String("v2.1.0")) {
            LOG_INFO("New version available!");
            LOG_INFO("Send 'u' to update");
        } else {
            LOG_INFO("Already up to date");
        }
    } else {
        LOG_WARNING("Failed to check version");
    }
}
```

#### 4.2 保存URL到EEPROM

```cpp
// 将固件URL保存到EEPROM，重启后保持

/**
 * @brief 保存固件URL到EEPROM
 */
void saveFirmwareUrlToEeprom(const char* url) {
    // 使用EEPROM存储URL
    // 地址：100-250（150字节）
    for (int i = 0; i < 150 && url[i] != '\0'; i++) {
        EEPROM.write(100 + i, url[i]);
    }
    EEPROM.write(100 + strlen(url), '\0');
    EEPROM.commit();
}

/**
 * @brief 从EEPROM加载固件URL
 */
void loadFirmwareUrlFromEeprom() {
    char url[151];
    for (int i = 0; i < 150; i++) {
        url[i] = EEPROM.read(100 + i);
        if (url[i] == '\0') break;
    }
    url[150] = '\0';

    if (strlen(url) > 0) {
        strcpy(otaConfig.updateServerUrl, url);
        LOG_INFO("Loaded firmware URL from EEPROM: %s", url);
    }
}
```

---

## 📖 完整配置流程

### 步骤1：配置初始固件URL

在 `esp8266_ssd1306_Clock.ino` 的 `setup()` 函数中：

```cpp
// 初始化OTA管理器
initOtaManager();

// 设置当前版本号
setOtaVersion("2.1.0");

// 配置GitHub Releases作为更新服务器
strcpy(otaConfig.updateServerUrl,
       "https://github.com/chakoe/esp8266_ssd1306_Clock/releases/download/v2.1.0/esp8266_ssd1306_Clock.ino.bin");

// 或使用CDN（推荐）
// strcpy(otaConfig.updateServerUrl,
//        "https://cdn.jsdelivr.net/gh/chakoe/esp8266_ssd1306_Clock@v2.1.0/esp8266_ssd1306_Clock.ino.bin");

LOG_INFO("OTA Manager initialized");
LOG_INFO("Firmware URL: %s", otaConfig.updateServerUrl);
```

### 步骤2：发布新版本

#### 2.1 在GitHub创建Release

1. 进入GitHub仓库
2. 点击 "Releases" → "Draft a new release"
3. 填写版本标签：`v2.2.0`
4. 上传固件文件：`esp8266_ssd1306_Clock.ino.bin`
5. 发布Release

#### 2.2 更新设备配置

**方法A：通过串口命令**
```
打开串口监视器 → 发送 'v' → 输入新URL
```

**方法B：自动检测（推荐）**
```
打开串口监视器 → 发送 'c' → 发送 'u'
```

**方法C：批量更新脚本**
编写脚本自动更新多个设备的URL。

---

## 🔄 固件地址变化处理流程

### 场景：发布新版本 v2.2.0

#### 步骤1：发布新版本
```
1. 编译新版本固件
2. 在GitHub创建Release: v2.2.0
3. 上传固件文件: esp8266_ssd1306_Clock.ino.bin
```

#### 步骤2：更新设备（3种方法）

**方法1：自动检测（推荐）**
```
设备 → 发送 'c' → 检测到新版本 → 发送 'u' → 自动更新
```

**方法2：手动更新URL**
```
设备 → 发送 'v' → 输入新URL → 发送 'u' → 更新
```

**方法3：强制更新**
```
设备 → 发送 'u' → 更新到配置的URL
```

#### 步骤3：验证更新
```
1. 设备自动重启
2. 查看串口日志确认版本
3. 发送 'o' 查看OTA状态
```

---

## 🛡️ 最佳实践

### 1. 版本管理

- ✅ 使用语义化版本号（vX.Y.Z）
- ✅ 统一文件命名规范
- ✅ 保持文件名不变，只改变版本标签
- ✅ 记录每个版本的更新日志

### 2. URL管理

- ✅ 使用CDN加速下载
- ✅ 定期备份固件文件
- ✅ 使用HTTPS确保安全
- ✅ 配置重定向机制

### 3. 更新策略

- ✅ 先在测试设备上验证
- ✅ 分批更新生产设备
- ✅ 保留回滚方案
- ✅ 监控更新成功率

### 4. 错误处理

- ✅ 实现重试机制
- ✅ 记录详细日志
- ✅ 提供回滚功能
- ✅ 通知更新失败

---

## 📊 配置对比

| 方案 | 优点 | 缺点 | 推荐度 |
|------|------|------|--------|
| **固定URL** | 简单易用 | 需要手动更新URL | ⭐⭐⭐⭐ |
| **动态检测** | 自动化 | 需要解析JSON | ⭐⭐⭐⭐⭐ |
| **配置文件** | 灵活 | 需要维护配置文件 | ⭐⭐⭐⭐ |
| **CDN加速** | 速度快 | 依赖第三方服务 | ⭐⭐⭐⭐⭐ |

---

## 🎯 推荐配置

### 综合方案（最佳实践）

```cpp
// 1. 使用CDN URL（速度快）
strcpy(otaConfig.updateServerUrl,
       "https://cdn.jsdelivr.net/gh/chakoe/esp8266_ssd1306_Clock@v2.1.0/esp8266_ssd1306_Clock.ino.bin");

// 2. 启用自动版本检测（自动化）
// 在loop()中定期检查
if (millis() - lastVersionCheck > 86400000) { // 24小时
    checkAndUpdateToLatest();
    lastVersionCheck = millis();
}

// 3. 支持手动更新URL（灵活性）
// 通过串口命令 'v' 更新

// 4. 保存URL到EEPROM（持久化）
// 重启后保持配置
```

---

## 📝 总结

### 关键要点

1. **统一文件命名** - 始终使用相同的文件名
2. **使用CDN加速** - 提高下载速度
3. **实现自动检测** - 自动发现新版本
4. **支持手动更新** - 灵活应对变化
5. **持久化配置** - 保存URL到EEPROM

### 处理地址变化的步骤

1. 发布新版本到GitHub
2. 更新版本标签（保持文件名不变）
3. 设备自动检测或手动更新URL
4. 触发OTA更新
5. 验证更新成功

### 推荐工作流

```
开发 → 编译 → 测试 → 发布Release → 设备自动更新 → 验证
```

---

## 🔗 相关资源

- **GitHub Releases**：https://github.com/chakoe/esp8266_ssd1306_Clock/releases
- **jsDelivr CDN**：https://www.jsdelivr.com/
- **GitHub API文档**：https://docs.github.com/en/rest
- **ArduinoJson库**：https://arduinojson.org/

---

**配置完成后，您的设备将能够自动检测和更新到最新版本！** 🚀
