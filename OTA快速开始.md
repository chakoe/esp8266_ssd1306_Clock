# OTA升级功能 - 快速开始指南

## 什么是OTA升级？

OTA（Over-The-Air）升级允许您通过WiFi网络远程更新ESP8266设备的固件，无需物理连接USB线。

## 当前状态

您的项目已经包含完整的OTA管理器代码（`ota_manager.h` 和 `ota_manager.cpp`），但尚未集成到主程序中。

## 快速启用（5分钟）

### 步骤1：修改主程序（2分钟）

打开 `esp8266_ssd1306_Clock.ino`，进行以下修改：

#### 1.1 添加头文件

在文件开头添加：

```cpp
#include "ota_manager.h"
```

#### 1.2 在setup()中初始化OTA

找到WiFi连接成功的代码后，添加：

```cpp
// 初始化OTA管理器
initOtaManager();

// 设置版本号
setOtaVersion("2.1.0");

// 配置更新服务器（替换为你的服务器地址）
strcpy(otaConfig.updateServerUrl,
       "http://192.168.1.100/firmware/esp8266_ssd1306_Clock.ino.bin");

LOG_INFO("OTA Manager initialized");
```

#### 1.3 在loop()中添加处理代码

在loop()函数的yield()之前添加：

```cpp
// 处理OTA更新
handleOtaUpdate();

// 串口命令：发送 'u' 触发OTA更新
if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == 'u' || command == 'U') {
        LOG_INFO("Starting OTA update...");
        if (startOtaUpdate(otaConfig.updateServerUrl)) {
            LOG_INFO("OTA update started");
        } else {
            LOG_WARNING("OTA failed: %s", otaState.error);
        }
    }
}
```

### 步骤2：编译固件（1分钟）

```bash
cd c:/Users/Administrator/esp8266_ssd1306_Clock
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 --export-binaries
```

固件位置：`build/esp8266.esp8266.nodemcuv2/esp8266_ssd1306_Clock.ino.bin`

### 步骤3：上传固件到设备（1分钟）

使用之前创建的批处理脚本：

```cmd
upload_to_com3.bat
```

**重要**：右键点击脚本，选择"以管理员身份运行"

### 步骤4：准备更新服务器（1分钟）

#### 选项A：使用本地HTTP服务器（推荐用于测试）

1. 在电脑上创建一个文件夹，例如 `C:\firmware`
2. 将编译好的 `.bin` 文件复制到该文件夹
3. 使用Python启动简单的HTTP服务器：

```bash
cd C:\firmware
python -m http.server 8000
```

4. 修改设备中的固件URL为：
```
http://你的电脑IP:8000/esp8266_ssd1306_Clock.ino.bin

```

#### 选项B：使用GitHub Releases

1. 在GitHub上创建Release
2. 上传 `.bin` 文件
3. 使用下载链接作为固件URL

示例：
```
https://github.com/chakoe/esp8266_ssd1306_Clock/releases/download/Releasev2.1.0/esp8266_ssd1306_Clock.ino.bin
```

### 步骤5：测试OTA更新（1分钟）

1. 打开串口监视器（波特率115200）
2. 发送字符 `u` 触发OTA更新
3. 观察日志输出：
   ```
   [时间] Starting OTA update...
   [时间] OTA Update started
   [时间] OTA Update Progress: 10%
   [时间] OTA Update Progress: 20%
   ...
   [时间] OTA Update successful
   ```
4. 设备将自动重启

## 串口命令

| 命令 | 功能 | 说明 |
|------|------|------|
| `u` | 触发OTA更新 | 开始从配置的URL下载并更新固件 |
| `o` | 显示OTA状态 | 查看当前OTA状态、进度等信息 |
| `v` | 设置固件URL | 输入新的固件下载地址 |

## 常见问题

### Q1: 更新失败，提示权限错误

**A**: 这是串口权限问题，与OTA无关。上传固件时需要管理员权限。

### Q2: OTA更新失败，提示"HTTP_UPDATE_FAILED"

**A**: 检查以下几点：
1. 设备是否连接到WiFi
2. 固件URL是否正确
3. 服务器是否可以访问
4. 固件文件是否存在

### Q3: 更新后设备不启动

**A**:
1. 使用串口上传恢复固件
2. 检查固件是否与设备兼容
3. 查看串口日志了解启动错误

### Q4: 如何查看OTA进度？

**A**:
1. 在串口监视器中会实时显示进度
2. 发送 `o` 命令查看详细状态
3. 可以修改代码在OLED屏幕上显示进度

## 高级功能

### 自动更新检查

在setup()中启用：

```cpp
otaConfig.autoUpdateEnabled = true;
otaConfig.checkInterval = 86400000; // 24小时检查一次
```

### Web界面OTA

添加Web服务器支持（详见 `ota_integration_example.cpp`）：

```cpp
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

void setup() {
    // ... 其他初始化 ...

    server.on("/ota/update", HTTP_POST, []() {
        String url = server.arg("url");
        if (startOtaUpdate(url.c_str())) {
            server.send(200, "text/plain", "OTA started");
        } else {
            server.send(500, "text/plain", otaState.error);
        }
    });

    server.begin();
}

void loop() {
    server.handleClient();
    handleOtaUpdate();
}
```

### 按键触发OTA

修改按键处理，长按某个按键触发OTA（详见 `ota_integration_example.cpp`）。

## 安全建议

1. **测试固件**：先通过串口上传测试
2. **备份固件**：保留旧版本
3. **使用HTTPS**：生产环境使用HTTPS URL
4. **版本验证**：更新前检查版本号
5. **渐进式更新**：先更新少量设备

## 参考文档

- **完整指南**：`OTA使用指南.md`
- **集成示例**：`ota_integration_example.cpp`
- **源代码**：`ota_manager.h` / `ota_manager.cpp`

## 总结

启用OTA升级只需3个简单步骤：

1. ✅ 修改主程序，添加OTA初始化代码
2. ✅ 编译并上传固件到设备
3. ✅ 准备更新服务器，通过串口命令触发更新

就这么简单！您现在可以远程更新设备固件了。

## 获取帮助

如有问题：
1. 查看 `OTA使用指南.md` 获取详细说明
2. 查看 `ota_integration_example.cpp` 获取更多示例
3. 检查串口日志了解错误信息
4. 在项目GitHub上提交Issue

---

**祝您使用愉快！** 🚀
