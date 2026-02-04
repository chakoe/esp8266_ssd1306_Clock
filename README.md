# ESP8266 SSD1306 智能时钟

基于ESP8266和SSD1306 OLED显示屏的智能时钟项目，支持NTP网络时间同步、RTC实时时钟、WiFi配网、自动亮度调节等功能。

## 功能特性

### 核心功能
- ⏰ **多时间源支持**：NTP网络时间、RTC实时时钟、手动设置时间
- 🌐 **WiFi自动连接**：支持WiFiManager配网，自动重连
- 🔐 **密码加密存储**：使用AES加密存储WiFi密码
- 💡 **自动亮度调节**：支持自动调暗和夜间模式
- 💾 **亮度永久保存**：亮度设置自动保存到EEPROM，重启后保持
- 🔋 **电源管理**：自动睡眠模式，节省电量
- 📊 **网络状态显示**：实时显示WiFi连接状态和信号强度
- 🚀 **OTA远程升级**：支持通过WiFi远程更新固件（新增！）

### 显示功能
- 📅 **日期显示**：年-月-日格式
- 🕐 **时间显示**：时:分:秒格式，支持大字体切换
- 📆 **星期显示**：中文星期显示
- 🏪 **市场日显示**：自定义市场日（太守、新桥、芦圩）
- 🔤 **字体切换**：支持大字体和小字体切换
- 🌙 **夜间模式**：自动切换夜间亮度

### 按键功能
- **K1键**：亮度设置模式下增加亮度
- **K2键**：短按切换字体大小，长按进入时间源设置
- **K3键**：短按进入亮度设置，长按进入时间设置
- **K4键**：短按显示/隐藏网络状态，长按重置WiFi进入配网模式

## 硬件要求

### 主控板
- ESP8266开发板（NodeMCU、Wemos D1 Mini等）

### 显示屏
- SSD1306 OLED显示屏（128x64像素）
- I2C接口（SCL: D1, SDA: D2）

### 时钟模块
- DS1307 RTC模块（可选，推荐）
- I2C接口（与OLED共用）

### 按键
- 4个按键（K1: D3, K2: D5, K3: D6, K4: D7）
- 使用内部上拉电阻（INPUT_PULLUP）

## 引脚定义

| 功能 | 引脚 | ESP8266引脚 |
|------|------|-------------|
| OLED SCL | D1 | GPIO5 |
| OLED SDA | D2 | GPIO4 |
| K1 | D3 | GPIO0 |
| K2 | D5 | GPIO14 |
| K3 | D6 | GPIO12 |
| K4 | D7 | GPIO13 |

## 依赖库

```cpp
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <RTClib.h>
#include <WiFiManager.h>
#include <Ticker.h>
```

### 库版本要求
- ESP8266WiFi: ESP8266 Arduino Core 2.7.4+
- U8g2: 2.28.8+
- NTPClient: 3.2.0+
- RTClib: 2.1.1+
- WiFiManager: 2.0.16-beta+

## 安装步骤

### 1. 克隆项目
```bash
git clone https://github.com/chakoe/esp8266_ssd1306_Clock.git
cd esp8266_ssd1306_Clock
```

### 2. 安装依赖库
使用Arduino IDE的库管理器安装以下库：
- U8g2 by olkra
- NTPClient by Fabrice Weinberg
- RTClib by Adafruit
- WiFiManager by tzapu

### 3. 配置Arduino IDE
1. 打开Arduino IDE
2. 文件 -> 首选项 -> 附加开发板管理器网址
3. 添加：`http://arduino.esp8266.com/stable/package_esp8266com_index.json`
4. 工具 -> 开发板 -> 开发板管理器 -> 搜索ESP8266 -> 安装
5. 工具 -> 开发板 -> 选择NodeMCU 1.0 (ESP-12E Module)

### 4. 编译上传
1. 连接ESP8266到电脑
2. 选择正确的串口
3. 点击上传按钮

### 5. 使用Arduino CLI（推荐）
```bash
# 编译固件
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 --export-binaries

# 上传固件
arduino-cli upload -p COM3 --fqbn esp8266:esp8266:nodemcuv2 --input-dir build/esp8266.esp8266.nodemcuv2
```

## OTA远程升级功能

### 🚀 功能说明

本项目支持OTA（Over-The-Air）远程固件升级，无需物理连接USB线即可更新设备固件。

### 📋 OTA命令

通过串口监视器（波特率115200）发送以下命令：

| 命令 | 功能 |
|------|------|
| `u` | 触发OTA更新（从GitHub下载最新固件） |
| `o` | 显示OTA状态和进度 |
| `v` | 设置自定义固件URL |

### 🎯 使用步骤

#### 1. 查看OTA状态
```
打开串口监视器 → 发送 'o'
```

#### 2. 触发OTA更新
```
打开串口监视器 → 发送 'u'
```

设备会自动从GitHub下载最新固件并安装，完成后自动重启。

#### 3. 观察更新进度
```
[时间] OTA Update started
[时间] OTA Update Progress: 10%
[时间] OTA Update Progress: 20%
...
[时间] OTA Update Progress: 100%
[时间] OTA Update successful
```

### 📖 详细文档

- **OTA功能已启用**：查看 `OTA功能已启用.md`
- **完整使用指南**：查看 `OTA使用指南.md`
- **快速开始**：查看 `OTA快速开始.md`
- **集成示例**：查看 `ota_integration_example.cpp`

### ⚠️ 注意事项

- 确保设备连接到WiFi
- 确保网络连接稳定
- 更新过程中不要断开电源
- 更新完成后设备会自动重启

## 配置说明

