# ESP8266 SSD1306 智能时钟

基于ESP8266和SSD1306 OLED显示屏时钟项目，支持NTP网络时间同步、RTC实时时钟、WiFi配网、自动亮度调节、Web OTA更新等功能。

## 📋 项目概述

- ✅ **代码质量**：模块化设计，遵循最佳实践
- ✅ **错误处理**：完善的错误恢复机制
- ✅ **内存优化**：使用PROGMEM，RAM使用率仅52%
- ✅ **安全性**：AES加密存储敏感信息
- ✅ **稳定性**：硬件看门狗，溢出保护，边界检查
- ✅ **可维护性**：清晰的代码结构和文档
- ✅ **可扩展性**：易于添加新功能

## 🎯 核心功能

### 时间管理
- ⏰ **多时间源支持**：NTP网络时间、RTC实时时钟、手动设置时间
- 🔄 **自动时间源切换**：网络断开时自动切换到RTC
- 🌐 **多NTP服务器**：支持故障转移，提高可靠性
- 📅 **日期显示**：YYYY-MM-DD格式
- 🕐 **时间显示**：HH:MM:SS格式，24小时制
- 📆 **星期显示**：中文星期显示（周日-周六）
- 🏪 **市场日显示**：自定义市场日（太守、新桥、芦圩）
- 🔤 **字体切换**：支持大字体（24pt）和小字体（18pt）

### 网络功能
- 🌐 **WiFi自动连接**：支持WiFiManager配网，自动重连
- 🔐 **密码加密存储**：使用AES-128加密存储WiFi密码
- 📊 **网络状态显示**：实时显示WiFi连接状态和信号强度
- 🔄 **智能重连**：网络断开时自动尝试重连
- 🌍 **多NTP服务器**：支持4个NTP服务器故障转移

### 显示功能
- 💡 **多级亮度调节**：4级亮度（低亮、中亮、高亮、最亮）
- 💾 **亮度永久保存**：亮度设置自动保存到EEPROM，重启后保持
- 🌙 **自动调暗**：5分钟无操作后自动降低亮度
- 🌙 **夜间模式**：晚上22:00-07:00自动切换到夜间亮度
- 🔋 **自动睡眠**：30分钟无操作后进入睡眠模式
- 📡 **时间源指示**：NTP(*), RTC(R), CLK(S), 错误(!)

### 按键功能
- **K1键**：亮度设置模式下增加亮度
- **K2键**：短按切换字体大小，长按进入时间源设置
- **K3键**：短按进入亮度设置，长按进入时间设置
- **K4键**：短按显示/隐藏网络状态，长按重置WiFi进入配网模式

### 高级功能
- 🔄 **Web OTA更新**：长按K1键5秒启动Web OTA服务器，支持浏览器固件更新
- 📊 **运行时监控**：内存使用、运行时间、错误统计
- 🛡️ **错误恢复**：自动错误检测和恢复机制
- 📝 **分级日志**：支持ERROR、WARNING、INFO、DEBUG、VERBOSE级别
- 🔧 **系统看门狗**：硬件看门狗防止系统卡死
- ⚡ **优化编译**：生产环境优化，减小固件大小

## 🏗️ 项目结构

