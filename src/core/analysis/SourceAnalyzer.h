#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <filesystem>

#include "struct.h"

struct SourceFile {
    std::string path;
    std::string content;
    std::vector<std::string> included_headers;
    std::unordered_set<std::string> used_symbols;
};

struct HeaderFile {
    std::string path;
    std::string content;
    std::unordered_set<std::string> provided_symbols;
};

struct RemovableDependency {
    std::string from_target;
    std::string to_target;
    std::vector<std::string> unused_headers;
    std::vector<std::string> unused_symbols;
    std::string reason;
    ConfidenceLevel confidence;
};

class SourceAnalyzer {
public:
    explicit SourceAnalyzer(std::unordered_map<std::string, BazelTarget> targets);
    
    // 分析特定目标
    void AnalyzeTarget(const std::string& target_name);
    
    // 分析所有目标
    void AnalyzeAllTargets();
    
    // 获取未使用的头文件
    std::vector<std::string> GetUnusedHeaders(
        const std::string& target_name, const std::string& dependency) const;
    
    // 获取未使用的符号
    std::vector<std::string> GetUnusedSymbols(
        const std::string& target_name, const std::string& dependency) const;
    
    // 检查依赖是否必要
    bool IsDependencyNeeded(const std::string& target_name, const std::string& dependency) const;
    
    // 获取目标的所有可移除依赖
    std::vector<RemovableDependency> GetRemovableDependencies(const std::string& target_name) const;

private:
    // 核心数据结构
    std::unordered_map<std::string, BazelTarget> targets_;
    std::unordered_map<std::string, std::vector<SourceFile>> target_sources_;
    std::unordered_map<std::string, std::vector<HeaderFile>> target_headers_;
    std::unordered_map<std::string, std::unordered_set<std::string>> target_used_symbols_;
    std::unordered_map<std::string, std::unordered_set<std::string>> target_provided_symbols_;
    
    // 文件解析方法
    void ParseSourceFile(const std::string& file_path, SourceFile& result);
    void ParseHeaderFile(const std::string& file_path, HeaderFile& result);
    
    // 内容分析方法
    std::vector<std::string> ExtractIncludes(const std::string& content);
    std::unordered_set<std::string> ExtractUsedSymbols(const std::string& content);
    std::unordered_set<std::string> ExtractProvidedSymbols(const std::string& content);
    
    // 工具方法
    bool IsHeaderUsed(const std::string& target_name, const std::string& header_path) const;
    bool IsSymbolUsed(const std::string& target_name, const std::string& symbol) const;
    bool IsSourceFile(const std::string& file_path) const;
    bool IsHeaderFile(const std::string& file_path) const;
    std::string GetFileName(const std::string& file_path) const;
    
    // 目标分析方法
    std::vector<std::string> GetTargetSourceFiles(const std::string& target_name) const;
    std::vector<std::string> GetTargetHeaders(const std::string& target_name) const;
    std::vector<std::string> GetDependencyHeaders(const std::string& dependency) const;
    std::unordered_set<std::string> GetDependencyProvidedSymbols(const std::string& dependency) const;
    
    // 符号解析辅助方法
    void ExtractFunctionsAndClasses(const std::string& content, std::unordered_set<std::string>& symbols);
    void ExtractVariables(const std::string& content, std::unordered_set<std::string>& symbols);
    void ExtractTypeDefs(const std::string& content, std::unordered_set<std::string>& symbols);
};