### 生产环境配置
编辑 `production_config.h` 文件：

```cpp
// WiFiManager配置
#define WIFI_MANAGER_AP_PASSWORD ""              // AP密码（空表示不设置密码，方便用户配置）
#define WIFI_MANAGER_AP_TIMEOUT 180              // AP模式超时时间（秒）
#define WIFI_MANAGER_CONNECT_TIMEOUT 30          // 连接超时时间（秒）

// 电源管理配置
#define AUTO_DIM_ENABLED true                    // 是否启用自动调暗
#define AUTO_DIM_TIMEOUT_MS 300000               // 自动调暗时间（毫秒）
#define AUTO_SLEEP_TIMEOUT_MS 1800000            // 自动睡眠时间（毫秒）
```

### 市场日配置
编辑 `system_manager.cpp` 中的 `getCorrectOffset()` 函数：

```cpp
// 设置目标日期：2025年11月29日
struct tm targetDate = {0};
targetDate.tm_year = 2025 - 1900;
targetDate.tm_mon = 10;      // 11月
targetDate.tm_mday = 29;
```

## 使用说明

### 首次使用（配网）
1. 上电后，长按K4键（超过500ms）进入配网模式
2. 连接WiFi热点：`Clock_AP_xxxxxxxx`（无需密码）
3. 在浏览器中打开 `192.168.4.1`
4. 选择WiFi网络并输入密码
5. 等待连接成功

### 正常使用
- **查看时间**：正常显示当前时间和日期
- **切换字体**：短按K2键
- **调整亮度**：短按K3键进入亮度设置，使用K1/K2调整，K4确认（设置自动保存）
- **设置时间**：长按K3键进入时间设置，使用K1/K2调整，K3切换字段，K4确认
- **切换时间源**：长按K2键进入时间源设置，使用K2选择，K4确认
- **查看网络状态**：短按K4键显示网络信息（5秒后自动消失）
- **重置WiFi**：长按K4键重置WiFi配置，重新配网

### 时间源说明
- **NTP**：网络时间（需要WiFi连接）
- **RTC**：实时时钟（需要DS1307模块）
- **CLK**：软件时钟（手动设置的时间）

系统会自动选择最佳时间源，优先级：RTC > NTP > CLK

## 故障排除

### WiFi连接失败
1. 检查WiFi密码是否正确
2. 检查路由器是否支持2.4GHz频段
3. 尝试长按K4键重置WiFi配置

### 时间显示错误
1. 检查网络连接状态
2. 检查NTP服务器是否可达
3. 尝试手动设置时间
4. 检查RTC模块是否连接正常

### OLED不显示
1. 检查I2C接线是否正确
2. 检查OLED地址是否正确（默认0x3C）
3. 检查电源供应是否正常

### 按键无响应
1. 检查按键接线是否正确
2. 检查按键是否正常工作
3. 重启设备

## 编译选项

### 开发模式
```bash
-DDEBUG_MODE
```

### 生产模式
```bash
-DPRODUCTION_MODE
```

或者直接在 `production_config.h` 中修改：
```cpp
#define PRODUCTION_MODE
```

## 代码结构

```
esp8266_ssd1306_Clock/
├── esp8266_ssd1306_Clock.ino    # 主程序
├── config.h                      # 基础配置
├── production_config.h           # 生产环境配置
├── global_config.h/cpp           # 全局配置和变量
├── eeprom_config.h/cpp           # EEPROM配置管理（亮度保存）
├── button_handler.h/cpp          # 按键处理
├── display_manager.h/cpp         # 显示管理
├── time_manager.h/cpp            # 时间管理
├── system_manager.h/cpp          # 系统管理
├── power_manager.h/cpp           # 电源管理
├── i2c_manager.h/cpp             # I2C通信管理
├── monitoring_system.h/cpp       # 系统监控
├── logger.h/cpp                  # 日志系统
├── utils.h/cpp                   # 工具函数
└── libraries/                    # 依赖库
```

## 性能优化

- 使用PROGMEM存储常量字符串，节省RAM
- 使用非阻塞延时替代delay()
- 使用yield()让出CPU时间
- 使用静态缓冲区避免频繁内存分配
- 使用硬件看门狗防止系统卡死
- 所有时间比较都有溢出保护

## 安全性

- WiFi密码使用AES加密存储
- 没有硬编码敏感信息
- 使用INPUT_PULLUP提高按键安全性
- 完善的错误处理机制

## 已知问题

1. 长期运行（超过49天）后，millis()会溢出，但已通过溢出保护解决
2. AES加密是简化实现，生产环境建议使用ESP8266硬件加密

## 更新日志

### v2.1 (2025-02-03)
- 新增亮度永久保存功能
- 使用EEPROM存储亮度设置
- 重启后自动恢复上次亮度设置
- 添加EEPROM数据校验机制

### v2.0 (2025-12-23)
- 修复所有已知的16个bug
- 添加所有millis()溢出保护
- 添加所有数组边界检查
- 优化按键消抖逻辑
- 完善错误处理机制
- 添加生产环境配置
- 创建完整文档

### v1.0 (2025-12-20)
- 初始版本
- 基本时钟功能
- WiFi配网
- NTP时间同步

## 贡献

欢迎提交Issue和Pull Request！

## 许可证

MIT License

## 作者

ESP8266 SSD1306 Clock Project

## 致谢

- U8g2库：https://github.com/olikraus/u8g2
- NTPClient库：https://github.com/arduino-libraries/NTPClient
- RTClib库：https://github.com/adafruit/RTClib
- WiFiManager库：https://github.com/tzapu/WiFiManager