```
esp8266_ssd1306_Clock/
├── esp8266_ssd1306_Clock.ino    # 主程序入口
├── config.h                      # 基础配置（引脚定义、常量）
├── production_config.h           # 生产环境配置（日志级别、优化选项）
├── production_config_complete.h  # 完整生产配置（备用）
├── global_config.h               # 全局配置和变量声明
├── global_config.cpp             # 全局配置和变量定义
├── eeprom_config.h               # EEPROM配置管理头文件
├── eeprom_config.cpp             # EEPROM配置管理实现
├── eeprom_config_test.cpp        # EEPROM配置测试
├── button_handler.h              # 按键处理头文件
├── button_handler.cpp            # 按键处理实现
├── display_manager.h             # 显示管理头文件
├── display_manager.cpp           # 显示管理实现
├── time_manager.h                # 时间管理头文件
├── time_manager.cpp              # 时间管理实现
├── system_manager.h              # 系统管理头文件
├── system_manager.cpp            # 系统管理实现
├── setup_manager.h               # 初始化管理头文件
├── setup_manager.cpp             # 初始化管理实现
├── config_manager.h              # 配置管理头文件
├── config_manager.cpp            # 配置管理实现
├── i2c_manager.h                 # I2C通信管理头文件
├── i2c_manager.cpp               # I2C通信管理实现
├── error_recovery.h              # 错误恢复头文件
├── error_recovery.cpp            # 错误恢复实现
├── runtime_monitor.h             # 运行时监控头文件
├── runtime_monitor.cpp           # 运行时监控实现
├── logger.h                      # 日志系统头文件
├── logger.cpp                    # 日志系统实现
├── utils.h                       # 工具函数头文件
├── utils.cpp                     # 工具函数实现
├── web_ota_manager.h             # Web OTA管理头文件
├── web_ota_manager.cpp           # Web OTA管理实现
├── test_framework.h              # 测试框架头文件
├── test_framework.cpp            # 测试框架实现
├── test_suites.h                 # 测试套件头文件
├── test_suites.cpp               # 测试套件实现
├── integration_tests.h           # 集成测试头文件
├── integration_tests.cpp         # 集成测试实现
├── boards.local.txt              # Arduino IDE编译配置
├── .gitignore                    # Git忽略文件
├── LICENSE                       # MIT许可证
├── README.md                     # 项目说明文档
├── USER_MANUAL.md                # 用户手册
└── .arts/                        # CodeArts配置
```

## 🛠️ 硬件要求

### 主控板
- ESP8266开发板（推荐：NodeMCU 1.0、Wemos D1 Mini）
- Flash大小：4MB（推荐）
- CPU频率：80MHz

### 显示屏
- SSD1306 OLED显示屏（128x64像素）
- I2C接口（SCL: D1/GPIO5, SDA: D2/GPIO4）
- 显示地址：0x3C（默认）

### 时钟模块（可选但推荐）
- DS1307 RTC模块
- I2C接口（与OLED共用）
- 支持纽扣电池供电（CR1220）

### 按键
- 4个按键（K1, K2, K3, K4）
- 使用内部上拉电阻（INPUT_PULLUP）
- 推荐使用轻触开关

### 电源
- 5V USB供电（推荐）
- 或3.3V供电（需注意电压稳定性）
- 正常功耗：100-150mA
- 睡眠功耗：20-30mA

## 🔌 引脚定义

| 功能 | 引脚 | ESP8266引脚 | GPIO | 说明 |
|------|------|-------------|------|------|
| OLED SCL | D1 | GPIO5 | 5 | I2C时钟线 |
| OLED SDA | D2 | GPIO4 | 4 | I2C数据线 |
| K1 | D3 | GPIO0 | 0 | 按键1（OTA触发） |
| K2 | D5 | GPIO14 | 14 | 按键2（字体/时间源） |
| K3 | D6 | GPIO12 | 12 | 按键3（亮度/时间设置） |
| K4 | D7 | GPIO13 | 13 | 按键4（网络状态/WiFi重置） |

### 接线图

```
ESP8266 (NodeMCU)       SSD1306 OLED
┌─────────────┐         ┌─────────────┐
│             │         │             │
│  D1 (GPIO5) ├─────────┤ SCL         │
│  D2 (GPIO4) ├─────────┤ SDA         │
│  3.3V       ├─────────┤ VCC         │
│  GND        ├─────────┤ GND         │
│             │         └─────────────┘
│             │
│  D3 (GPIO0) ├─────────┤ K1 (按键)   ├─ GND
│  D5 (GPIO14)├─────────┤ K2 (按键)   ├─ GND
│  D6 (GPIO12)├─────────┤ K3 (按键)   ├─ GND
│  D7 (GPIO13)├─────────┤ K4 (按键)   ├─ GND
│             │
│  D1 (GPIO5) ├─────────┤ SCL (RTC)   │
│  D2 (GPIO4) ├─────────┤ SDA (RTC)   │
│  3.3V       ├─────────┤ VCC (RTC)   │
│  GND        ├─────────┤ GND (RTC)   │
└─────────────┘
```

## 📦 依赖库

### 必需库
```cpp
#include <ESP8266WiFi.h>        // ESP8266 WiFi功能
#include <WiFiManager.h>         // WiFi配网管理
#include <Wire.h>                // I2C通信
#include <U8g2lib.h>             // OLED显示驱动
#include <NTPClient.h>           // NTP时间同步
#include <WiFiUdp.h>             // UDP通信
#include <RTClib.h>              // RTC时钟驱动
#include <Ticker.h>              // 软件看门狗
```

