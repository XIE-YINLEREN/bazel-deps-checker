#include "DependencyGraph.h"
#include <algorithm>
#include <cctype>
#include <vector>

DependencyGraph::DependencyGraph(std::unordered_map<std::string, BazelTarget> targets)
    : grap_targets_(std::move(targets)) {
    BuildGraph();
    BuildReverseDependencies();
}

void DependencyGraph::SetSourceAnalyzer(SourceAnalyzer* source_analyzer) const {
    source_analyzer_ = source_analyzer;
}

void DependencyGraph::BuildGraph() {
    graph_.clear();
    
    for (const auto& [name, target] : grap_targets_) {
        std::vector<std::string> dependencies;
        dependencies.reserve(target.deps.size());
        
        for (const auto& dep : target.deps) {
            std::string simplified_dep = SimplifyDependencyName(dep);
            if (!simplified_dep.empty() && simplified_dep.find("@") == std::string::npos) {
                dependencies.push_back(std::move(simplified_dep));
            }
        }
        
        graph_[name] = std::move(dependencies);
    }
}

void DependencyGraph::BuildReverseDependencies() {
    reverse_deps_cache_.clear();
    
    // 构建反向依赖关系图
    for (const auto& [target, deps] : graph_) {
        for (const auto& dep : deps) {
            reverse_deps_cache_[dep].insert(target);
        }
    }
}

std::string DependencyGraph::SimplifyDependencyName(const std::string& dep) const {
    std::string result = dep;
    
    result.erase(std::remove(result.begin(), result.end(), ' '), result.end());
    result.erase(std::remove(result.begin(), result.end(), '\t'), result.end());
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    
    return result;
}

std::vector<std::vector<std::string>> DependencyGraph::FindCycles() const {
    // 使用DFS检测所有循环
    std::vector<std::vector<std::string>> cycles;
    // 0:未访问, 1:访问中, 2:已访问
    std::unordered_map<std::string, int> color;
    // 记录父节点以便回溯
    std::unordered_map<std::string, std::string> parent;
    
    for (const auto& [node, _] : graph_) {
        if (color[node] == 0) {
            FindCyclesDFS(node, color, parent, cycles);
        }
    }
    
    return cycles;
}

void DependencyGraph::FindCyclesDFS(
    const std::string& node,
    std::unordered_map<std::string, int>& color,
    std::unordered_map<std::string, std::string>& parent,
    std::vector<std::vector<std::string>>& cycles) const {
    
    // 标记为访问中
    color[node] = 1; 
    
    auto it = graph_.find(node);
    if (it != graph_.end()) {
        for (const auto& neighbor : it->second) {
            if (color[neighbor] == 0) {
                // 未访问的节点
                parent[neighbor] = node;
                FindCyclesDFS(neighbor, color, parent, cycles);
            } else if (color[neighbor] == 1) {
                // 找到后向边，说明有循环
                std::vector<std::string> cycle;
                std::string current = node;
                
                // 回溯构建循环路径
                while (current != neighbor) {
                    cycle.push_back(current);
                    current = parent[current];
                }
                cycle.push_back(neighbor);
                // 闭合循环
                cycle.push_back(node);
                
                std::reverse(cycle.begin(), cycle.end());
                cycles.push_back(std::move(cycle));
            }
            // color[neighbor] == 2: 已访问完成，忽略
        }
    }
    
    // 标记为已访问完成
    color[node] = 2; 
}

std::unordered_set<std::string> DependencyGraph::GetTransitiveDependencies(
    const std::string& target) const {
    std::unordered_set<std::string> transitive_deps;
    
    auto it = graph_.find(target);
    if (it == graph_.end()) {
        return transitive_deps;
    }
    
    std::queue<std::string> queue;
    for (const auto& dep : it->second) {
        queue.push(dep);
    }
    
    while (!queue.empty()) {
        std::string current = std::move(queue.front());
        queue.pop();
        
        if (!transitive_deps.insert(current).second) {
            continue;
        }
        
        auto current_it = graph_.find(current);
        if (current_it != graph_.end()) {
            for (const auto& dep : current_it->second) {
                if (transitive_deps.find(dep) == transitive_deps.end()) {
                    queue.push(dep);
                }
            }
        }
    }
    
    return transitive_deps;
}

std::vector<std::string> DependencyGraph::FindUnusedDependencies(const std::string& target) const {
    std::vector<std::string> unused_deps;
    
    auto it = graph_.find(target);
    if (it == graph_.end()) {
        return unused_deps;
    }
    
    // 预分配内存
    unused_deps.reserve(it->second.size());
    
    // 使用源码分析器进行精确分析
    if (source_analyzer_) {
        for (const auto& direct_dep : it->second) {
            if (!IsDependencyTrulyNeeded(target, direct_dep)) {
                unused_deps.push_back(direct_dep);
            }
        }
    } else {
        // 使用简单的依赖图分析
        for (const auto& declared_dep : it->second) {
            if (!IsDependencyUsed(declared_dep, target)) {
                unused_deps.push_back(declared_dep);
            }
        }
    }
    
    return unused_deps;
}

