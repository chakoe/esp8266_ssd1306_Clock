# ESP8266 SSD1306 时钟 - OTA升级使用指南

## 概述

OTA（Over-The-Air）升级功能允许您通过WiFi网络远程更新ESP8266设备的固件，无需物理连接USB线。本项目已经实现了完整的OTA升级管理模块（`ota_manager.h` 和 `ota_manager.cpp`）。

## 功能特性

- ✅ 远程固件更新
- ✅ 进度显示（0-100%）
- ✅ 错误处理和恢复
- ✅ 自动检查更新
- ✅ 版本管理
- ✅ LED指示灯反馈

## 当前状态

⚠️ **注意**：OTA管理器代码已经实现，但尚未集成到主程序中。需要进行以下步骤才能启用OTA功能：

1. 在主程序中集成OTA管理器
2. 配置更新服务器URL
3. 添加OTA触发机制（按键或Web界面）

## 集成步骤

### 1. 在主程序中包含OTA头文件

在 `esp8266_ssd1306_Clock.ino` 的开头添加：

```cpp
#include "ota_manager.h"
```

### 2. 在setup()函数中初始化OTA管理器

在 `setup()` 函数的WiFi连接成功后添加：

```cpp
// 初始化OTA管理器
initOtaManager();
```

### 3. 在loop()函数中处理OTA更新

在 `loop()` 函数中添加：

```cpp
// 处理OTA更新
handleOtaUpdate();
```

### 4. 配置OTA参数

在 `setup()` 函数中设置OTA配置：

```cpp
// 设置OTA版本号
setOtaVersion("2.1.0");

// 配置更新服务器URL
strcpy(otaConfig.updateServerUrl, "http://your-server.com/firmware.bin");

// 启用自动更新检查（可选）
otaConfig.autoUpdateEnabled = true;
otaConfig.checkInterval = 86400000; // 24小时检查一次
```

## 使用方法

### 方法1：手动触发OTA更新

通过串口命令触发OTA更新：

```cpp
// 在loop()函数中添加串口命令处理
if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == 'u' || command == 'U') {
        LOG_INFO("Starting OTA update...");
        if (startOtaUpdate("http://your-server.com/firmware.bin")) {
            LOG_INFO("OTA update started successfully");
        } else {
            LOG_WARNING("OTA update failed: %s", otaState.error);
        }
    }
}
```

### 方法2：通过按键触发OTA更新

修改按键处理逻辑，添加OTA触发功能：

```cpp
// 在button_handler.cpp中添加OTA触发
if (btnIndex == 3 && pressDuration >= 5000) { // K4键长按5秒
    LOG_INFO("Long press detected, starting OTA update...");
    if (startOtaUpdate(otaConfig.updateServerUrl)) {
        displayOtaProgress();
    }
}
```

### 方法3：Web界面OTA更新（推荐）

添加简单的Web服务器来触发OTA更新：

```cpp
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

void setupWebServer() {
    server.on("/ota", HTTP_POST, []() {
        String firmwareUrl = server.arg("url");
        if (firmwareUrl.length() > 0) {
            LOG_INFO("OTA update requested: %s", firmwareUrl.c_str());
            server.send(200, "text/plain", "OTA update started");

            // 开始OTA更新
            startOtaUpdate(firmwareUrl.c_str());
        } else {
            server.send(400, "text/plain", "Missing firmware URL");
        }
    });

    server.begin();
}

void loop() {
    server.handleClient();
    handleOtaUpdate();
}
```

## 固件服务器设置

### 方案1：使用HTTP服务器

1. 搭建一个简单的HTTP服务器（Apache、Nginx等）
2. 将编译好的固件文件（`.bin`）上传到服务器
3. 确保固件文件可以通过HTTP访问

### 方案2：使用GitHub Releases

1. 在GitHub上创建Release
2. 上传固件文件到Release
3. 使用GitHub的下载链接作为固件URL

示例URL：
```
https://github.com/yourusername/esp8266_ssd1306_Clock/releases/download/v2.1.0/esp8266_ssd1306_Clock.ino.bin
```

### 方案3：使用专用OTA服务器

使用专门的OTA服务，如：
- **Mongoose OS Cloud**
- **AWS IoT OTA**
- **Azure IoT Hub**
- **自定义OTA服务器**

## 编译固件用于OTA

### 使用Arduino CLI

```bash
# 编译并导出bin文件
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 --export-binaries

# 固件位置
build/esp8266.esp8266.nodemcuv2/esp8266_ssd1306_Clock.ino.bin
```

### 使用Arduino IDE

1. 工具 -> 开发板 -> NodeMCU 1.0 (ESP-12E Module)
2. 草图 -> 导出编译的二进制文件
3. 在项目目录中找到 `.ino.bin` 文件

## OTA状态监控

### 获取OTA状态