### 库版本要求
- ESP8266WiFi: ESP8266 Arduino Core 2.7.4+
- U8g2: 2.28.8+
- NTPClient: 3.2.0+
- RTClib: 2.1.1+
- WiFiManager: 2.0.16-beta+

### 库安装方法

#### 使用Arduino IDE库管理器
1. 打开Arduino IDE
2. 工具 -> 管理库
3. 搜索并安装以下库：
   - U8g2 by olkra
   - NTPClient by Fabrice Weinberg
   - RTClib by Adafruit
   - WiFiManager by tzapu

#### 使用命令行安装
```bash
# U8g2
git clone https://github.com/olikraus/u8g2.git ~/Arduino/libraries/u8g2

# NTPClient
git clone https://github.com/arduino-libraries/NTPClient.git ~/Arduino/libraries/NTPClient

# RTClib
git clone https://github.com/adafruit/RTClib.git ~/Arduino/libraries/RTClib

# WiFiManager
git clone https://github.com/tzapu/WiFiManager.git ~/Arduino/libraries/WiFiManager
```

## 🚀 安装步骤

### 方法一：使用Arduino IDE

#### 1. 安装ESP8266开发板
1. 打开Arduino IDE
2. 文件 -> 首选项
3. 在"附加开发板管理器网址"中添加：
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
4. 工具 -> 开发板 -> 开发板管理器
5. 搜索"ESP8266"并安装

#### 2. 克隆项目
```bash
git clone https://github.com/chakoe/esp8266_ssd1306_Clock.git
cd esp8266_ssd1306_Clock
```

#### 3. 打开项目
1. 在Arduino IDE中打开 `esp8266_ssd1306_Clock.ino`
2. 工具 -> 开发板 -> 选择"NodeMCU 1.0 (ESP-12E Module)"
3. 工具 -> Flash Size -> 选择"4MB (FS:2MB OTA:~1019KB)"
4. 工具 -> Upload Speed -> 选择"921600"

#### 4. 编译上传
1. 连接ESP8266到电脑
2. 选择正确的串口（如COM3）
3. 点击上传按钮
4. 等待上传完成

### 方法二：使用命令行

#### 1. 安装Arduino CLI
```bash
# Windows
# 从 https://arduino.cc/en/software 下载Arduino CLI

# macOS
brew install arduino-cli

# Linux
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
```

#### 2. 配置开发板
```bash
arduino-cli core update-index
arduino-cli core install esp8266:esp8266
```

#### 3. 安装依赖库
```bash
arduino-cli lib install "U8g2"
arduino-cli lib install "NTPClient"
arduino-cli lib install "RTClib"
arduino-cli lib install "WiFiManager"
```

#### 4. 编译上传
```bash
# 编译
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 \
  --build-property build.partitions=minimal \
  --build-property "upload.speed=921600" \
  --build-property "compiler.c.extra_flags=-DPRODUCTION_MODE" \
  --build-property "compiler.cpp.extra_flags=-DPRODUCTION_MODE" \
  .

# 上传
arduino-cli upload -p COM3 --fqbn esp8266:esp8266:nodemcuv2 --input-dir build
```

## ⚙️ 配置说明

### 生产环境配置

编辑 `production_config.h` 文件：

```cpp
// 生产模式开关
#define PRODUCTION_MODE

// 日志级别设置
#define DEFAULT_LOG_LEVEL LOG_LEVEL_WARNING  // 生产环境：仅输出错误和警告
#define ENABLE_TIMESTAMP false               // 生产环境：禁用时间戳
#define ENABLE_DEBUG_LOGS false              // 生产环境：禁用调试日志

// 系统配置
#define WATCHDOG_TIMEOUT_MS 8000             // 硬件看门狗超时时间
#define WIFI_TIMEOUT_SECONDS 30              // WiFi连接超时时间
#define NTP_SYNC_INTERVAL_MS 60000           // NTP同步间隔

// 电源管理配置
#define AUTO_DIM_ENABLED true                // 启用自动调暗
#define AUTO_DIM_TIMEOUT_MS 300000           // 5分钟自动调暗
#define AUTO_SLEEP_TIMEOUT_MS 1800000        // 30分钟自动睡眠

// WiFiManager配置
#define WIFI_MANAGER_AP_PASSWORD ""          // AP密码（空表示不设置密码）
#define WIFI_MANAGER_AP_TIMEOUT 180          // AP模式超时时间（秒）
#define WIFI_MANAGER_CONNECT_TIMEOUT 30      // 连接超时时间（秒）

// Web OTA配置
#define WEB_OTA_USERNAME "admin"             // OTA用户名
#define WEB_OTA_PASSWORD "admin123"          // OTA密码
```

