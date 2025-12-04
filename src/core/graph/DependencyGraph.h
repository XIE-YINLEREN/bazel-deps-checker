#ifndef DEPENDENCY_GRAPH_H
#define DEPENDENCY_GRAPH_H

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <set>
#include <fstream>

#include "analysis/SourceAnalyzer.h"

class DependencyGraph {
public:
    explicit DependencyGraph(std::unordered_map<std::string, BazelTarget> targets);
    
    // 禁用拷贝和移动
    DependencyGraph(const DependencyGraph&) = delete;
    DependencyGraph& operator=(const DependencyGraph&) = delete;
    
    // 图分析功能
    std::vector<std::vector<std::string>> FindCycles() const;

    // 获取传递依赖
    std::unordered_set<std::string> GetTransitiveDependencies(const std::string& target) const;

    // 查找未使用的依赖
    std::vector<std::string> FindUnusedDependencies(const std::string& target) const;
    
    // 获取直接依赖
    const std::vector<std::string>& GetDirectDependencies(const std::string& target) const;
    
    // 未使用依赖检测相关
    std::unordered_set<std::string> GetReverseDependencies(const std::string& target) const;

    // 设置源码分析器
    void SetSourceAnalyzer(SourceAnalyzer* source_analyzer) const;
private:
    // 源代码分析器
    mutable SourceAnalyzer* source_analyzer_;           

    // 目标名称到BazelTarget的映射
    std::unordered_map<std::string, BazelTarget> grap_targets_;

    // 依赖图表示：目标 -> 依赖列表
    std::unordered_map<std::string, std::vector<std::string>> graph_;

    // 反向依赖关系缓存
    mutable std::unordered_map<std::string, std::unordered_set<std::string>> reverse_deps_cache_;
    
    // 构建依赖图
    void BuildGraph();

    // 构建反向依赖关系缓存
    void BuildReverseDependencies();

    // 简化依赖名称
    std::string SimplifyDependencyName(const std::string& dep) const;
    
    // 循环检测相关
    void FindCyclesDFS(
        const std::string& node,
        std::unordered_map<std::string, int>& color,
        std::unordered_map<std::string, std::string>& parent,
        std::vector<std::vector<std::string>>& cycles) const;

    // 检查依赖是否被使用
    bool IsDependencyUsed(const std::string& dependency, const std::string& exclude_target) const;

    // 检查传递依赖是否真正需要
    bool IsDependencyTrulyNeeded(const std::string& target, const std::string& dependency) const;

    // 检查依赖是否被传递依赖需要
    bool IsDependencyNeededByTransitiveDeps(const std::string& target, const std::string& dependency) const;

    // 查找所有未使用的依赖
    std::vector<RemovableDependency> FindAllUnusedDependencies() const;

    // 查找传递冗余依赖
    std::vector<std::string> FindTransitiveRedundantDependencies(const std::string& target) const;
};

#endif