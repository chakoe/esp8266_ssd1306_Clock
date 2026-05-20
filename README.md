# ESP8266 SSD1306 智能时钟

一个基于ESP8266的智能时钟项目，集成OLED显示、RTC时钟、WiFi同步、圩日计算等功能。

## 📋 目录

- [项目简介](#项目简介)
- [主要功能](#主要功能)
- [硬件要求](#硬件要求)
- [引脚接线](#引脚接线)
- [按键功能](#按键功能)
- [软件功能](#软件功能)
- [安装使用](#安装使用)
- [配置说明](#配置说明)
- [版本历史](#版本历史)
- [许可证](#许可证)

## 🎯 项目简介

本项目是一个功能丰富的智能时钟，专为农村集市（圩日）设计。除了基本的时间显示功能外，还集成了圩日计算、WiFi时间同步、OTA升级等实用功能。

### 核心特性

- **多时间源支持**：RTC、NTP、手动设置三种时间源自动切换
- **圩日计算**：基于基准日期的3天周期循环计算（太守→新桥→芦圩）
- **WiFi管理**：支持AP配置模式，自动连接和重连
- **OTA升级**：支持Web界面在线升级固件
- **系统监控**：看门狗监控、错误恢复、运行时监控

## ⚡ 主要功能

### 1. 时间显示
- 大字体时间显示（时:分:秒）
- 日期显示（年-月-日）
- 星期显示（中文）
- 圩日显示（太守/新桥/芦圩）
- 时间源图标显示

### 2. 圩日计算 ⭐
- **基准日期**：2000-01-01 对应 太守
- **计算公式**：`marketIndex = ((daysDiff % 3) + 3) % 3`
- **周期循环**：每3天循环一次
  - 第0天：太守
  - 第1天：新桥
  - 第2天：芦圩

### 3. WiFi功能
- AP配置模式（首次使用）
- 自动连接已配置的WiFi
- NTP时间同步
- Web OTA升级界面

### 4. 系统管理
- 多级亮度调节
- 时间源切换
- 系统状态监控
- 错误自动恢复

## 🔧 硬件要求

### 核心组件

| 组件 | 型号/规格 | 数量 | 说明 |
|------|-----------|------|------|
| **MCU** | ESP8266EX | 1 | 主控制器，4MB Flash |
| **显示屏** | SSD1306 OLED | 1 | 128x64分辨率，I2C接口 |
| **RTC模块** | DS1307 | 1 | 实时时钟模块，I2C接口 |
| **按键** | 轻触按键 | 4 | 功能按键 |

### 可选组件

| 组件 | 说明 |
|------|------|
| **USB转TTL** | 用于固件上传和调试 |
| **面包板/PCB** | 电路连接 |

## 🔌 引脚接线

### 详细接线表

#### 1. SSD1306 OLED 显示屏（I2C）

| OLED引脚 | ESP8266引脚 | GPIO | 说明 |
|----------|-------------|------|------|
| VCC | 3.3V | - | 电源（3.3V）|
| GND | GND | - | 地线 |
| SCL | D1 | GPIO5 | I2C时钟线 |
| SDA | D2 | GPIO4 | I2C数据线 |

#### 2. DS1307 RTC模块（I2C）

| RTC引脚 | ESP8266引脚 | GPIO | 说明 |
|---------|-------------|------|------|
| VCC | 5V/3.3V | - | 电源 |
| GND | GND | - | 地线 |
| SCL | D1 | GPIO5 | I2C时钟线（与OLED共用）|
| SDA | D2 | GPIO4 | I2C数据线（与OLED共用）|

#### 3. 按键连接

| 按键 | GPIO | ESP8266引脚 
|----- |-------|------|
| K1 | GPIO0  | D3 | 
| K2 | GPIO14 | D5 | 
| K3 | GPIO12 | D6 | 
| K4 | GPIO13 | D7 | 

#### 4. USB转TTL（用于固件上传）

| TTL引脚 | ESP8266引脚 | 说明 |
|---------|-------------|------|
| TX | RX | 串口发送 |
| RX | TX | 串口接收 |
| GND | GND | 地线 |
| 5V/3.3V | - | 电源（可选）|

### I2C设备地址

| 设备 | I2C地址 | 说明 |
|------|---------|------|
| SSD1306 OLED | 0x3C | 默认地址 |
| DS1307 RTC | 0x68 | 默认地址 |

## 🎮 按键功能

### 操作说明

#### 1. 正常显示模式

| 按键 | 名称 | 短按功能 | 长按功能 |
|------|------|----------|----------|
| **K1** | 短按| 无 | 上/增加键 | 长按进入Web OTA升级
| **K2** | 短按| 切换字体大小 | 下/减少键
| **K3** | 短按| 切换亮度 | 长按设置时间
| **K4** | 短按| 显示/隐藏网络 | 确认键 | 长按重置WIFI配置


#### 2. 设置模式

进入设置模式后，可以设置以下参数：

| 设置项 | 操作说明 |
|--------|----------|
| **年份** | K1增加，K2减少，K3切换，K4确认 |
| **月份** | K1增加，K2减少，K3切换，K4确认 |
| **日期** | K1增加，K2减少，K3切换，K4确认 |
| **小时** | K1增加，K2减少，K3切换，K4确认 |
| **分钟** | K1增加，K2减少，K3切换，K4确认 |
| **秒数** | K1增加，K2减少，K3切换，K4确认 |

#### 3. 亮度调节

在设置模式中，可以调节屏幕亮度：

| 亮度级别 | 说明 |
|----------|------|
| 1 | 较暗 |
| 2 | 中等 |
| 3 | 较亮 |
| 4 | 最亮 |

#### 4. 时间源切换

长按K4可以切换时间源：

| 时间源 | 图标 | 说明 |
|--------|------|------|
| RTC | 🕐 | 实时时钟模块 |
| NTP | 🌐 | 网络时间同步 |
| 手动 | ⚙️ | 手动设置时间 |

### 按键防抖和长按检测

- **防抖时间**：50ms
- **长按判定时间**：500ms
- **状态重置时间**：2000ms

## 💻 软件功能

### 1. 时间管理系统

- **多时间源支持**
  - RTC：硬件实时时钟，断电保持
  - NTP：网络时间协议，自动同步
  - 手动：用户手动设置

- **自动切换逻辑**
  - 优先使用NTP时间
  - NTP失败时回退到RTC
  - RTC失败时使用手动设置

### 2. WiFi管理系统

- **AP配置模式**
  - 首次使用自动进入
  - 创建热点：Clck_AP_XXXXXX
  - 密码：无
  - 配置页面：192.168.4.1

- **自动连接**
  - 保存WiFi配置
  - 自动重连机制
  - 连接失败处理

### 3. 圩日计算系统

**计算原理：**
```
基准日期：2000-01-01（太守）
周期：3天
公式：marketIndex = ((daysDiff % 3) + 3) % 3

结果映射：
- 0：太守
- 1：新桥
- 2：芦圩
```
### 4. OTA升级系统

- **Web界面升级**
  - 访问：http://[设备IP]/update
  - 上传新的固件文件（.bin）
  - 自动重启应用新固件

### 5. 系统监控

- **看门狗监控**
  - 30秒检查周期
  - 自动恢复机制

- **错误处理**
  - 多级错误分类
  - 自动恢复策略
  - 错误日志记录

## 🚀 安装使用

### 1. 环境准备

**所需工具：**
- Arduino IDE 或 Arduino CLI
- ESP8266 Arduino Core（3.1.2或更高）
- USB转TTL模块

**所需库：**
- RTClib（RTC库）
- U8g2（OLED显示库）
- NTPClient（NTP客户端）
- WiFiManager（WiFi配置）
- ArduinoJson（JSON解析）

### 2. 编译固件

**使用Arduino CLI：**
```bash
# 编译固件
arduino-cli compile --fqbn esp8266:esp8266:generic --export-binaries --warnings none

# 输出文件位置
build/esp8266.esp8266.generic/esp8266_ssd1306_Clock.ino.bin
```

**使用Arduino IDE：**
1. 打开 `esp8266_ssd1306_Clock.ino`
2. 选择开发板：Generic ESP8266 Module
3. 点击"上传"

### 3. 上传固件

**使用Arduino CLI：**
```bash
# 上传固件
arduino-cli upload -p COM3 --fqbn esp8266:esp8266:generic --input-dir build/esp8266.esp8266.generic
```

**使用esptool：**
```bash
esptool.py --port COM3 write_flash 0x00000 esp8266_ssd1306_Clock.ino.bin
```

### 4. 首次配置

1. 上电后，设备进入AP配置模式
2. 连接WiFi热点：Clck_AP_XXXXXX
3. 密码：无
4. 浏览器访问：192.168.4.1
5. 配置WiFi参数并保存
6. 设备自动重启并连接WiFi

## ⚙️ 配置说明

### 系统配置（config.h）

```cpp
// 引脚定义
#define K1_PIN 0   // GPIO0 (D3)
#define K2_PIN 14  // GPIO14 (D5)
#define K3_PIN 12  // GPIO12 (D6)
#define K4_PIN 13  // GPIO13 (D7)

// 按键参数
const unsigned long DEBOUNCE_DELAY = 50;      // 防抖时间(ms)
const unsigned long LONG_PRESS_TIME = 500;    // 长按时间(ms)

// 系统参数
const unsigned long WATCHDOG_INTERVAL = 30000;     // 看门狗周期(ms)
const unsigned long NTP_SYNC_INTERVAL = 60000;     // NTP同步间隔(ms) - 60秒
const unsigned long RTC_SYNC_INTERVAL = 1800000;   // RTC同步间隔(ms) - 30分钟
const unsigned long TIME_SOURCE_SWITCH_DELAY = 3000; // 时间源切换延迟(ms) - 3秒
```

### 时间管理配置说明

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `NTP_SYNC_INTERVAL` | 60000ms (60秒) | NTP时间同步检查间隔 |
| `RTC_SYNC_INTERVAL` | 1800000ms (30分钟) | RTC自动同步NTP时间间隔 |
| `TIME_SOURCE_SWITCH_DELAY` | 3000ms (3秒) | 时间源切换后延迟检查时间 |

**配置建议**:
- NTP同步间隔: 建议60-300秒，避免频繁请求被限流
- RTC同步间隔: 建议30-60分钟，平衡准确性和网络消耗
- 切换延迟: 建议3-5秒，确保时间源初始化完成

### 圩日配置（global_config.h）

```cpp
struct MarketConfig {
    static constexpr int BASE_YEAR = 2000;   // 基准年份
    static constexpr int BASE_MONTH = 1;     // 基准月份
    static constexpr int BASE_DAY = 1;       // 基准日期
    static constexpr int CYCLE_DAYS = 3;     // 周期天数
};
```

### NTP服务器配置

默认NTP服务器列表：
- pool.ntp.org
- time.nist.gov
- asia.pool.ntp.org

## 📝 版本历史

### v2.3.0 (2026-05-19) - UI界面美化与优化
- 🎨 **复古风格UI重构**: 采用三段式布局，增加装饰性线条和边框
  - 顶部：日期居中显示，两侧增加对称装饰线
  - 中部：大字号时间显示，上下增加分隔线聚焦视觉中心
  - 底部：圩日与星期分列左右，底部增加贯穿底线
- 🛠️ **设置界面优化**: 增加圆角矩形外框，优化高亮反馈（加粗双平行线）
- 💡 **亮度调节拟物化**: 进度条增加刻度设计，模拟物理滑块质感
- ⚡ **显示稳定性提升**: 修复非等宽字体导致的时间文字跳动问题，采用固定坐标对齐
- 🔧 **布局微调**: 优化分隔线位置与宽度，确保信息层级清晰且不重叠
- 💾 **配置持久化增强**: 实现字体大小状态的断电保存功能
  - 扩展EEPROM存储结构，支持多配置项同步更新
  - 增加CRC8校验逻辑，确保存储数据的完整性与可靠性

### v2.2.0 (2026-05-17) - 时间准确性修复
- 🐛 **修复NTP更新间隔**: 将硬编码5秒改为配置的60秒间隔
  - 降低NTP服务器压力，提升时间获取成功率
  - 减少网络资源浪费（请求频率降低83%）
- ✨ **新增RTC定期同步**: 每30分钟自动同步NTP时间到RTC
  - 消除RTC长期漂移，保持时间准确性
  - 非阻塞设计，不影响主循环性能
- 🐛 **优化时间源切换**: 添加切换后3秒延迟检查机制
  - 提高时间源切换成功率
  - 避免频繁切换导致的不稳定
- 📝 **配置参数更新**: 新增时间源切换延迟常量
- ✅ **修复验证**: 所有修改遵循"最小侵入、保持兼容"原则

**修复详情**:
- 修改文件: `esp8266_ssd1306_Clock.ino`（主循环优化）、`config.h`（新增常量）
- 核心改进: NTP间隔优化、RTC自动同步、时间源切换优化
- 预期效果: 时间准确性显著提升，长期运行稳定可靠

### v2.1.0 (2025-02-03)
- ✅ 优化圩日计算公式，使用基准日期2000-01-01
- ✅ 简化计算逻辑，移除偏移量缓存机制
- ✅ 修复DateTime类的拷贝构造函数警告
- ✅ 修复编译警告，提升代码质量
- ✅ 验证计算准确性，确保功能正常

### v2.0.0 (2025-01-15)
- 🎉 重构代码架构，模块化设计
- ✨ 添加OTA升级功能
- 🐛 修复多个稳定性问题
- 📝 完善文档和注释

### v1.0.0 (2024-12-01)
- 🎉 初始版本发布
- ✨ 基本时间显示功能
- ✨ 圩日计算功能
- ✨ WiFi配置和NTP同步

## 🤝 贡献指南

欢迎提交Issue和Pull Request！

1. Fork本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 创建Pull Request

## 📄 许可证

本项目采用 [GNU General Public License v2](LICENSE) 许可证。

```
Copyright (C) 2024-2025 ESP8266 SSD1306 Clock Project

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
```

## 📞 联系方式

- **项目主页**：[GitHub Repository]
- **问题反馈**：[Issues]
- **功能建议**：[Feature Requests]

## 🙏 致谢

感谢以下开源项目：
- [Arduino ESP8266](https://github.com/esp8266/Arduino)
- [U8g2](https://github.com/olikraus/u8g2)
- [RTClib](https://github.com/adafruit/RTClib)
- [NTPClient](https://github.com/arduino-libraries/NTPClient)

---

**Made with ❤️ for rural market communities**
