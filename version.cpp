#include "version.h"
#include "logger.h"

// =============================================================================
// 全局版本信息
// =============================================================================

static VersionInfo versionInfo = {
  FIRMWARE_VERSION_MAJOR,
  FIRMWARE_VERSION_MINOR,
  FIRMWARE_VERSION_PATCH,
  FIRMWARE_VERSION_DATE,
  BUILD_TIME,
  GIT_BRANCH,
  GIT_COMMIT
};

// 版本字符串缓存
static char versionString[32] = {0};
static char fullVersionInfo[128] = {0};
static char buildInfo[128] = {0};

// =============================================================================
// 版本管理函数实现
// =============================================================================

/**
 * @brief 初始化版本管理模块
 */
void initVersionManager() {
  // 生成版本字符串
  snprintf(versionString, sizeof(versionString),
           "v%d.%d.%d",
           versionInfo.major,
           versionInfo.minor,
           versionInfo.patch);

  // 生成完整版本信息
  snprintf(fullVersionInfo, sizeof(fullVersionInfo),
           "Firmware: %s | Date: %s | Build: %s",
           versionString,
           versionInfo.date,
           versionInfo.buildTime);

  // 生成构建信息
  snprintf(buildInfo, sizeof(buildInfo),
           "Build Date: %s %s | Branch: %s | Commit: %s",
           BUILD_DATE,
           versionInfo.buildTime,
           versionInfo.gitBranch,
           versionInfo.gitCommit);

  LOG_INFO("Version Manager initialized");
  LOG_INFO("Firmware version: %s", versionString);
}

/**
 * @brief 获取版本信息结构体
 * @return VersionInfo 版本信息结构体
 */
VersionInfo getVersionInfo() {
  return versionInfo;
}

/**
 * @brief 获取版本字符串
 * @return const char* 版本字符串(如 "v2.1.0")
 */
const char* getVersionString() {
  return versionString;
}

/**
 * @brief 获取完整版本信息字符串
 * @return const char* 完整版本信息(包含日期、编译时间等)
 */
const char* getFullVersionInfo() {
  return fullVersionInfo;
}

/**
 * @brief 获取版本号作为整数(格式: major*10000 + minor*100 + patch)
 * @return uint32_t 版本号整数
 */
uint32_t getVersionNumber() {
  return (uint32_t)versionInfo.major * 10000 +
         (uint32_t)versionInfo.minor * 100 +
         (uint32_t)versionInfo.patch;
}

/**
 * @brief 比较两个版本号
 * @param v1 第一个版本号
 * @param v2 第二个版本号
 * @return int 负数表示v1<v2, 0表示相等, 正数表示v1>v2
 */
int compareVersions(uint32_t v1, uint32_t v2) {
  if (v1 < v2) {
    return -1;
  } else if (v1 > v2) {
    return 1;
  } else {
    return 0;
  }
}

/**
 * @brief 检查版本是否兼容
 * @param targetVersion 目标版本号
 * @return true 如果当前版本 >= 目标版本
 * @return false 如果当前版本 < 目标版本
 */
bool isVersionCompatible(uint32_t targetVersion) {
  uint32_t currentVersion = getVersionNumber();
  return compareVersions(currentVersion, targetVersion) >= 0;
}

/**
 * @brief 打印版本信息到串口
 */
void printVersionInfo() {
  Serial.println("\n========================================");
  Serial.println("      Firmware Version Information");
  Serial.println("========================================");
  Serial.print("Version:     ");
  Serial.println(versionString);
  Serial.print("Release:     ");
  Serial.println(versionInfo.date);
  Serial.print("Build Time:  ");
  Serial.println(versionInfo.buildTime);
  Serial.print("Git Branch:  ");
  Serial.println(versionInfo.gitBranch);
  Serial.print("Git Commit:  ");
  Serial.println(versionInfo.gitCommit);
  Serial.print("Version ID:  ");
  Serial.println(getVersionNumber());
  Serial.println("========================================\n");
}

/**
 * @brief 获取构建信息字符串
 * @return const char* 构建信息(包含日期、时间、Git信息)
 */
const char* getBuildInfo() {
  return buildInfo;
}
