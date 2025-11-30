#include "DependencyGraph.h"
#include <algorithm>
#include <cctype>

DependencyGraph::DependencyGraph(std::unordered_map<std::string, BazelTarget> targets)
    : grap_targets_(std::move(targets)) {
    BuildGraph();
    BuildReverseDependencies();
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
    
    for (const auto& declared_dep : it->second) {
        if (!IsDependencyUsed(declared_dep, target)) {
            unused_deps.push_back(declared_dep);
        }
    }
    
    return unused_deps;
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

std::unordered_set<std::string> DependencyGraph::GetReverseDependencies(
    const std::string& target) const {
    auto it = reverse_deps_cache_.find(target);
    if (it != reverse_deps_cache_.end()) {
        return it->second;
    }
    return {};
}

void DependencyGraph::ExportToDot(const std::string& output_path) const {
    std::ofstream dot_file(output_path);
    if (!dot_file.is_open()) {
        return;
    }
    
    dot_file << "digraph DependencyGraph {\n";
    dot_file << "  rankdir=TB;\n";
    dot_file << "  node [shape=box, style=filled, fillcolor=lightblue];\n";
    dot_file << "  edge [arrowhead=vee];\n\n";
    
    // 添加节点
    for (const auto& [node, _] : graph_) {
        dot_file << "  \"" << node << "\";\n";
    }
    
    dot_file << "\n";
    
    // 添加边
    for (const auto& [node, dependencies] : graph_) {
        for (const auto& dep : dependencies) {
            dot_file << "  \"" << node << "\" -> \"" << dep << "\";\n";
        }
    }
    
    dot_file << "}\n";
}

bool DependencyGraph::HasTarget(const std::string& target) const {
    return graph_.find(target) != graph_.end();
}

const std::vector<std::string>& DependencyGraph::GetDirectDependencies(
    const std::string& target) const {
    static const std::vector<std::string> empty_deps;
    
    auto it = graph_.find(target);
    if (it != graph_.end()) {
        return it->second;
    }
    
    return empty_deps;
}