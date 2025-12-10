#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "graph/DependencyGraph.h"
#include "analysis/SourceAnalyzer.h"

// 循环类型枚举
enum class CycleType {
    DIRECT_CYCLE,        // 直接循环（双向依赖）
    DIAMOND_DEPENDENCY,  // 菱形依赖
    COMPLEX_CYCLE,       // 复杂循环（大于3个节点）
    SIMPLE_CYCLE         // 简单循环（2-3个节点）
};

// 循环分析结果
struct CycleAnalysis {
    std::vector<std::string> cycle;            // 循环路径
    CycleType cycle_type;                      // 循环类型
    bool contains_test_targets;                // 是否包含测试目标
    bool contains_external_deps;               // 是否包含外部依赖
    std::vector<RemovableDependency> removable_dependencies;  // 可移除的依赖
    std::vector<std::string> suggested_fixes;  // 建议的修复方案
};

inline std::ostream& operator<<(std::ostream& os, CycleType type) {
    switch (type) {
        case CycleType::DIRECT_CYCLE: 
            os << "DIRECT_CYCLE";
            break;
        case CycleType::DIAMOND_DEPENDENCY: 
            os << "DIAMOND_DEPENDENCY";
            break;
        case CycleType::COMPLEX_CYCLE: 
            os << "COMPLEX_CYCLE";
            break;
        case CycleType::SIMPLE_CYCLE: 
            os << "SIMPLE_CYCLE";
            break;
        default:
            os << "UNKNOWN";
            break;
    }
    return os;
}

class CycleDetector {
public:
    // 构造函数，接受依赖图和目标映射
    CycleDetector(const DependencyGraph& graph, const std::unordered_map<std::string, BazelTarget>& targets, const std::string workspace_path);
    
    // 分析所有循环依赖
    std::vector<CycleAnalysis> AnalyzeCycles();
    
    // 分析未使用依赖
    std::vector<RemovableDependency> AnalyzeUnusedDependencies();
private:
    // 分类单个循环
    CycleAnalysis ClassifyCycle(const std::vector<std::string>& cycle) const;
    
    // 分析循环中的可移除依赖
    void AnalyzeRemovableDependencies(CycleAnalysis& analysis) const;
    
    // 代码级别的依赖分析
    std::vector<RemovableDependency> AnalyzeDependencyAtCodeLevel(const std::string& from, const std::string& to) const;
    
    // Target级别的依赖分析
    std::vector<RemovableDependency> AnalyzeDependencyAtTargetLevel(const std::string& from, const std::string& to) const;
    
    // 计算依赖移除的置信度
    ConfidenceLevel CalculateConfidence(const RemovableDependency& dep) const;
    
    // 检查依赖是否关键（无可替代路径）
    bool IsCriticalDependency(const std::string& from, const std::string& to) const;
    
    // 确定循环的基本类型
    CycleType DetermineBaseCycleType(const std::vector<std::string>& cycle) const;
    
    // 检查是否为直接循环（双向依赖）
    bool IsDirectCycle(const std::vector<std::string>& cycle) const;
    
    // 检查是否为菱形依赖
    bool IsDiamondDependency(const std::vector<std::string>& cycle) const;
    
    // 检查循环是否包含测试目标
    bool ContainsTestTargets(const std::vector<std::string>& cycle) const;
    
    // 检查循环是否包含外部依赖
    bool ContainsExternalDeps(const std::vector<std::string>& cycle) const;
    
    // 应用额外的分类建议
    void ApplyAdditionalClassifications(CycleAnalysis& analysis) const;
    
    // 根据循环类型添加特定建议
    void AddTypeSpecificSuggestions(CycleAnalysis& analysis) const;
    
    // 提取公共接口名
    std::string ExtractCommonInterface(const std::vector<std::string>& targets) const;
    
    // 将循环类型转换为字符串
    std::string CycleTypeToString(CycleType type) const;
    
private:
    const std::string workspace_path_;                          // 工作区路径
    const DependencyGraph& graph_;                              // 依赖图引用
    const std::unordered_map<std::string, BazelTarget>& targets_;  // 目标映射引用
    std::shared_ptr<SourceAnalyzer> source_analyzer_;           // 源代码分析器
};