### 市场日配置

编辑 `system_manager.cpp` 中的 `getCorrectOffset()` 函数：

```cpp
// 设置目标日期：2026年2月4日（太守日）
// 需要确保该日期对应市场日索引0（太守）
int getCorrectOffset() {
  // 计算从2023年1月1日到目标日期的天数
  // 2023年：365天（不是闰年）
  // 2024年：366天（是闰年）
  // 2025年：365天（不是闰年）
  // 2026年：1月31天 + 2月1-3日 = 34天

  long totalDays = 365 + 366 + 365 + 34;

  // 计算偏移量，确保目标日期对应太守（市场索引0）
  // getDaysSince2023_01_01(2026, 2, 4) = 1130
  // 1130 % 3 = 2
  // 需要满足：(1130 + offset) % 3 = 0
  // 因此 offset = (0 - 2 + 3) % 3 = 1
  cachedOffsetValue = (0 - (totalDays % 3) + 3) % 3;

  return cachedOffsetValue;
}
```

## 📖 使用说明

### 首次使用（WiFi配网）

1. **上电启动**
   - 连接电源到ESP8266设备
   - OLED将显示启动画面（时钟图标）
   - 约1秒后显示时间

2. **进入配网模式**
   - 长按K4键（超过500ms）
   - OLED显示"配网模式"
   - 等待3秒后进入后台配网

3. **连接WiFi热点**
   - 在手机/电脑上搜索WiFi热点：`Clock_AP_xxxxxxxx`（无需密码）
   - 连接到热点

4. **配置WiFi**
   - 打开浏览器访问：`http://192.168.4.1`
   - 选择你的WiFi网络
   - 输入WiFi密码
   - 点击"保存"

5. **等待连接**
   - 等待设备连接到WiFi
   - 连接成功后自动重启
   - OLED显示当前时间

### 正常使用

#### 基本操作
- **查看时间**：正常显示当前时间和日期
- **切换字体**：短按K2键（大字体/小字体）
- **查看网络状态**：短按K4键（显示5秒后自动消失）

#### 亮度设置
1. 短按K3键进入亮度设置
2. 使用K1/K2调整亮度（低亮/中亮/高亮/最亮）
3. 短按K4键确认并退出（设置自动保存到EEPROM）
4. 或等待15秒自动退出（设置自动保存）

#### 时间设置
1. 长按K3键（超过500ms）进入时间设置
2. 使用K1/K2调整当前字段的值
3. 短按K3键切换设置字段（年→月→日→时→分→秒）
4. 短按K4键确认并退出
5. 或等待15秒自动退出

#### 时间源切换
1. 长按K2键（超过500ms）进入时间源设置
2. 短按K2键在以下选项中切换：
   - **NTP**：网络时间（需要WiFi连接）
   - **RTC**：实时时钟（需要DS1307模块）
   - **CLK**：软件时钟（手动设置的时间）
3. 短按K4键确认并退出
4. 或等待15秒自动退出

#### WiFi重置
1. 长按K4键（超过500ms）重置WiFi配置
2. OLED显示"配网模式"
3. 等待3秒后进入后台配网
4. 按照首次使用的步骤重新配置WiFi

#### Web OTA更新
1. 长按K1键（超过5秒）启动Web OTA服务器
2. OLED显示"OTA模式"
3. 在手机/电脑上连接到设备WiFi
4. 打开浏览器访问：`http://<设备IP>/update`
5. 输入用户名和密码（默认：admin/admin123）
6. 选择固件文件并上传
7. 等待上传完成，设备自动重启

### 时间源说明

#### NTP（网络时间）
- 需要WiFi连接
- 时间最准确
- 支持多NTP服务器故障转移
- 每60秒自动同步一次

