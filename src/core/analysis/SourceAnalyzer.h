#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <stack>
#include <set>

#include "log/logger.h"
#include "struct.h"

// 源文件信息结构
struct SourceInfo {
    std::string path;                               // 源文件路径
    std::string file_name;                          // 文件名
    std::unordered_set<std::string> includes;       // 包含的头文件
};

// 头文件信息结构
struct HeaderInfo {
    std::string path;                               // 头文件路径
    std::string file_name;                          // 文件名
    std::unordered_set<std::string> includes;       // 包含的其他头文件
};

// 目标分析结果结构
struct TargetAnalysis {
    std::vector<SourceInfo> source_files;               // 源文件分析结果
    std::vector<HeaderInfo> header_files;               // 头文件分析结果
    std::unordered_set<std::string> included_headers;   // 所有包含的头文件（递归）
    std::unordered_set<std::string> provided_headers;   // 目标提供的头文件
};

// 可移除的依赖信息
struct RemovableDependency {
    std::string from_target;                // 来源目标
    std::string to_target;                  // 目标依赖
    std::string reason;                     // 移除原因
    ConfidenceLevel confidence;             // 置信度
};

class SourceAnalyzer {
public:
    // 构造函数，接收目标映射
    explicit SourceAnalyzer(const std::unordered_map<std::string, BazelTarget>& targets);
    
    // 析构函数
    ~SourceAnalyzer();
    
    // 分析单个目标（按需分析）
    void AnalyzeTarget(const std::string& target_name);
    
    // 检查头文件是否被目标使用
    bool IsHeaderUsed(const std::string& target_name, const std::string& header_path);
    
    // 检查依赖是否被需要（正确的逻辑：检查目标是否使用了依赖提供的头文件）
    bool IsDependencyNeeded(const std::string& target_name, const std::string& dependency);
    
    // 获取目标的可移除依赖列表
    std::vector<RemovableDependency> GetRemovableDependencies(const std::string& target_name);
    
    // 获取目标包含的所有头文件
    const std::unordered_set<std::string>& GetTargetIncludedHeaders(const std::string& target_name);
    
    // 获取目标提供的所有头文件
    const std::unordered_set<std::string>& GetTargetProvidedHeaders(const std::string& target_name);
    
    // 获取目标的源文件列表
    std::vector<std::string> GetTargetSourceFiles(const std::string& target_name) const;
    
    // 获取目标的头文件列表
    std::vector<std::string> GetTargetHeaderFiles(const std::string& target_name) const;
    
    // 清空所有缓存
    void ClearCache();
    
    // 清空指定目标的缓存
    void ClearTargetCache(const std::string& target_name);
    
private:
    // 确保目标已分析
    void EnsureTargetAnalyzed(const std::string& target_name);
    
    // 解析源文件
    bool ParseSourceFile(const std::string& file_path, SourceInfo& result);
    
    // 解析头文件
    bool ParseHeaderFile(const std::string& file_path, HeaderInfo& result);
    
    // 递归分析头文件包含关系
    void RecursivelyAnalyzeHeaderIncludes(const std::string& source_file,
                                         const std::unordered_set<std::string>& direct_includes,
                                         TargetAnalysis& analysis);
    
    // 查找头文件的实际路径
    std::string FindHeaderPath(const std::string& header_name);
    
    // 从行中提取包含的头文件
    void ExtractIncludesFromLine(const std::string& line, std::unordered_set<std::string>& includes);
    
    // 去除字符串两端的空白字符
    std::string Trim(const std::string& str) const;
    
    // 获取文件扩展名
    std::string GetFileExtension(const std::string& file_path) const;
    
    // 判断是否为源文件扩展名
    bool IsSourceFileExtension(const std::string& ext) const;
    
    // 判断是否为头文件扩展名
    bool IsHeaderFileExtension(const std::string& ext) const;
    
private:
    const std::unordered_map<std::string, BazelTarget>& targets_;  // 目标映射引用
    std::unordered_map<std::string, TargetAnalysis> target_analysis_;  // 分析结果缓存
    std::unordered_set<std::string> analyzed_targets_;  // 已分析目标集合
    mutable std::mutex analysis_mutex_;  // 分析缓存互斥锁
};
