#pragma once

#include <unordered_map>
#include <thread>
#include <future>
#include <array>
#include <filesystem>
#include <vector>

#include "log/logger.h"


struct BazelTarget {
    std::string name;
    std::string path;
    std::vector<std::string> deps;
    std::vector<std::string> srcs;
    std::string rule_type;
};


class AdvancedBazelQueryParser {
private:
    std::string workspace_path;
    std::string bazel_binary;
    std::thread bazel_thread;
    std::string original_dir;
    const int timeout_seconds{30};
public:
    AdvancedBazelQueryParser(const std::string& workspace_path, 
                             const std::string& bazel_binary = "bazel.exe");

    // 解析整个工作区的Bazel目标
    std::unordered_map<std::string, BazelTarget> ParseWorkspace();

private:
    // 执行Bazel命令并返回输出
    std::string ExecuteBazelCommand(const std::string& command);

    // 静态方法执行命令行指令
    static std::string ExecuteCommand(const std::string& command);

    // 解析综合输出格式
    std::unordered_map<std::string, BazelTarget> ParseComprehensiveOutput(const std::string& output);

    // 解析单个目标信息
    BazelTarget ParseTargetFromOutput(const std::string& line);

    // 提取目标名称
    std::string ExtractTargetName(const std::string& target_label);

    // 查询单个目标的详细信息
    BazelTarget QuerySingleTargetDetails(const std::string& target_label);

    // 从kind输出中提取规则类型
    std::string ExtractRuleTypeFromKind(const std::string& kind_output);

    // 提取依赖列表
    std::vector<std::string> ExtractDependencies(const std::string& deps_output, 
                                                 const std::string& current_target);

    // 分割命令输出为行
    std::vector<std::string> SplitOutput(const std::string& output);

    // 使用单独查询的方式解析目标
    std::unordered_map<std::string, BazelTarget> ParseWithIndividualQueries();
};