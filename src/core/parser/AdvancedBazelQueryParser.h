#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <sstream>
#include <thread>
#include <future>
#include <array>
#include <memory>
#include <filesystem>


#include "struct.h"
#include "log/logger.h"

class AdvancedBazelQueryParser {
public:
    AdvancedBazelQueryParser(const std::string& workspace_path,
                           const std::string& bazel_binary = "bazel");
    
    std::unordered_map<std::string, BazelTarget> ParseWorkspace();
    
private:
    // 配置相关
    std::string workspace_path;
    std::string bazel_binary;
    std::string original_dir;
    int timeout_seconds{30};
    
    // 查询策略
    enum class QueryStrategy {
        COMPREHENSIVE,  // 一次性查询
        INDIVIDUAL      // 逐个查询
    };
    
    // 查询实现
    std::unordered_map<std::string, BazelTarget> ParseWithComprehensiveQuery();
    std::unordered_map<std::string, BazelTarget> ParseWithIndividualQueries();
    
    // 命令执行
    std::string ExecuteBazelCommand(const std::string& command);
    std::string ExecuteSystemCommand(const std::string& command);
    
    // 解析单个目标
    BazelTarget ParseTargetFromLabelKind(const std::string& line);
    void QueryTargetDetails(BazelTarget& target);
    
    // 工具方法
    std::vector<std::string> SplitLines(const std::string& input);
    std::string ExtractTargetName(const std::string& target_label);
    std::string ExtractRuleType(const std::string& kind_output);
    std::vector<std::string> ExtractDependencies(const std::string& target_label, const std::string& deps_output);
    std::unordered_map<std::string, BazelTarget> ParseAllTargetsFallback();
    
    // 目录管理
    void ChangeToWorkspaceDirectory();
    void RestoreOriginalDirectory();
    
    // 环境验证
    bool ValidateBazelEnvironment();
};