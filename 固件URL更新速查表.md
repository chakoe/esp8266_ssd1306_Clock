# 固件URL更新速查表

## 🎯 当前配置

### 固件URL
```
https://github.com/chakoe/esp8266_ssd1306_Clock/releases/download/Releasev2.1.0/esp8266_ssd1306_Clock.ino.bin
```

### CDN URL（推荐）
```
https://cdn.jsdelivr.net/gh/chakoe/esp8266_ssd1306_Clock@Releasev2.1.0/esp8266_ssd1306_Clock.ino.bin
```

---

## 📋 固件地址变化处理

### 场景1：发布新版本（如 v2.2.0）

#### 方法A：通过串口命令更新
```
1. 打开串口监视器（波特率115200）
2. 发送 'v' 进入URL配置模式
3. 发送新URL：
   https://github.com/chakoe/esp8266_ssd1306_Clock/releases/download/Releasev2.2.0/esp8266_ssd1306_Clock.ino.bin
4. 发送 'u' 触发OTA更新
```

#### 方法B：使用CDN URL（推荐）
```
1. 打开串口监视器
2. 发送 'v' 进入URL配置模式
3. 发送CDN URL：
   https://cdn.jsdelivr.net/gh/chakoe/esp8266_ssd1306_Clock@Releasev2.2.0/esp8266_ssd1306_Clock.ino.bin
4. 发送 'u' 触发OTA更新
```

#### 方法C：使用Python脚本批量更新
```bash
# 更新单个设备
python update_firmware_url.py COM3 v2.2.0

# 更新多个设备
python update_firmware_url.py COM3 COM4 COM5 v2.2.0

# 交互模式
python update_firmware_url.py
```

---

## 🔧 常用版本URL

### GitHub Releases URL
| 版本 | URL |
|------|-----|
| v2.1.0 | `https://github.com/chakoe/esp8266_ssd1306_Clock/releases/download/Releasev2.1.0/esp8266_ssd1306_Clock.ino.bin` |
| v2.2.0 | `https://github.com/chakoe/esp8266_ssd1306_Clock/releases/download/Releasev2.2.0/esp8266_ssd1306_Clock.ino.bin` |
| v2.3.0 | `https://github.com/chakoe/esp8266_ssd1306_Clock/releases/download/Releasev2.3.0/esp8266_ssd1306_Clock.ino.bin` |

### CDN URL（推荐）
| 版本 | URL |
|------|-----|
| v2.1.0 | `https://cdn.jsdelivr.net/gh/chakoe/esp8266_ssd1306_Clock@Releasev2.1.0/esp8266_ssd1306_Clock.ino.bin` |
| v2.2.0 | `https://cdn.jsdelivr.net/gh/chakoe/esp8266_ssd1306_Clock@Releasev2.2.0/esp8266_ssd1306_Clock.ino.bin` |
| v2.3.0 | `https://cdn.jsdelivr.net/gh/chakoe/esp8266_ssd1306_Clock@Releasev2.3.0/esp8266_ssd1306_Clock.ino.bin` |

---

## 📱 串口命令速查

| 命令 | 功能 | 示例 |
|------|------|------|
| `o` | 显示OTA状态 | 查看当前OTA状态和进度 |
| `u` | 触发OTA更新 | 从配置的URL下载并更新 |
| `v` | 设置固件URL | 配置新的固件下载地址 |
| `c` | 检查最新版本 | 检测GitHub上的最新版本（需要实现） |

---

## 🔄 URL更新流程

### 单设备更新
```
串口连接 → 发送 'v' → 输入新URL → 发送 'u' → 等待更新 → 重启
```

### 批量更新
```
运行脚本 → 选择设备 → 选择版本 → 自动更新 → 完成
```

---

## ⚠️ 注意事项

### 更新前检查
- ✅ 确保设备连接到WiFi
- ✅ 确认新URL可以访问
- ✅ 备份当前固件
- ✅ 准备回滚方案

### 更新过程中
- ⏱️ 不要断开WiFi连接
- ⏱️ 不要断开设备电源
- ⏱️ 等待更新完成
- ⏱️ 设备会自动重启

### 更新失败处理
1. 检查串口日志
2. 验证URL是否正确
3. 确认网络连接
4. 使用串口上传恢复

---

## 🛠️ 故障排除

### 问题1：URL更新失败
**原因**：URL格式错误或过长
**解决**：使用CDN URL（更短、更快）

### 问题2：OTA更新失败
**原因**：网络问题或固件不存在
**解决**：
1. 检查网络连接
2. 验证URL是否正确
3. 确认固件文件已上传

### 问题3：更新后设备不启动
**原因**：固件不兼容
**解决**：
1. 使用串口上传恢复固件
2. 检查固件版本兼容性

### 问题4：批量更新部分失败
**原因**：部分设备离线或串口被占用
**解决**：
1. 检查设备连接状态
2. 关闭其他串口程序
3. 逐个更新失败的设备

---

## 📊 URL格式对比

| URL类型 | 长度 | 速度 | 可靠性 | 推荐度 |
|---------|------|------|--------|--------|
| GitHub原生 | 长 | 慢 | 高 | ⭐⭐⭐ |
| CDN加速 | 中 | 快 | 高 | ⭐⭐⭐⭐⭐ |
| 自定义服务器 | 短 | 最快 | 中 | ⭐⭐⭐⭐ |

---

## 🎯 最佳实践

### 1. 使用CDN URL
- ✅ 下载速度更快
- ✅ 全球节点分布
- ✅ 自动缓存
- ✅ 免费使用

### 2. 统一文件命名
```
始终使用: esp8266_ssd1306_Clock.ino.bin
只改变版本标签: Releasev2.1.0, Releasev2.2.0
```

### 3. 版本管理
```
使用语义化版本: v2.1.0
主版本.次版本.修订版本
```

### 4. 批量更新
```
使用Python脚本批量更新多个设备
减少手动操作错误
```

---

## 🔗 相关资源

- **完整指南**：`GitHub_Releases配置指南.md`
- **更新工具**：`update_firmware_url.py`
- **OTA文档**：`OTA使用指南.md`
- **快速开始**：`OTA快速开始.md`

---

## 💡 快速命令

### 查看当前OTA状态
```
发送: o
```

### 更新到v2.2.0（CDN）
```
发送: v
发送: https://cdn.jsdelivr.net/gh/chakoe/esp8266_ssd1306_Clock@Releasev2.2.0/esp8266_ssd1306_Clock.ino.bin
发送: u
```

### 批量更新多个设备
```bash
python update_firmware_url.py COM3 COM4 COM5 v2.2.0
```

---

## 📝 总结

### 关键要点
1. **使用CDN URL** - 速度快、稳定
2. **统一文件命名** - 避免URL变化
3. **批量更新脚本** - 提高效率
4. **版本标签管理** - 清晰的版本控制

### 处理地址变化的步骤
1. 发布新版本到GitHub
2. 更新版本标签（保持文件名不变）
3. 使用CDN URL更新设备配置
4. 触发OTA更新
5. 验证更新成功

---

**随时可用的CDN URL模板：**
```
https://cdn.jsdelivr.net/gh/chakoe/esp8266_ssd1306_Clock@ReleasevX.Y.Z/esp8266_ssd1306_Clock.ino.bin
```

只需替换 `X.Y.Z` 为实际版本号即可！🚀
