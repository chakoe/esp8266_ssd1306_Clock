# GitHub仓库设置指南

## 🚨 问题说明

当前错误：
```
[WARN] GitHub repository not found
[WARN] Failed to check version
```

**原因**：GitHub仓库 `chakoe/esp8266_ssd1306_Clock` 不存在或无法访问。

---

## 🔍 问题诊断

### 检查仓库是否存在

在浏览器中访问：
```
https://github.com/chakoe/esp8266_ssd1306_Clock
```

如果看到404页面，说明仓库不存在。

### 检查API访问

在浏览器中访问：
```
https://api.github.com/repos/chakoe/esp8266_ssd1306_Clock/releases/latest
```

如果看到：
```json
{
  "message": "Not Found",
  "documentation_url": "https://docs.github.com/rest"
}
```

说明仓库不存在。

---

## ✅ 解决方案

### 方案1：创建GitHub仓库（推荐）

#### 步骤1：创建仓库

1. 访问 https://github.com/new
2. 填写仓库信息：
   - **Repository name**: `esp8266_ssd1306_Clock`
   - **Description**: `ESP8266 SSD1306 智能时钟项目`
   - **Public**: ✅（必须公开）
   - **Initialize with README**: 可选

3. 点击"Create repository"

#### 步骤2：上传代码

```bash
# 初始化git仓库
cd c:/Users/Administrator/esp8266_ssd1306_Clock
git init

# 添加所有文件
git add .

# 提交
git commit -m "Initial commit - ESP8266 SSD1306 Clock with OTA support"

# 添加远程仓库
git remote add origin https://github.com/chakoe/esp8266_ssd1306_Clock.git

# 推送到GitHub
git branch -M main
git push -u origin main
```

#### 步骤3：创建Release

1. 访问仓库页面
2. 点击"Releases" → "Create a new release"
3. 填写信息：
   - **Tag**: `v2.1.0`
   - **Release title**: `Release v2.1.0`
   - **Description**: `ESP8266 SSD1306 Clock - Initial release with OTA support`
4. 上传固件文件：
   - 选择 `build/esp8266.esp8266.nodemcuv2/esp8266_ssd1306_Clock.ino.bin`
5. 点击"Publish release"

#### 步骤4：验证

访问：
```
https://api.github.com/repos/chakoe/esp8266_ssd1306_Clock/releases/latest
```

应该看到类似：
```json
{
  "tag_name": "v2.1.0",
  "name": "Release v2.1.0",
  "html_url": "https://github.com/chakoe/esp8266_ssd1306_Clock/releases/tag/v2.1.0"
}
```

---

### 方案2：修改为现有仓库

如果已有其他仓库，修改代码中的仓库地址：

#### 步骤1：找到现有仓库

确认您有一个公开的GitHub仓库。

#### 步骤2：修改代码

在 `ota_manager.cpp` 中修改：

```cpp
// 修改前
String url = "https://api.github.com/repos/chakoe/esp8266_ssd1306_Clock/releases/latest";

// 修改为您的仓库
String url = "https://api.github.com/repos/您的用户名/您的仓库名/releases/latest";
```

同时修改 `buildFirmwareUrl()` 函数：

```cpp
String buildFirmwareUrl(const char* version) {
    String url = "https://github.com/您的用户名/您的仓库名/releases/download/";
    url += version;
    url += "/esp8266_ssd1306_Clock.ino.bin";
    return url;
}
```

#### 步骤3：重新编译和上传

```bash
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 --export-binaries
arduino-cli upload -p COM3 --fqbn esp8266:esp8266:nodemcuv2 --input-dir build/esp8266.esp8266.nodemcuv2
```

---

### 方案3：使用本地服务器（临时方案）

如果不想使用GitHub，可以使用本地HTTP服务器：

#### 步骤1：启动本地服务器

```bash
# 使用Python
cd c:/Users/Administrator/esp8266_ssd1306_Clock/build/esp8266.esp8266.nodemcuv2
python -m http.server 8000
```

