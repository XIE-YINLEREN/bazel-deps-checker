#pragma once

#include "graph/DependencyGraph.h"
#include "SourceAnalyzer.h" 
#include <cstddef>
#include <vector>
#include <string>
#include <unordered_set>
#include <memory>

enum class CycleType {
    DIRECT_CYCLE,
    DIAMOND_DEPENDENCY, 
    COMPLEX_CYCLE,
    SIMPLE_CYCLE
};


struct CycleAnalysis {
    std::vector<std::string> cycle;
    CycleType cycle_type;
    std::vector<std::string> suggested_fixes;
    std::vector<RemovableDependency> removable_dependencies;
    bool contains_test_targets;
    bool contains_external_deps;

    // 辅助方法
    bool containsTestTargets() const;
    bool containsExternalDeps() const;
    std::string toString() const;
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
    CycleDetector(const DependencyGraph& graph, std::unordered_map<std::string, BazelTarget> targets);

    // 分析所有循环并生成报告
    std::vector<CycleAnalysis> AnalyzeCycles();

    // 为特定循环生成修复建议
    std::string GenerateFixSuggestion(const CycleAnalysis& analysis) const;

private:
    // 分析和分类循环
    CycleAnalysis ClassifyCycle(const std::vector<std::string>& cycle) const;

    // 分析可移除依赖
    void AnalyzeRemovableDependencies(CycleAnalysis& analysis) const;

    // 依赖分析方法
    std::vector<RemovableDependency> AnalyzeDependencyAtCodeLevel(const std::string& from, const std::string& to) const;
    std::vector<RemovableDependency> AnalyzeDependencyAtTargetLevel(const std::string& from, const std::string& to) const;

    // 置信度计算
    ConfidenceLevel CalculateConfidence(const RemovableDependency& dep) const;

    // 关键依赖检查
    bool IsCriticalDependency(const std::string& from, const std::string& to) const;

    // 循环类型判定
    CycleType DetermineBaseCycleType(const std::vector<std::string>& cycle) const;

    // 额外分类应用
    void ApplyAdditionalClassifications(CycleAnalysis& analysis) const;

    // 类型特定建议添加
    void AddTypeSpecificSuggestions(CycleAnalysis& analysis) const;

    // 直接循环检测
    bool IsDirectCycle(const std::vector<std::string>& cycle) const;

    // 钻石依赖检测
    bool IsDiamondDependency(const std::vector<std::string>& cycle) const;

    // 提取公共接口建议
    std::string ExtractCommonInterface(const std::vector<std::string>& targets) const;

    // 辅助方法
    bool ContainsTestTargets(const std::vector<std::string>& cycle) const;

    // 外部依赖检查
    bool ContainsExternalDeps(const std::vector<std::string>& cycle) const;

    // 类型转字符串
    std::string CycleTypeToString(CycleType type) const;

    const DependencyGraph& graph_; 
    std::unordered_map<std::string, BazelTarget> targets_;
    std::shared_ptr<SourceAnalyzer> source_analyzer_;
};