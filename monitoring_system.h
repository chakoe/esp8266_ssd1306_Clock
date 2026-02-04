/**
 * @file monitoring_system.h
 * @brief 生产环境监控系统
 * 
 * 提供系统健康监控、性能指标收集和故障检测功能
 * 
 * @author ESP8266 SSD1306 Clock Project
 * @version 2.0
 * @date 2025-12-23
 */

#ifndef MONITORING_SYSTEM_H
#define MONITORING_SYSTEM_H

#include <Arduino.h>
#include "global_config.h"
#include "production_config.h"

// =============================================================================
// 监控系统配置
// =============================================================================

// 监控间隔配置
#define MONITOR_SYSTEM_INTERVAL 60000        // 1分钟系统监控
#define MONITOR_HEALTH_INTERVAL 300000        // 5分钟健康检查
#define MONITOR_PERFORMANCE_INTERVAL 120000  // 2分钟性能监控

// 阈值配置
#define CPU_USAGE_WARNING_THRESHOLD 80        // CPU使用率警告阈值 (%)
#define MEMORY_WARNING_THRESHOLD 85          // 内存使用率警告阈值 (%)
#define TEMPERATURE_WARNING_THRESHOLD 60      // 温度警告阈值 (°C)
#define ERROR_RATE_WARNING_THRESHOLD 5        // 错误率警告阈值 (/hour)

// 历史数据保存
#define METRICS_HISTORY_SIZE 24              // 保存24小时历史数据
#define PERFORMANCE_SAMPLES_PER_HOUR 30       // 每小时30个性能样本

// =============================================================================
// 系统健康状态枚举
// =============================================================================

enum SystemHealthStatus {
    HEALTH_EXCELLENT = 0,    // 优秀：所有指标正常
    HEALTH_GOOD = 1,         // 良好：轻微警告
    HEALTH_WARNING = 2,       // 警告：需要注意
    HEALTH_CRITICAL = 3,      // 严重：需要立即处理
    HEALTH_UNKNOWN = 4        // 未知：无法确定状态
};

// =============================================================================
// 性能指标结构体
// =============================================================================

struct SystemMetrics {
    // CPU和内存指标
    float cpuUsagePercent;                // CPU使用率 (%)
    uint32_t freeHeapSize;               // 可用堆内存 (bytes)
    uint32_t minFreeHeap;                // 最小可用堆内存历史
    float heapUsagePercent;              // 堆内存使用率 (%)
    
    // 网络指标
    uint32_t wifiUptimeSeconds;           // WiFi连接时长 (seconds)
    int wifiSignalStrength;               // WiFi信号强度 (dBm)
    uint32_t ntpSyncCount;               // NTP同步次数
    uint32_t ntpFailCount;              // NTP失败次数
    float ntpSuccessRate;                // NTP成功率 (%)
    
    // 时间和RTC指标
    uint32_t rtcSyncCount;               // RTC同步次数
    uint32_t rtcErrorCount;              // RTC错误次数
    float rtcAccuracy;                   // RTC精度 (seconds/day)
    uint32_t uptimeSeconds;              // 系统运行时间 (seconds)
    
    // 显示指标
    uint32_t displayRefreshCount;         // 显示刷新次数
    uint32_t displayErrorCount;          // 显示错误次数
    float averageFrameRate;               // 平均帧率 (FPS)
    
    // 错误指标
    uint32_t totalErrorCount;            // 总错误次数
    uint32_t watchdogResets;             // 看门狗重启次数
    uint32_t manualResets;               // 手动重启次数
    float errorRatePerHour;              // 每小时错误率
    
    // 硬件指标
    float internalTemperature;            // 内部温度 (°C)
    uint32_t i2cErrorCount;             // I2C错误次数
    uint32_t buttonPressCount;           // 按键按下次数
    
    // 电源指标
    float voltageLevel;                   // 电压等级 (V)
    float currentConsumption;             // 电流消耗 (mA)
    uint32_t powerEvents;                // 电源事件次数
};

// =============================================================================
// 健康检查结果结构体
// =============================================================================

struct HealthCheckResult {
    SystemHealthStatus overallStatus;      // 整体健康状态
    SystemHealthStatus cpuStatus;         // CPU健康状态
    SystemHealthStatus memoryStatus;      // 内存健康状态
    SystemHealthStatus networkStatus;     // 网络健康状态
    SystemHealthStatus timeStatus;        // 时间源健康状态
    SystemHealthStatus displayStatus;     // 显示健康状态
    SystemHealthStatus hardwareStatus;     // 硬件健康状态
    SystemHealthStatus powerStatus;       // 电源健康状态
    
    char summaryMessage[100];            // 健康状态摘要
    uint32_t criticalIssuesCount;        // 严重问题数量
    uint32_t warningIssuesCount;         // 警告问题数量
    uint32_t timestamp;                  // 检查时间戳
};

