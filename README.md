# ESP8266 SSD1306 时钟项目

一个基于ESP8266和SSD1306 OLED显示屏的智能时钟项目，具有网络时间同步、RTC备份和丰富的用户交互功能。

## 功能特性

- **多时间源支持**：支持NTP网络时间、RTC硬件时钟、手动设置时间等多种时间源
- **智能时间源切换**：自动在不同时间源间切换，确保时间准确性
- **OLED显示**：使用SSD1306 128x64 OLED显示屏显示时间、日期和星期
- **中文字体支持**：支持中文显示，包括星期和市场日等
- **WiFi配置**：集成WiFiManager，支持一键配网
- **多种显示模式**：支持大字体/小字体显示，亮度调节
- **按键控制**：4个按键提供丰富的交互功能
- **RTC备份**：使用DS1307/DS3231实时时钟芯片，断电后仍能保持时间
- **加密存储**：WiFi密码采用AES加密存储
- **看门狗保护**：内置硬件和软件看门狗，提高系统稳定性

## 硬件要求

- **主控芯片**：ESP8266 (推荐使用D1 Mini开发板)
- **显示屏**：SSD1306 128x64 OLED (I2C接口)
- **实时时钟**：DS1307或DS3231 RTC模块
- **按键**：4个轻触开关 (K1-K4)
- **连接方式**：I2C总线连接OLED和RTC

### 引脚分配

| 模块 | 功能 | 引脚 |
|------|------|------|
| OLED/RTC | SDA | D2 (GPIO4) |
| OLED/RTC | SCL | D1 (GPIO5) |
| 按键 | K1 | D3 (GPIO0) |
| 按键 | K2 | D5 (GPIO14) |
| 按键 | K3 | D6 (GPIO12) |
| 按键 | K4 | D7 (GPIO13) |

## 软件依赖

本项目需要以下Arduino库：

- **ESP8266 Core for Arduino** (>= 2.4.0)
- **U8g2** - OLED图形库
- **NTPClient** - 网络时间协议客户端
- **RTClib** - 实时时钟库
- **WiFiManager** - WiFi配置管理器
- **Adafruit BusIO** - 总线接口库 (RTClib依赖)

## 安装与配置

### 环境准备

1. 安装Arduino IDE
2. 添加ESP8266开发板支持：
   - 打开文件 > 首选项
   - 在"附加开发板管理器网址"中添加：`http://arduino.esp8266.com/stable/package_esp8266com_index.json`
   - 打开工具 > 开发板 > 开发板管理器
   - 搜索并安装"ESP8266 by ESP8266 Community"

3. 安装依赖库：
   - 打开项目 > 加载库 > 库管理
   - 搜索并安装上述依赖库

### 项目编译与上传

使用提供的批处理脚本进行编译和上传：

```bash
# Windows下编译并上传
build_and_upload.bat
```

或者使用Arduino CLI：

```bash
# 编译
arduino-cli compile --fqbn esp8266:esp8266:d1_mini --build-property "build.extra_flags=-DPRODUCTION_MODE"

# 上传 (假设端口为COM3)
arduino-cli upload -p COM3 --fqbn esp8266:esp8266:d1_mini
```

## 使用说明

### 首次启动

1. 连接硬件并通电
2. 长按K4键进入WiFi配置模式
3. ESP8266将创建名为"ESP8266-Clock"的热点
4. 连接该热点并配置您的WiFi信息
5. 设备重启后会自动连接网络并同步时间

### 按键功能

#### 常规模式

- **K1**：无功能
- **K2**：短按切换字体大小
- **K3**：短按进入亮度设置模式
- **K4**：短按显示/隐藏网络状态信息

#### 长按功能

- **K2**：长按进入时间源设置模式
- **K3**：长按进入时间设置模式
- **K4**：长按重置WiFi配置（进入配网模式）

#### 设置模式

- **K1**：增加数值
- **K2**：减少数值
- **K3**：切换设置字段（年/月/日/时/分/秒）
- **K4**：确认并退出设置模式

#### 亮度设置模式

- **K1**：增加亮度等级
- **K2**：减少亮度等级
- **K3**：增加亮度等级（循环切换亮度）
- **K4**：确认并退出亮度设置模式

#### 时间源设置模式

- **K2**：选择下一个时间源
- **K4**：确认并退出时间源设置模式

## 配置选项

### 时间设置

项目支持多种时间源，按优先级排序：
1. NTP网络时间 (最高优先级)
2. RTC硬件时钟
3. 手动设置时间
4. 无时间源 (最低优先级)

### NTP服务器

默认使用以下NTP服务器（按优先级）：
- `ntp1.aliyun.com`
- `cn.pool.ntp.org`
- `time.nist.gov`

### 显示设置

- **亮度等级**：支持多个亮度级别，可根据环境调节
- **字体大小**：支持大字体和小字体显示模式
- **更新频率**：显示刷新间隔可配置

## 故障排除

### 常见问题

1. **无法连接WiFi**
   - 检查WiFi密码是否正确
   - 长按K4进入配网模式重新配置
   - 确认WiFi信号强度

2. **时间不准确**
   - 检查网络连接状态
   - 确认RTC模块连接正常
   - 手动设置时间或切换时间源

3. **OLED无显示**
   - 检查I2C连接是否正确
   - 确认SDA/SCL引脚连接
   - 检查OLED模块是否损坏

4. **RTC不工作**
   - 检查RTC模块I2C地址
   - 确认电池是否安装且有电
   - 验证I2C总线连接

### 日志调试

启用调试模式可在串口监视器查看详细日志信息：
- 波特率：115200
- 信息包括：WiFi状态、NTP同步、RTC状态、按键事件等

## 开发信息

### 项目结构

```
esp8266_ssd1306_Clock/
├── esp8266_ssd1306_Clock.ino    # 主程序入口
├── config.h                     # 项目配置和常量定义
├── global_config.h/cpp          # 全局变量和配置
├── button_handler.h/cpp         # 按键处理逻辑
├── display_manager.h/cpp        # 显示管理
├── time_manager.h/cpp           # 时间管理
├── system_manager.h/cpp         # 系统管理
├── utils.h/cpp                  # 工具函数
├── logger.h/cpp                 # 日志系统
├── i2c_manager.h/cpp            # I2C管理
├── power_manager.h/cpp          # 电源管理
├── monitoring_system.h          # 系统监控
├── build_and_upload.bat         # Windows编译上传脚本
├── compile_to_bin.bat           # 编译为固件脚本
├── upload.bat                   # 上传脚本
└── libraries/                   # 依赖库
```

### 编译选项

- `PRODUCTION_MODE`：生产模式，启用优化和安全功能
- `DEBUG_MODE`：调试模式，启用详细日志输出

### 安全特性

- WiFi密码AES加密存储
- 硬件看门狗防死锁
- 错误处理和恢复机制
- 网络连接自动重连

## 许可证

本项目使用MIT许可证，详见LICENSE文件。

## 贡献

欢迎提交Issue和Pull Request来改进项目。