bool DependencyGraph::IsDependencyTrulyNeeded(const std::string& target, 
                                            const std::string& dependency) const {
    // 直接检查目标是否使用了依赖中的头文件
    if (!source_analyzer_->IsDependencyNeeded(target, dependency)) {
        // 目标没有直接使用这个依赖
        // 但还需要检查这个依赖是否被目标的传递依赖所需要
        return IsDependencyNeededByTransitiveDeps(target, dependency);
    }

    // 目标直接使用了这个依赖
    return true;
}

bool DependencyGraph::IsDependencyNeededByTransitiveDeps(const std::string& target,
                                                       const std::string& dependency) const {
    // 检查目标的直接依赖是否依赖这个库
    auto target_deps_it = graph_.find(target);
    if (target_deps_it == graph_.end()) {
        return false;
    }
    
    for (const auto& direct_dep : target_deps_it->second) {
        if (direct_dep == dependency) continue;
        
        // 获取直接依赖的传递依赖
        std::unordered_set<std::string> transitive_deps = GetTransitiveDependencies(direct_dep);
        if (transitive_deps.find(dependency) != transitive_deps.end()) {
            // 检查这个直接依赖是否真正需要该依赖
            if (source_analyzer_) {
                if (source_analyzer_->IsDependencyNeeded(direct_dep, dependency)) {
                    // 直接依赖真正需要这个依赖，所以目标需要传递声明
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool DependencyGraph::IsDependencyUsed(const std::string& dependency, 
                                     const std::string& exclude_target) const {
    // 使用缓存的反向依赖关系来快速检查
    auto reverse_it = reverse_deps_cache_.find(dependency);
    if (reverse_it == reverse_deps_cache_.end()) {
        // 没有其他目标依赖它
        return false;
    }
    
    const auto& reverse_deps = reverse_it->second;
    
    // 检查是否有除了exclude_target之外的其他目标依赖它
    for (const auto& depender : reverse_deps) {
        if (depender != exclude_target) {
            return true;
        }
    }
    
    return false;
}

std::unordered_set<std::string> DependencyGraph::GetReverseDependencies(const std::string& target) const {
    auto it = reverse_deps_cache_.find(target);
    if (it != reverse_deps_cache_.end()) {
        return it->second;
    }
    return {};
}

const std::vector<std::string>& DependencyGraph::GetDirectDependencies(const std::string& target) const {
    static const std::vector<std::string> empty_deps;
    
    auto it = graph_.find(target);
    if (it != graph_.end()) {
        return it->second;
    }
    
    return empty_deps;
}

std::vector<RemovableDependency> DependencyGraph::FindAllUnusedDependencies() const {
    std::vector<RemovableDependency> all_unused_deps;
    
    size_t total_deps = 0;
    for (const auto& [_, deps] : graph_) {
        total_deps += deps.size();
    }

     // 假设25%的依赖是未使用的
    all_unused_deps.reserve(total_deps / 4);
    
    for (const auto& [target_name, _] : graph_) {
        std::vector<std::string> unused_deps = FindUnusedDependencies(target_name);
        if (!unused_deps.empty()) {
            const size_t current_size = all_unused_deps.size();
            all_unused_deps.resize(current_size + unused_deps.size());
            
            auto it = all_unused_deps.begin() + current_size;
            for (const auto& unused_dep : unused_deps) {
                it->from_target = target_name;
                it->to_target = unused_dep;
                it->reason = "Dependency is not used by source code";
                it->confidence = source_analyzer_ ? ConfidenceLevel::HIGH : ConfidenceLevel::MEDIUM;
                ++it;
            }
        }
    }
    
    // 压缩
    if (all_unused_deps.capacity() > all_unused_deps.size() * 1.5) {
        all_unused_deps.shrink_to_fit();
    }
    
    return all_unused_deps;
}

std::vector<std::string> DependencyGraph::FindTransitiveRedundantDependencies(const std::string& target) const {
    std::vector<std::string> redundant_deps;
    
    auto it = graph_.find(target);
    if (it == graph_.end()) {
        return redundant_deps;
    }
    
    // 获取所有传递依赖
    std::unordered_set<std::string> transitive_deps = GetTransitiveDependencies(target);
    
    // 检查每个直接依赖是否可以被省略
    for (const auto& direct_dep : it->second) {
        // 检查这个直接依赖是否已经通过其他路径成为传递依赖
        bool is_transitive_through_other = false;
        
        for (const auto& other_dep : it->second) {
            if (other_dep == direct_dep) continue;
            
            // 获取其他依赖的传递依赖
            std::unordered_set<std::string> other_transitive = GetTransitiveDependencies(other_dep);
            if (other_transitive.find(direct_dep) != other_transitive.end()) {
                // 这个依赖已经通过其他路径传递了
                is_transitive_through_other = true;
                
                // 使用源码分析器进一步确认
                if (source_analyzer_) {
                    if (!source_analyzer_->IsDependencyNeeded(target, direct_dep) &&
                        !IsDependencyNeededByTransitiveDeps(target, direct_dep)) {
                        // 目标不需要这个依赖，可以省略
                        redundant_deps.push_back(direct_dep);
                    }
                } else {
                    // 如果没有源码分析器，基于依赖图判断
                    redundant_deps.push_back(direct_dep);
                }
                break;
            }
        }
    }
    
    return redundant_deps;
}