// =============================================================================
// 监控配置结构体
// =============================================================================

struct MonitoringConfig {
    bool enabled;                        // 是否启用监控
    bool metricsCollectionEnabled;         // 是否启用指标收集
    bool healthCheckEnabled;              // 是否启用健康检查
    bool alertingEnabled;                // 是否启用警报
    bool loggingEnabled;                  // 是否启用监控日志
    
    uint32_t systemCheckInterval;         // 系统检查间隔
    uint32_t healthCheckInterval;        // 健康检查间隔
    uint32_t performanceCheckInterval;     // 性能检查间隔
    
    uint32_t metricsRetentionHours;       // 指标保留小时数
    uint32_t alertThresholdLevel;         // 警报阈值级别
};

// =============================================================================
// 历史数据结构体
// =============================================================================

struct MetricsHistory {
    SystemMetrics samples[METRICS_HISTORY_SIZE];  // 历史样本数组
    uint8_t currentIndex;                         // 当前索引
    uint8_t sampleCount;                          // 样本数量
    uint32_t lastUpdateTime;                       // 最后更新时间
};

// =============================================================================
// 全局变量声明
// =============================================================================

extern MonitoringConfig monitoringConfig;
extern SystemMetrics currentMetrics;
extern HealthCheckResult lastHealthCheck;
extern MetricsHistory metricsHistory;

// =============================================================================
// 核心监控函数
// =============================================================================

// 初始化监控系统
bool initMonitoringSystem();

// 更新监控系统状态
void updateMonitoringSystem();

// 执行完整的系统监控
void performSystemMonitoring();

// 执行健康检查
HealthCheckResult performHealthCheck();

// =============================================================================
// 指标收集函数
// =============================================================================

// 收集CPU和内存指标
void collectCpuMemoryMetrics();

// 收集网络相关指标
void collectNetworkMetrics();

// 收集时间同步指标
void collectTimeMetrics();

// 收集显示相关指标
void collectDisplayMetrics();

// 收集硬件相关指标
void collectHardwareMetrics();

// 收集电源相关指标
void collectPowerMetrics();

// 收集错误统计指标
void collectErrorMetrics();

// =============================================================================
// 健康评估函数
// =============================================================================

// 评估CPU健康状态
SystemHealthStatus evaluateCpuHealth();

// 评估内存健康状态
SystemHealthStatus evaluateMemoryHealth();

// 评估网络健康状态
SystemHealthStatus evaluateNetworkHealth();

// 评估时间源健康状态
SystemHealthStatus evaluateTimeHealth();

// 评估显示健康状态
SystemHealthStatus evaluateDisplayHealth();

// 评估硬件健康状态
SystemHealthStatus evaluateHardwareHealth();

// 评估电源健康状态
SystemHealthStatus evaluatePowerHealth();

// =============================================================================
// 数据管理函数
// =============================================================================

// 保存当前指标到历史记录
void saveMetricsToHistory();

// 获取历史平均指标
SystemMetrics getAverageMetrics(uint8_t hours);

// 清理过期的历史数据
void cleanupOldMetrics();

// 获取系统运行统计
String getSystemStatistics();

// =============================================================================
// 警报和通知函数
// =============================================================================

// 检查是否需要发送警报
bool shouldSendAlert(const HealthCheckResult& result);

// 发送健康状态警报
void sendHealthAlert(const HealthCheckResult& result);

// 发送性能警报
void sendPerformanceAlert(const SystemMetrics& metrics);

// 发送错误警报
void sendErrorAlert(const char* errorMessage);

// =============================================================================
// 监控配置函数
// =============================================================================

// 加载监控配置
bool loadMonitoringConfig();

// 保存监控配置
bool saveMonitoringConfig();

// 更新监控配置
void updateMonitoringConfig(const MonitoringConfig& config);

// 重置监控配置为默认值
void resetMonitoringConfigToDefault();

// =============================================================================
// 实用工具函数
// =============================================================================

// 格式化健康状态为字符串
const char* healthStatusToString(SystemHealthStatus status);

// 格式化指标为显示字符串
String formatMetricsForDisplay(const SystemMetrics& metrics);

// 计算系统整体得分
float calculateSystemScore(const SystemMetrics& metrics);

// 生成健康报告
String generateHealthReport(const HealthCheckResult& result);

// 获取监控诊断信息
String getMonitoringDiagnostics();

// =============================================================================
// 性能优化相关
// =============================================================================

// 优化监控性能
void optimizeMonitoringPerformance();

// 暂停监控（用于性能密集型操作）
void pauseMonitoring();

// 恢复监控
void resumeMonitoring();

// 获取监控开销统计
String getMonitoringOverhead();

#endif // MONITORING_SYSTEM_H