#### 步骤2：修改固件URL

通过串口发送 `v` 命令，然后输入：
```
http://192.168.1.100:8000/esp8266_ssd1306_Clock.ino.bin
```

（替换为您的电脑IP地址）

#### 步骤3：触发更新

发送 `u` 命令进行更新。

---

## 📋 完整设置检查清单

### GitHub仓库设置

- [ ] 仓库已创建
- [ ] 仓库是公开的（Public）
- [ ] 仓库名称正确：`esp8266_ssd1306_Clock`
- [ ] 代码已上传到仓库
- [ ] Release已创建
- [ ] 固件文件已上传到Release
- [ ] Release tag正确：`v2.1.0`

### 设备配置

- [ ] 设备已连接到WiFi
- [ ] 设备可以访问外网
- [ ] DNS解析正常
- [ ] 固件已上传到设备

---

## 🔧 快速测试

### 测试GitHub API访问

在设备串口监视器中发送：
```
c
```

**成功输出：**
```
[时间] Fetching latest version from GitHub...
[时间] Repository: chakoe/esp8266_ssd1306_Clock
[时间] Latest version from GitHub: v2.1.0
[时间] Current version: v2.1.0
[时间] Already up to date
```

**失败输出（仓库不存在）：**
```
[时间] Fetching latest version from GitHub...
[时间] Repository: chakoe/esp8266_ssd1306_Clock
========================================
  GitHub Repository Not Found
========================================
Repository: chakoe/esp8266_ssd1306_Clock

Possible reasons:
1. Repository does not exist
2. Repository name is incorrect
3. Repository is private

Solutions:
1. Create the repository on GitHub
2. Update the repository name in code
3. Make the repository public
4. Create a Release with a tag

For now, OTA update is disabled.
You can still use manual update with 'u' command.
========================================
```

---

## 🎯 推荐操作流程

### 完整设置流程

1. **创建GitHub仓库**
   ```
   访问 https://github.com/new
   创建公开仓库：esp8266_ssd1306_Clock
   ```

2. **上传代码**
   ```bash
   git init
   git add .
   git commit -m "Initial commit"
   git remote add origin https://github.com/chakoe/esp8266_ssd1306_Clock.git
   git push -u origin main
   ```

3. **创建Release**
   ```
   访问仓库页面
   点击 Releases → Create a new release
   Tag: v2.1.0
   上传固件文件
   Publish release
   ```

4. **测试OTA功能**
   ```
   打开串口监视器
   发送: c
   查看是否成功获取版本
   ```

5. **验证更新**
   ```
   发布新版本 v2.2.0
   发送: a
   验证自动更新
   ```

---

## 📝 临时解决方案

在GitHub仓库设置完成之前，您仍然可以使用手动OTA更新：

### 手动更新步骤

1. **编译固件**
   ```bash
   arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 --export-binaries
   ```

2. **上传到服务器**
   - 上传到任意HTTP服务器
   - 或使用本地Python服务器

3. **配置URL**
   ```
   发送: v
   输入: http://your-server.com/firmware.bin
   ```

4. **触发更新**
   ```
   发送: u
   ```

---

## 🔗 相关资源

- **GitHub仓库创建**：https://github.com/new
- **GitHub Releases文档**：https://docs.github.com/en/repositories/releasing-projects-on-github
- **GitHub API文档**：https://docs.github.com/en/rest
- **Git基础教程**：https://git-scm.com/docs/gittutorial

---

## 🎊 总结

### 问题原因

❌ GitHub仓库 `chakoe/esp8266_ssd1306_Clock` 不存在

### 解决方案

✅ **方案1**：创建GitHub仓库（推荐）
✅ **方案2**：修改为现有仓库
✅ **方案3**：使用本地服务器（临时）

### 下一步

1. 创建GitHub仓库
2. 上传代码和固件
3. 创建Release
4. 测试OTA功能

---

**请按照上述步骤设置GitHub仓库，然后OTA功能就可以正常工作了！** 🚀
