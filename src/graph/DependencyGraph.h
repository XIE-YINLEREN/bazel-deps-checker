#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class DependencyGraph {
public:
    void AddTarget(const std::string& target, 
                   const std::vector<std::string>& dependencies);
    
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
    
    bool HasCycleDFS(const std::string& node, 
                     std::unordered_set<std::string>& visited,
                     std::unordered_set<std::string>& recursion_stack,
                     std::vector<std::string>& path) const;
};