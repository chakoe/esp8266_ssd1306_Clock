#ifndef VERSION_H
#define VERSION_H

#include <Arduino.h>

// =============================================================================
// 版本号统一管理模块
// =============================================================================

/**
 * @brief 版本信息结构体
 * 
 * 统一管理固件版本的所有相关信息
 */
struct VersionInfo {
  uint8_t major;    // 主版本号
  uint8_t minor;    // 次版本号
  uint8_t patch;    // 补丁版本号
  const char* date; // 版本日期
  const char* buildTime; // 编译时间
  const char* gitBranch; // Git分支名称
  const char* gitCommit; // Git提交哈希(短)
};

// =============================================================================
// 版本宏定义
// =============================================================================

#define FIRMWARE_VERSION_MAJOR 2
#define FIRMWARE_VERSION_MINOR 1
#define FIRMWARE_VERSION_PATCH 0
#define FIRMWARE_VERSION_DATE "2025-02-03"

// 编译时间宏(由编译器自动生成)
#define BUILD_TIME __TIME__
#define BUILD_DATE __DATE__

// Git信息宏(需要在编译时通过 -D 参数传入)
// 示例: -DGIT_BRANCH=\"master\" -DGIT_COMMIT=\"abc1234\"
#ifndef GIT_BRANCH
  #define GIT_BRANCH "unknown"
#endif

#ifndef GIT_COMMIT
  #define GIT_COMMIT "unknown"
#endif

// =============================================================================
// 版本字符串宏
// =============================================================================

#define FIRMWARE_VERSION_STRING "v" \
  STRINGIFY(FIRMWARE_VERSION_MAJOR) "." \
  STRINGIFY(FIRMWARE_VERSION_MINOR) "." \
  STRINGIFY(FIRMWARE_VERSION_PATCH)

#define STRINGIFY(x) STRINGIFY_IMPL(x)
#define STRINGIFY_IMPL(x) #x

// =============================================================================
// 版本管理函数声明
// =============================================================================

/**
 * @brief 初始化版本管理模块
 */
void initVersionManager();

/**
 * @brief 获取版本信息结构体
 * @return VersionInfo 版本信息结构体
 */
VersionInfo getVersionInfo();

/**
 * @brief 获取版本字符串
 * @return const char* 版本字符串(如 "v2.1.0")
 */
const char* getVersionString();

/**
 * @brief 获取完整版本信息字符串
 * @return const char* 完整版本信息(包含日期、编译时间等)
 */
const char* getFullVersionInfo();

/**
 * @brief 获取版本号作为整数(格式: major*10000 + minor*100 + patch)
 * @return uint32_t 版本号整数
 */
uint32_t getVersionNumber();

/**
 * @brief 比较两个版本号
 * @param v1 第一个版本号
 * @param v2 第二个版本号
 * @return int 负数表示v1<v2, 0表示相等, 正数表示v1>v2
 */
int compareVersions(uint32_t v1, uint32_t v2);

/**
 * @brief 检查版本是否兼容
 * @param targetVersion 目标版本号
 * @return true 如果当前版本 >= 目标版本
 * @return false 如果当前版本 < 目标版本
 */
bool isVersionCompatible(uint32_t targetVersion);

/**
 * @brief 打印版本信息到串口
 */
void printVersionInfo();

/**
 * @brief 获取构建信息字符串
 * @return const char* 构建信息(包含日期、时间、Git信息)
 */
const char* getBuildInfo();

#endif // VERSION_H
