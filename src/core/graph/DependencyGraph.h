#ifndef DEPENDENCY_GRAPH_H
#define DEPENDENCY_GRAPH_H

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <set>
#include <fstream>
#include <string_view>

#include "analysis/SourceAnalyzer.h"

class DependencyGraph {
public:
    explicit DependencyGraph(const std::unordered_map<std::string, BazelTarget>& targets);
    
    // 禁用拷贝和移动
    DependencyGraph(const DependencyGraph&) = delete;
    DependencyGraph& operator=(const DependencyGraph&) = delete;
    
    // 图分析功能
    std::vector<std::vector<std::string>> FindCycles() const;

    // 获取传递依赖
    std::unordered_set<std::string> GetTransitiveDependencies(const std::string& target) const;

    // 查找未使用的依赖
    std::vector<std::string> FindUnusedDependencies(const std::string& target) const;

    // 查找所有未使用依赖
    std::vector<RemovableDependency> FindAllUnusedDependencies() const;
    
    // 获取直接依赖
    const std::vector<std::string>& GetDirectDependencies(const std::string& target) const;

    // 检查是否存在直接边
    bool HasDirectEdge(const std::string& from, const std::string& to) const;
    
    // 未使用依赖检测相关
    std::unordered_set<std::string> GetReverseDependencies(const std::string& target) const;

    // 设置源码分析器
    void SetSourceAnalyzer(SourceAnalyzer* source_analyzer) const;
private:
    // 源代码分析器，仅用于未使用依赖的代码级判定
    mutable SourceAnalyzer* source_analyzer_;

    // 目标名称到 BazelTarget 的只读映射
    const std::unordered_map<std::string, BazelTarget>& graph_targets_;

    // 字符串级依赖图：保留对外接口和报告生成所需的 target 名称
    std::unordered_map<std::string, std::vector<std::string>> graph_;
    // 直接边快速查找：避免高频 direct-edge 判断退化成线性扫描
    std::unordered_map<std::string, std::unordered_set<std::string>> adjacency_set_;
    // 节点 ID 索引：为内部 BFS / 后续图算法优化提供整数邻接表
    std::vector<std::string> node_names_;
    std::unordered_map<std::string, size_t> node_ids_;
    std::vector<std::vector<size_t>> adjacency_ids_;

    // 反向依赖关系缓存：dependency -> dependers
    mutable std::unordered_map<std::string, std::unordered_set<std::string>> reverse_deps_cache_;

    // 传递依赖缓存：target -> all reachable dependency names
    mutable std::unordered_map<std::string, std::unordered_set<std::string>> transitive_deps_cache_;
    mutable std::unordered_set<std::string> empty_dependency_set_;
    // target + dependency 粒度的“传递依赖是否真正需要”缓存
    mutable std::unordered_map<std::string, bool> dependency_need_cache_;
    
    // 构建依赖图
    void BuildGraph();

    // 构建反向依赖关系缓存
    void BuildReverseDependencies();

    // 简化依赖名称
    std::string SimplifyDependencyName(const std::string& dep) const;
    size_t GetOrCreateNodeId(const std::string& node_name);
    size_t GetNodeId(const std::string& node_name) const;
    const std::unordered_set<std::string>& GetTransitiveDependenciesRef(const std::string& target) const;
    
    // 循环检测相关
    void FindCyclesDFS(
        const std::string& node,
        std::unordered_map<std::string, int>& color,
        std::unordered_map<std::string, std::string>& parent,
        std::vector<std::vector<std::string>>& cycles,
        std::set<std::string>& cycle_signatures,
        const std::unordered_set<std::string>* allowed_nodes = nullptr) const;
    std::string CanonicalizeCycle(const std::vector<std::string>& cycle) const;
    std::vector<std::unordered_set<std::string>> FindStronglyConnectedComponents() const;
    void FindStronglyConnectedComponentsDFS(
        const std::string& node,
        int& current_index,
        std::unordered_map<std::string, int>& index_map,
        std::unordered_map<std::string, int>& low_link,
        std::vector<std::string>& stack,
        std::unordered_set<std::string>& on_stack,
        std::vector<std::unordered_set<std::string>>& components) const;
    // 检查依赖是否被使用
    bool IsDependencyUsed(const std::string& dependency, const std::string& exclude_target) const;

    // 检查传递依赖是否真正需要
    bool IsDependencyTrulyNeeded(const std::string& target, const std::string& dependency) const;

    // 检查依赖是否被传递依赖需要
    bool IsDependencyNeededByTransitiveDeps(const std::string& target, const std::string& dependency) const;

    // 查找传递冗余依赖
    std::vector<std::string> FindTransitiveRedundantDependencies(const std::string& target) const;
};

#endif