```cpp
// 获取当前状态
const char* status = getOtaStatusString(otaState.status);
LOG_INFO("OTA Status: %s", status);

// 获取进度
LOG_INFO("OTA Progress: %d%%", otaState.progress);

// 获取错误信息
if (otaState.status == OTA_STATUS_FAILED) {
    LOG_WARNING("OTA Error: %s", otaState.error);
}
```

### OTA状态枚举

```cpp
typedef enum {
    OTA_STATUS_IDLE,           // 空闲状态
    OTA_STATUS_CHECKING,       // 检查更新
    OTA_STATUS_DOWNLOADING,    // 下载中
    OTA_STATUS_UPDATING,       // 更新中
    OTA_STATUS_SUCCESS,        // 成功
    OTA_STATUS_FAILED,         // 失败
    OTA_STATUS_ERROR           // 错误
} OtaStatus;
```

## 显示OTA进度

在OLED屏幕上显示OTA更新进度：

```cpp
void displayOtaProgress() {
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.setCursor(10, 20);
    u8g2.print("OTA Update");

    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.setCursor(10, 40);
    u8g2.print("Progress:");

    u8g2.setFont(u8g2_font_ncenB24_tr);
    u8g2.setCursor(30, 65);
    u8g2.print(otaState.progress);
    u8g2.print("%");

    u8g2.sendBuffer();
}
```

## 安全考虑

### 1. 固件验证

建议添加固件签名验证：

```cpp
bool verifyFirmware(const char* firmwareUrl) {
    // 下载固件并验证签名
    // 实现SHA256签名验证
    return true;
}
```

### 2. HTTPS支持

使用HTTPS URL提高安全性：

```cpp
String firmwareUrl = "https://your-server.com/firmware.bin";
```

### 3. 版本检查

在更新前检查版本号：

```cpp
bool isNewerVersion(const char* latestVersion) {
    // 比较版本号
    // 返回true如果最新版本比当前版本新
    return true;
}
```

### 4. 回滚机制

实现固件回滚功能：

```cpp
void rollbackFirmware() {
    // 回滚到上一个版本
    ESP.restart();
}
```

## 故障排除

### 问题1：OTA更新失败

**原因**：
- WiFi连接不稳定
- 固件URL错误
- 服务器无法访问
- 固件文件损坏

**解决方法**：
1. 检查WiFi连接状态
2. 验证固件URL是否正确
3. 测试服务器是否可以访问
4. 重新编译固件

### 问题2：更新后设备不启动

**原因**：
- 固件文件不兼容
- Flash分区配置错误
- 内存不足

**解决方法**：
1. 使用串口上传恢复固件
2. 检查Flash分区表配置
3. 优化固件大小

### 问题3：进度显示不准确

**原因**：
- 回调函数未正确设置
- 网络延迟

**解决方法**：
1. 确保 `setOtaProgressCallback()` 被调用
2. 增加进度更新频率

## 完整示例代码

### 集成OTA到主程序

```cpp
// 在esp8266_ssd1306_Clock.ino中添加

#include "ota_manager.h"

// 在setup()函数中添加
void setup() {
    // ... 现有初始化代码 ...

    // WiFi连接成功后
    if (systemState.networkConnected) {
        // 初始化OTA管理器
        initOtaManager();

        // 设置版本号
        setOtaVersion("2.1.0");

        // 配置更新服务器
        strcpy(otaConfig.updateServerUrl,
               "http://your-server.com/firmware.bin");

        LOG_INFO("OTA Manager initialized");
    }
}

// 在loop()函数中添加
void loop() {
    // ... 现有循环代码 ...

    // 处理OTA更新
    handleOtaUpdate();

    // 检查串口命令
    if (Serial.available() > 0) {
        char cmd = Serial.read();
        if (cmd == 'u') {
            LOG_INFO("Starting OTA update...");
            if (startOtaUpdate(otaConfig.updateServerUrl)) {
                LOG_INFO("OTA update started");
            } else {
                LOG_WARNING("OTA failed: %s", otaState.error);
            }
        }
    }
}
```

## 最佳实践

1. **测试固件**：先通过串口上传测试固件，确认无误后再使用OTA
2. **备份固件**：保留旧版本固件以便回滚
3. **渐进式更新**：先更新少量设备，确认稳定后再全面更新
4. **监控日志**：更新时监控日志输出
5. **网络稳定**：确保WiFi连接稳定
6. **电量充足**：更新时确保设备有足够电量
7. **版本管理**：使用语义化版本号（如 v2.1.0）

## 总结

OTA升级功能已经完整实现，只需简单集成到主程序中即可使用。建议先通过串口命令测试OTA功能，确认正常后再添加Web界面或按键触发。

通过OTA升级，您可以：
- 远程更新设备固件
- 快速修复bug
- 添加新功能
- 提高用户体验

如有问题或建议，请查看项目文档或提交Issue。
