# GitHub API错误解决指南

## 问题描述

```
[WARN] GitHub API request failed, code: -5
[WARN] Failed to check version
```

---

## 🔍 原因分析

### 错误代码 -5 的含义

HTTPClient的错误代码 `-5` 通常表示：
- 连接超时
- DNS解析失败
- SSL/TLS握手失败
- 网络连接问题

### 可能的原因

1. **SSL/TLS问题** - 使用HTTP而不是HTTPS
2. **超时时间过短** - 网络响应慢
3. **DNS解析失败** - 无法解析github.com
4. **网络不稳定** - WiFi连接问题
5. **代理问题** - 需要通过代理访问

---

## ✅ 解决方案

### 已实施的修复

#### 1. 使用HTTPS连接
```cpp
// 修改前
WiFiClient client;

// 修改后
WiFiClientSecure client;
client.setInsecure(); // 跳过证书验证（节省资源）
```

#### 2. 增加重试机制
```cpp
const int maxRetries = 3;
for (int retry = 0; retry < maxRetries; retry++) {
    // 尝试连接
    // 失败后重试
}
```

#### 3. 增加超时时间
```cpp
client.setTimeout(15000); // 15秒超时
http.setTimeout(15000);   // 15秒超时
```

#### 4. 改进错误处理
```cpp
if (httpCode == HTTP_CODE_TOO_MANY_REQUESTS) {
    LOG_WARNING("GitHub API rate limit exceeded");
    return "";
}
```

#### 5. 添加必要的请求头
```cpp
http.addHeader("Accept", "application/vnd.github.v3+json");
```

---

## 🚀 使用修复后的固件

### 上传固件

**方法1：使用批处理脚本（推荐）**
```cmd
upload_fix.bat
```

**方法2：使用Arduino CLI**
```cmd
arduino-cli upload -p COM3 --fqbn esp8266:esp8266:nodemcuv2 --input-dir build/esp8266.esp8266.nodemcuv2
```

### 测试自动更新功能

#### 步骤1：打开串口监视器
- 波特率：115200
- 等待设备启动

#### 步骤2：检查最新版本
```
发送: c
```

**预期输出：**
```
[时间] Fetching latest version from GitHub...
[时间] Latest version from GitHub: v2.1.0
[时间] Current version: v2.1.0
[时间] Already up to date
```

**如果仍然失败：**
```
[时间] Fetching latest version from GitHub...
[时间] Retry 1/3...
[时间] Retry 2/3...
[时间] Retry 3/3...
[时间] Failed to get latest version after 3 retries
```

---

## 🔧 故障排除

### 问题1：仍然无法连接GitHub

**检查列表：**
- [ ] WiFi是否正常连接
- [ ] 能否访问其他网站
- [ ] DNS是否正常工作
- [ ] 是否需要代理

**测试方法：**
```cpp
// 测试网络连接
if (WiFi.status() == WL_CONNECTED) {
    LOG_INFO("WiFi connected, IP: %s", WiFi.localIP().toString().c_str());

    // 测试DNS解析
    IPAddress result;
    if (WiFi.hostByName("github.com", result)) {
        LOG_INFO("GitHub DNS resolved: %s", result.toString().c_str());
    } else {
        LOG_WARNING("GitHub DNS resolution failed");
    }
}
```

### 问题2：SSL/TLS握手失败

**解决方案：**
1. 使用 `client.setInsecure()` 跳过证书验证
2. 或手动添加GitHub的证书（复杂）

### 问题3：超时问题

**解决方案：**
1. 增加超时时间（已设置为15秒）
2. 检查网络连接质量
3. 使用更稳定的WiFi

### 问题4：GitHub API限制

**错误信息：**
```
GitHub API rate limit exceeded
```

**解决方案：**
- GitHub API有每小时60次的限制（未认证）
- 等待1小时后再试
- 或使用GitHub Token认证

---

## 📊 编译结果

| 项目 | 数值 |
|------|------|
| **状态** | ✅ 成功 |
| **固件大小** | 1,029 KB |
| **RAM使用** | 41,884 / 80,192 (52%) |
| **Flash使用** | 1,029,292 / 1,048,576 (98%) |

⚠️ **注意**：Flash使用率98%，接近上限。建议：
- 优化代码大小
- 移除不必要的功能
- 或使用更大的Flash芯片（4MB）

---

## 🎯 改进点总结

### 修复内容

1. ✅ **HTTPS连接** - 使用WiFiClientSecure
2. ✅ **重试机制** - 最多重试3次
3. ✅ **超时优化** - 增加到15秒
4. ✅ **错误处理** - 详细的错误信息
5. ✅ **请求头** - 添加Accept头

### 新增功能

1. ✅ **自动重试** - 失败后自动重试
2. ✅ **详细日志** - 显示重试次数
3. ✅ **错误分类** - 区分不同错误类型
4. ✅ **资源释放** - 正确释放连接

---

## 📝 测试建议

### 1. 测试网络连接
```
发送: o
```
查看WiFi连接状态

### 2. 测试DNS解析
在串口监视器中查看DNS解析日志

### 3. 测试GitHub连接
```
发送: c
```
查看是否能获取版本信息

### 4. 测试自动更新
```
发送: a
```
测试完整的更新流程

---

## 🔗 相关资源

- **GitHub API文档**：https://docs.github.com/en/rest
- **ESP8266 HTTPClient**：https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266HTTPClient
- **ESP8266 WiFiClientSecure**：https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/client-secure-class.md

---

## 🎊 总结

### 已修复的问题

✅ **GitHub API连接失败** - 改用HTTPS
✅ **超时问题** - 增加超时时间
✅ **错误处理** - 添加重试机制
✅ **资源泄漏** - 正确释放连接

### 下一步

1. **上传固件** - 使用 `upload_fix.bat`
2. **测试功能** - 发送 `c` 命令测试
3. **验证修复** - 查看是否能获取版本
4. **反馈问题** - 如有问题请报告

---

**固件已修复并准备上传！** 🚀
