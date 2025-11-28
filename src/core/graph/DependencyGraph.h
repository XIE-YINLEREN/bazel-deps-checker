#pragma once

#include <unordered_map>
#include <unordered_set>
#include <algorithm>

#include "struct.h"

class DependencyGraph {
public:
    DependencyGraph(std::unordered_map<std::string, BazelTarget> targets);

    // 检测循环依赖
    std::vector<std::vector<std::string>> FindCycles() const;
    
    // 获取传递依赖
    std::unordered_set<std::string> GetTransitiveDependencies(
        const std::string& target) const;
    
    // 查找未使用的依赖
    std::vector<std::string> FindUnusedDependencies(
        const std::string& target) const;
    
    // 可视化输出
    void ExportToDot(const std::string& output_path) const;
    
private:
    std::unordered_map<std::string, std::vector<std::string>> graph_;
    
    std::unordered_map<std::string, BazelTarget> grap_targets;

    // 构建依赖模型
    void BuildGraph();

    // 辅助方法
    bool HasCycleDFS(const std::string& node, 
                     std::unordered_set<std::string>& visited,
                     std::unordered_set<std::string>& recursion_stack,
                     std::vector<std::string>& path) const;
};