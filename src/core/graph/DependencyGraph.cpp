#include "DependencyGraph.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

DependencyGraph::DependencyGraph(const std::unordered_map<std::string, BazelTarget>& targets)
    : source_analyzer_(nullptr), graph_targets_(targets) {
    BuildGraph();
    BuildReverseDependencies();
}

void DependencyGraph::SetSourceAnalyzer(SourceAnalyzer* source_analyzer) const {
    source_analyzer_ = source_analyzer;
}

void DependencyGraph::BuildGraph() {
    graph_.clear();
    adjacency_set_.clear();
    node_names_.clear();
    node_ids_.clear();
    adjacency_ids_.clear();
    
    for (const auto& [name, target] : graph_targets_) {
        const size_t from_id = GetOrCreateNodeId(name);
        std::vector<std::string> dependencies;
        std::unordered_set<std::string> dependency_set;
        dependencies.reserve(target.deps.size());
        dependency_set.reserve(target.deps.size());
        
        for (const auto& dep : target.deps) {
            std::string simplified_dep = SimplifyDependencyName(dep);
            if (!simplified_dep.empty() && simplified_dep.find("@") == std::string::npos) {
                const size_t to_id = GetOrCreateNodeId(simplified_dep);
                dependency_set.insert(simplified_dep);
                dependencies.push_back(std::move(simplified_dep));
                adjacency_ids_[from_id].push_back(to_id);
            }
        }
        
        graph_[name] = std::move(dependencies);
        adjacency_set_[name] = std::move(dependency_set);
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

size_t DependencyGraph::GetOrCreateNodeId(const std::string& node_name) {
    const auto it = node_ids_.find(node_name);
    if (it != node_ids_.end()) {
        return it->second;
    }

    const size_t new_id = node_names_.size();
    node_names_.push_back(node_name);
    node_ids_.emplace(node_name, new_id);
    adjacency_ids_.emplace_back();
    return new_id;
}

size_t DependencyGraph::GetNodeId(const std::string& node_name) const {
    const auto it = node_ids_.find(node_name);
    return it == node_ids_.end() ? static_cast<size_t>(-1) : it->second;
}

std::vector<std::vector<std::string>> DependencyGraph::FindCycles() const {
    std::vector<std::vector<std::string>> cycles;
    std::unordered_map<std::string, int> color;
    std::unordered_map<std::string, std::string> parent;
    std::set<std::string> cycle_signatures;
    color.reserve(graph_.size());
    parent.reserve(graph_.size());

    const auto components = FindStronglyConnectedComponents();
    for (const auto& component : components) {
        if (component.empty()) {
            continue;
        }

        if (component.size() == 1) {
            const auto only_node_it = component.begin();
            auto graph_it = graph_.find(*only_node_it);
            if (graph_it == graph_.end() ||
                std::find(graph_it->second.begin(), graph_it->second.end(), *only_node_it) ==
                    graph_it->second.end()) {
                continue;
            }
            std::vector<std::string> self_cycle = {*only_node_it};
            const std::string signature = CanonicalizeCycle(self_cycle);
            if (cycle_signatures.insert(signature).second) {
                cycles.push_back(std::move(self_cycle));
            }
            continue;
        }

        if (component.size() == 2) {
            auto it = component.begin();
            const std::string first = *it;
            ++it;
            const std::string second = *it;
            if (HasDirectEdge(first, second) && HasDirectEdge(second, first)) {
                std::vector<std::string> cycle = {first, second};
                const std::string signature = CanonicalizeCycle(cycle);
                if (cycle_signatures.insert(signature).second) {
                    cycles.push_back(std::move(cycle));
                }
                continue;
            }
        }

        for (const auto& node : component) {
            if (color[node] == 0) {
                FindCyclesDFS(node, color, parent, cycles, cycle_signatures, &component);
            }
        }
    }

    return cycles;
}

void DependencyGraph::FindCyclesDFS(
    const std::string& node,
    std::unordered_map<std::string, int>& color,
    std::unordered_map<std::string, std::string>& parent,
    std::vector<std::vector<std::string>>& cycles,
    std::set<std::string>& cycle_signatures,
    const std::unordered_set<std::string>* allowed_nodes) const {
    
    color[node] = 1; 
    
    auto it = graph_.find(node);
    if (it != graph_.end()) {
        for (const auto& neighbor : it->second) {
            if (allowed_nodes && allowed_nodes->find(neighbor) == allowed_nodes->end()) {
                continue;
            }
            if (color[neighbor] == 0) {
                parent[neighbor] = node;
                FindCyclesDFS(neighbor, color, parent, cycles, cycle_signatures, allowed_nodes);
            } else if (color[neighbor] == 1) {
                std::vector<std::string> cycle;
                std::string current = node;
                
                while (current != neighbor) {
                    cycle.push_back(current);
                    current = parent[current];
                }
                cycle.push_back(neighbor);
                cycle.push_back(node);
                
                std::reverse(cycle.begin(), cycle.end());
                const std::string signature = CanonicalizeCycle(cycle);
                if (cycle_signatures.insert(signature).second) {
                    cycles.push_back(std::move(cycle));
                }
            }
        }
    }
    
    color[node] = 2; 
}

std::vector<std::unordered_set<std::string>> DependencyGraph::FindStronglyConnectedComponents() const {
    std::vector<std::unordered_set<std::string>> components;
    std::unordered_map<std::string, int> index_map;
    std::unordered_map<std::string, int> low_link;
    std::vector<std::string> stack;
    std::unordered_set<std::string> on_stack;
    int current_index = 0;

    index_map.reserve(graph_.size());
    low_link.reserve(graph_.size());
    stack.reserve(graph_.size());
    on_stack.reserve(graph_.size());

    for (const auto& [node, _] : graph_) {
        if (index_map.find(node) == index_map.end()) {
            FindStronglyConnectedComponentsDFS(
                node, current_index, index_map, low_link, stack, on_stack, components);
        }
    }

    return components;
}

void DependencyGraph::FindStronglyConnectedComponentsDFS(
    const std::string& node,
    int& current_index,
    std::unordered_map<std::string, int>& index_map,
    std::unordered_map<std::string, int>& low_link,
    std::vector<std::string>& stack,
    std::unordered_set<std::string>& on_stack,
    std::vector<std::unordered_set<std::string>>& components) const {
    index_map[node] = current_index;
    low_link[node] = current_index;
    ++current_index;
    stack.push_back(node);
    on_stack.insert(node);

    auto graph_it = graph_.find(node);
    if (graph_it != graph_.end()) {
        for (const auto& neighbor : graph_it->second) {
            if (index_map.find(neighbor) == index_map.end()) {
                FindStronglyConnectedComponentsDFS(
                    neighbor, current_index, index_map, low_link, stack, on_stack, components);
                low_link[node] = std::min(low_link[node], low_link[neighbor]);
            } else if (on_stack.find(neighbor) != on_stack.end()) {
                low_link[node] = std::min(low_link[node], index_map[neighbor]);
            }
        }
    }

    if (low_link[node] != index_map[node]) {
        return;
    }

    std::unordered_set<std::string> component;
    while (!stack.empty()) {
        std::string current = std::move(stack.back());
        stack.pop_back();
        on_stack.erase(current);
        component.insert(current);
        if (current == node) {
            break;
        }
    }
    components.push_back(std::move(component));
}

bool DependencyGraph::HasDirectEdge(const std::string& from, const std::string& to) const {
    auto adjacency_it = adjacency_set_.find(from);
    if (adjacency_it == adjacency_set_.end()) {
        return false;
    }
    return adjacency_it->second.find(to) != adjacency_it->second.end();
}

std::unordered_set<std::string> DependencyGraph::GetTransitiveDependencies(
    const std::string& target) const {
    return GetTransitiveDependenciesRef(target);
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
    const std::string cache_key = target + '\n' + dependency;
    auto cached = dependency_need_cache_.find(cache_key);
    if (cached != dependency_need_cache_.end()) {
        return cached->second;
    }

    // 检查目标的直接依赖是否依赖这个库
    auto target_deps_it = graph_.find(target);
    if (target_deps_it == graph_.end()) {
        return false;
    }
    
    for (const auto& direct_dep : target_deps_it->second) {
        if (direct_dep == dependency) continue;
        
        // 获取直接依赖的传递依赖
        const auto& transitive_deps = GetTransitiveDependenciesRef(direct_dep);
        if (transitive_deps.find(dependency) != transitive_deps.end()) {
            // 检查这个直接依赖是否真正需要该依赖
            if (source_analyzer_) {
                if (source_analyzer_->IsDependencyNeeded(direct_dep, dependency)) {
                    // 直接依赖真正需要这个依赖，所以目标需要传递声明
                    dependency_need_cache_[cache_key] = true;
                    return true;
                }
            }
        }
    }
    
    dependency_need_cache_[cache_key] = false;
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
    const auto& transitive_deps = GetTransitiveDependenciesRef(target);
    (void)transitive_deps;
    
    // 检查每个直接依赖是否可以被省略
    for (const auto& direct_dep : it->second) {
        // 检查这个直接依赖是否已经通过其他路径成为传递依赖
        for (const auto& other_dep : it->second) {
            if (other_dep == direct_dep) continue;
            
            // 获取其他依赖的传递依赖
            const auto& other_transitive = GetTransitiveDependenciesRef(other_dep);
            if (other_transitive.find(direct_dep) != other_transitive.end()) {
                // 这个依赖已经通过其他路径传递了
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

const std::unordered_set<std::string>& DependencyGraph::GetTransitiveDependenciesRef(
    const std::string& target) const {
    auto cache_it = transitive_deps_cache_.find(target);
    if (cache_it != transitive_deps_cache_.end()) {
        return cache_it->second;
    }

    const size_t target_id = GetNodeId(target);
    if (target_id == static_cast<size_t>(-1) || target_id >= adjacency_ids_.size()) {
        return empty_dependency_set_;
    }

    std::unordered_set<std::string> transitive_deps;
    std::queue<size_t> queue;
    std::unordered_set<size_t> visited_ids;

    for (const size_t dep_id : adjacency_ids_[target_id]) {
        queue.push(dep_id);
    }

    while (!queue.empty()) {
        const size_t current = queue.front();
        queue.pop();

        if (!visited_ids.insert(current).second) {
            continue;
        }

        if (current >= node_names_.size()) {
            continue;
        }
        transitive_deps.insert(node_names_[current]);

        if (current >= adjacency_ids_.size()) {
            continue;
        }

        for (const size_t dep_id : adjacency_ids_[current]) {
            if (visited_ids.find(dep_id) == visited_ids.end()) {
                queue.push(dep_id);
            }
        }
    }

    auto [inserted_it, _] = transitive_deps_cache_.emplace(target, std::move(transitive_deps));
    return inserted_it->second;
}

std::string DependencyGraph::CanonicalizeCycle(const std::vector<std::string>& cycle) const {
    if (cycle.empty()) {
        return "";
    }

    std::vector<std::string> normalized = cycle;
    if (normalized.size() > 1 && normalized.front() == normalized.back()) {
        normalized.pop_back();
    }

    if (normalized.empty()) {
        return "";
    }

    auto min_it = std::min_element(normalized.begin(), normalized.end());
    std::rotate(normalized.begin(), min_it, normalized.end());

    std::ostringstream os;
    for (size_t index = 0; index < normalized.size(); ++index) {
        if (index > 0) {
            os << "->";
        }
        os << normalized[index];
    }
    return os.str();
}
