#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "parser/AdvancedBazelQueryParser.h"

class BuildFileParser {
private:
    std::string workspace_root;
    std::unique_ptr<AdvancedBazelQueryParser> bazel_query_parser;
public:
    explicit BuildFileParser(const std::string& workspace_root);
    
    // 解析整个工作区的BUILD文件
    std::unordered_map<std::string, BazelTarget> ParseWorkspace();
    
    // 解析单个BUILD文件
    std::vector<BazelTarget> ParseBuildFile(const std::string& file_path);
    
private:
    // 解析单个目标
    BazelTarget ParseTarget(const std::string& content, const std::string& target_name);

    // 提取依赖列表
    std::vector<std::string> ExtractDependencies(const std::string& content);
    
    // 提取规则类型
    std::string ExtractRuleType(const std::string& content);
};