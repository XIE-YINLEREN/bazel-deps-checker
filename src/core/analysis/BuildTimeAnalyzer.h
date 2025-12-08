#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <set>
#include <filesystem>
#include <functional>

#include "nlohmann/json.hpp"
#include "log/logger.h"
#include "pipe.h"

using json = nlohmann::json;

namespace fs = std::filesystem;

class BuildTimeAnalyzer {
public:
    BuildTimeAnalyzer(const std::string& bazel_binary, const std::string& workspace_path);

    ~BuildTimeAnalyzer();

    // 构建并生成profile文件
    bool createProfile(const std::string& target = "//...") const;
    
    // 分析profile文件
    json analyzeProfile() const;
    
    // 获取构建耗时详情
    std::map<std::string, double> getBuildTimeBreakdown() const;
    
    // 获取最耗时的目标
    std::vector<std::pair<std::string, double>> getTopTimeConsumingTargets(int top_n = 10) const;
    
    // 获取内存使用详情
    std::map<std::string, size_t> getMemoryUsage() const;
    
    // 生成构建报告
    std::string generateBuildReport() const;
    
    // 获取所有构建目标
    std::set<std::string> getAllTargets() const;
    
    // 获取目标间的依赖关系
    std::map<std::string, std::set<std::string>> getTargetDependencies() const;
    
    // 清理profile文件
    void cleanupProfile() const;
    
    // 设置自定义profile选项
    void setCustomProfileOptions(const std::string& options);
    
    // 获取profile文件路径
    std::string getProfilePath() const;
    
    // 执行构建并返回结果
    std::pair<bool, std::string> executeBuild(const std::string& target) const;

private:
    std::string bazel_binary_;
    std::string workspace_path_;
    std::string profile_file_path_;
    std::string profile_options_;
    
    // 内部辅助方法
    bool validateEnvironment() const;
    std::string constructBuildCommand(const std::string& target) const;
    json loadProfileJson() const;
    void analyzeCriticalPath(const json& profile_data, json& result) const;
    void analyzePhaseTimes(const json& profile_data, json& result) const;
    void analyzeActionCounts(const json& profile_data, json& result) const;
    void analyzeCachePerformance(const json& profile_data, json& result) const;
    std::string formatDuration(double seconds) const;
    std::string formatMemory(size_t bytes) const;
    
    // 默认profile选项
    static const std::string DEFAULT_PROFILE_OPTIONS;
};