#### RTC（实时时钟）
- 需要DS1307模块
- 断电后仍可运行（需要纽扣电池）
- 每30分钟与NTP同步一次（如果NTP可用）

#### CLK（软件时钟）
- 手动设置的时间
- 不依赖外部设备
- 精度较低（依赖ESP8266内部时钟）

#### 自动切换逻辑
系统会自动选择最佳时间源，优先级：RTC > NTP > CLK

- 如果RTC可用且时间有效，优先使用RTC
- 如果RTC不可用，尝试使用NTP
- 如果NTP不可用，使用软件时钟
- 网络断开时自动切换到RTC或软件时钟

## 🐛 故障排除

### OLED不显示

#### 可能原因
1. 电源未连接
2. OLED接线错误
3. I2C地址错误
4. 固件未正确上传

#### 解决方法
1. 检查电源连接（3.3V或5V）
2. 检查I2C接线（SCL: D1/GPIO5, SDA: D2/GPIO4）
3. 使用I2C扫描工具检查OLED地址（默认0x3C）
4. 重新上传固件
5. 按复位按钮重启设备
6. 检查OLED是否损坏（更换OLED测试）

### WiFi连接失败

#### 可能原因
1. WiFi密码错误
2. 路由器不支持2.4GHz频段
3. WiFi信号太弱
4. 路由器设置了MAC地址过滤

#### 解决方法
1. 长按K4键重置WiFi配置
2. 重新配网时确认WiFi密码正确
3. 确保路由器支持2.4GHz频段（ESP8266不支持5GHz）
4. 将设备靠近路由器
5. 检查路由器设置，允许新设备连接
6. 查看串口日志了解详细错误信息

### 时间显示错误

#### 可能原因
1. 网络未连接
2. NTP服务器不可达
3. RTC模块故障
4. 时区设置错误

#### 解决方法
1. 检查网络连接状态（短按K4查看）
2. 尝试手动设置时间（长按K3）
3. 检查RTC模块连接
4. 重启设备
5. 检查时区设置（默认东八区）
6. 查看串口日志了解详细错误信息

### 按键无响应

#### 可能原因
1. 按键接线错误
2. 按键故障
3. 引脚冲突
4. 固件问题

#### 解决方法
1. 检查按键接线（K1: D3, K2: D5, K3: D6, K4: D7）
2. 使用万用表测试按键
3. 检查引脚是否被其他功能占用
4. 重新上传固件
5. 重启设备

### 设备频繁重启

#### 可能原因
1. 电源不稳定
2. 供电不足
3. 看门狗触发
4. 固件问题
5. 内存不足

#### 解决方法
1. 检查电源供应（建议使用5V 2A电源适配器）
2. 使用更短的USB线
3. 检查电源电压是否稳定
4. 重新上传固件
5. 查看串口日志了解详细错误信息
6. 检查内存使用情况

### WiFi密码加密失败

#### 可能原因
1. EEPROM数据损坏
2. AES加密失败
3. 密码长度过长

#### 解决方法
1. 长按K4键重置WiFi配置
2. 重新配网
3. 检查EEPROM状态
4. 查看串口日志了解详细错误信息

## 📊 性能指标

### 内存使用
- **RAM使用**：41,720 / 80,192 字节（52%）
- **Flash使用**：924,840 / 1,048,576 字节（88%）
- **IRAM使用**：61,439 / 65,536 字节（93%）

### 固件大小
- **编译后大小**：约965KB
- **压缩后大小**：约717KB
- **上传时间**：约64秒（921600波特率）

### 运行时性能
- **主循环时间**：< 50ms
- **显示更新间隔**：1000ms
- **NTP同步间隔**：60000ms
- **WiFi重连间隔**：15000ms

### 功耗
- **正常模式**：100-150mA
- **睡眠模式**：20-30mA
- **待机模式**：10-20mA

## 🔒 安全性

### 密码加密
- WiFi密码使用AES-128加密存储
- 加密密钥基于设备ID和多个随机源生成
- 支持AES和XOR两种加密方式（兼容旧版本）

### 输入验证
- 所有用户输入都有边界检查
- 防止缓冲区溢出
- 防止整数溢出

### 错误处理
- 完善的错误恢复机制
- 硬件看门狗防止系统卡死
- 软件看门狗监控主循环

