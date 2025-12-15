#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <mutex>

namespace bazel_analyzer {

// 前置声明
struct BuildEvent;
struct CriticalPathNode;
struct OptimizationSuggestion;
struct BuildPhaseStats;
struct TargetStats;
struct AnalysisConfig;
struct AnalysisResult;

class BuildTimeAnalyzer {
public:
    BuildTimeAnalyzer(const std::string& workspace_path = ".",
                      const std::string& bazel_binary = "bazel");
    
    ~BuildTimeAnalyzer();
    
    // 静态常量定义
    static const std::string DEFAULT_PROFILE_OPTIONS;
    static const std::string DEFAULT_PROFILE_FILE;
    
    // 配置设置方法
    void SetWorkspacePath(const std::string& workspace_path);
    void SetBazelBinary(const std::string& bazel_binary);
    void SetBuildTargets(const std::vector<std::string>& targets);
    
    // 主接口 - 运行完整分析
    AnalysisResult RunFullAnalysis(const std::vector<std::string>& targets = {},
                                  bool force_regenerate = false);
    
    // 获取分析结果
    std::vector<CriticalPathNode> GetCriticalPath() const;
    std::vector<OptimizationSuggestion> GetOptimizationSuggestions() const;
    BuildPhaseStats GetBuildStats() const;
    
    // 清理功能
    void ClearAnalysis();
    
private:
    // 私有实现类
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// 结构体定义放在类外部
enum class EventType {
    ACTION,
    PACKAGE_LOAD,
    ANALYSIS,
    EXECUTION,
    VFS,
    OTHER
};

struct BuildEvent {
    std::string name;
    EventType type;
    std::chrono::microseconds start_time;
    std::chrono::microseconds duration;
    std::string thread_id;
    std::string category;
    std::vector<std::string> dependencies;
    std::string output;
    std::string rule;
    double percentage_of_total;
    
    BuildEvent(const std::string& n, EventType t, 
               std::chrono::microseconds start, std::chrono::microseconds dur)
        : name(n), type(t), start_time(start), duration(dur), percentage_of_total(0.0) {}
};

struct CriticalPathNode {
    std::string event_name;
    std::chrono::microseconds cumulative_duration;
    std::vector<std::string> path;
    std::string rule_type;
    
    bool operator<(const CriticalPathNode& other) const {
        return cumulative_duration > other.cumulative_duration;
    }
};

struct OptimizationSuggestion {
    enum class Severity {
        HIGH,
        MEDIUM,
        LOW
    };
    
    std::string issue;
    std::string suggestion;
    Severity severity;
    double estimated_improvement;
    std::vector<std::string> affected_targets;
};

struct BuildPhaseStats {
    std::chrono::microseconds total_duration{0};
    std::chrono::microseconds loading_duration{0};
    std::chrono::microseconds analysis_duration{0};
    std::chrono::microseconds execution_duration{0};
    std::chrono::microseconds vfs_duration{0};
    std::chrono::microseconds other_duration{0};
    
    std::string longest_loading_event;
    std::chrono::microseconds longest_loading_time{0};
    std::string longest_analysis_event;
    std::chrono::microseconds longest_analysis_time{0};
    std::string longest_execution_event;
    std::chrono::microseconds longest_execution_time{0};
    
    size_t max_concurrent_actions{0};
    double average_concurrency{0};
    
    size_t cache_hits{0};
    size_t cache_misses{0};
    double cache_hit_rate{0.0};
    
    size_t peak_memory_usage{0};
    size_t average_memory_usage{0};
    
    size_t total_vfs_operations{0};
    size_t vfs_read_operations{0};
    size_t vfs_write_operations{0};
};

struct AnalysisResult {
    bool success{false};
    std::string error_message;
    std::chrono::microseconds generation_time{0};
    std::chrono::microseconds analysis_time{0};
    BuildPhaseStats stats;
    std::vector<OptimizationSuggestion> suggestions;
    std::vector<CriticalPathNode> critical_paths;
    
    std::string report_path;
    std::string csv_path;
    std::string json_path;
    std::string dot_path;
};


struct TargetStats {
    std::string target_name;
    std::chrono::microseconds total_build_time{0};
    std::chrono::microseconds analysis_time{0};
    std::chrono::microseconds execution_time{0};
    std::chrono::microseconds loading_time{0};
    size_t action_count{0};
    size_t vfs_operations{0};
    size_t cache_hits{0};
    size_t cache_misses{0};
    double cache_hit_rate{0.0};
    
    TargetStats() = default;
    explicit TargetStats(const std::string& name) : target_name(name) {}
};

} // namespace bazel_analyzer