### 网络安全
- Web OTA支持用户名密码认证
- WiFiManager使用HTTPS（可选）
- 不存储敏感信息

## 🧪 测试

### 单元测试
项目包含完整的单元测试框架，可以测试各个模块的功能：

```cpp
// EEPROM配置测试
#include "eeprom_config_test.cpp"

// 集成测试
#include "integration_tests.cpp"

// 测试套件
#include "test_suites.cpp"
```

### 运行测试
```bash
# 编译测试版本
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 \
  --build-property "compiler.c.extra_flags=-DTEST_MODE" \
  --build-property "compiler.cpp.extra_flags=-DTEST_MODE" \
  .
```

### 测试覆盖
- EEPROM读写测试
- 按键处理测试
- 时间管理测试
- 显示管理测试
- 网络连接测试
- 错误恢复测试

## 📝 开发指南

### 代码规范
- 使用4空格缩进
- 使用驼峰命名法（变量和函数）
- 使用大写命名法（常量）
- 添加详细的注释
- 遵循Google C++风格指南

### 添加新功能
1. 在对应的模块中添加代码
2. 更新头文件
3. 添加单元测试
4. 更新文档
5. 提交Pull Request

### 调试方法
1. 在 `production_config.h` 中启用调试日志：
   ```cpp
   #define ENABLE_DEBUG_LOGS true
   #define DEFAULT_LOG_LEVEL LOG_LEVEL_DEBUG
   ```
2. 使用Arduino IDE的串口监视器
3. 波特率设置为115200
4. 查看日志输出

### 性能优化
- 使用PROGMEM存储常量字符串
- 使用静态缓冲区避免频繁内存分配
- 使用非阻塞延时替代delay()
- 使用yield()让出CPU时间
- 使用硬件看门狗防止系统卡死

## 🤝 贡献

欢迎提交Issue和Pull Request！

### 贡献流程
1. Fork项目
2. 创建特性分支
3. 提交更改
4. 推送到分支
5. 创建Pull Request

### 代码审查
所有提交的代码都会经过严格的代码审查，确保代码质量。

## 📄 许可证

MIT License

Copyright (c) 2025 ESP8266 SSD1306 Clock Project

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## 👥 作者

ESP8266 SSD1306 Clock Project

## 🙏 致谢

- [U8g2库](https://github.com/olikraus/u8g2) by olkra
- [NTPClient库](https://github.com/arduino-libraries/NTPClient) by Fabrice Weinberg
- [RTClib库](https://github.com/adafruit/RTClib) by Adafruit
- [WiFiManager库](https://github.com/tzapu/WiFiManager) by tzapu
- [ESP8266 Arduino Core](https://github.com/esp8266/Arduino) by ESP8266 Community

## 📞 技术支持

如果您遇到问题或有建议，请：
1. 查看[用户手册](USER_MANUAL.md)
2. 搜索[Issues](https://github.com/chakoe/esp8266_ssd1306_Clock/issues)
3. 提交新的Issue
4. 联系技术支持

## 📅 更新日志

### v2.1 (2025-02-03)
- 新增亮度永久保存功能
- 使用EEPROM存储亮度设置
- 重启后自动恢复上次亮度设置
- 添加EEPROM数据校验机制
- 优化内存使用

### v2.0 (2025-12-23)
- 修复所有已知的16个bug
- 添加所有millis()溢出保护
- 添加所有数组边界检查
- 优化按键消抖逻辑
- 完善错误处理机制
- 添加生产环境配置
- 创建完整文档
- 添加Web OTA更新功能
- 添加运行时监控功能
- 添加错误恢复机制

### v1.0 (2025-12-20)
- 初始版本
- 基本时钟功能
- WiFi配网
- NTP时间同步

## 🌟 功能路线图

- [ ] 支持更多NTP服务器
- [ ] 支持自定义显示主题
- [ ] 支持温度传感器
- [ ] 支持湿度传感器
- [ ] 支持MQTT协议
- [ ] 支持Home Assistant集成
- [ ] 支持语音控制
- [ ] 支持触摸屏
- [ ] 支持WiFi Direct
- [ ] 支持蓝牙

---

感谢您使用ESP8266 SSD1306智能时钟！如果您觉得这个项目有用，请给个⭐️Star！
