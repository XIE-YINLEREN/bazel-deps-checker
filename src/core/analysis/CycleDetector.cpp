#include "CycleDetector.h"
#include <algorithm>
#include <memory>
#include <sstream>
#include <unordered_set>
#include <string_view>
#include <iostream>

CycleDetector::CycleDetector(const DependencyGraph& graph, std::unordered_map<std::string, BazelTarget> targets) 
    : graph_(graph), targets_(std::move(targets)) {
    source_analyzer_ = std::make_shared<SourceAnalyzer>(targets_);
}

std::vector<CycleAnalysis> CycleDetector::AnalyzeCycles() {
    std::vector<CycleAnalysis> analyses;
    
    // 发现所有循环
    auto cycles = graph_.FindCycles();
    analyses.reserve(cycles.size());
    
    for (const auto& cycle : cycles) {
        if (cycle.size() >= 2) {
            analyses.push_back(ClassifyCycle(cycle));
        }
    }
    
    // 按循环大小排序，小的优先处理
    std::sort(analyses.begin(), analyses.end(), 
              [](const CycleAnalysis& a, const CycleAnalysis& b) {
                  return a.cycle.size() < b.cycle.size();
              });
    
    return analyses;
}

CycleAnalysis CycleDetector::ClassifyCycle(const std::vector<std::string>& cycle) const {
    CycleAnalysis analysis;
    analysis.cycle = cycle;
    analysis.cycle_type = DetermineBaseCycleType(cycle);
    
    // 设置额外分类标志
    analysis.contains_test_targets = ContainsTestTargets(cycle);
    analysis.contains_external_deps = ContainsExternalDeps(cycle);
    
    // 分析可移除的依赖
    AnalyzeRemovableDependencies(analysis);
    
    // 添加基础建议
    AddTypeSpecificSuggestions(analysis);
    
    // 应用额外分类
    ApplyAdditionalClassifications(analysis);
    
    return analysis;
}

void CycleDetector::AnalyzeRemovableDependencies(CycleAnalysis& analysis) const {
    std::vector<RemovableDependency> all_removable_deps;
    
    // 分析循环中的每条边
    for (size_t i = 0; i < analysis.cycle.size(); ++i) {
        std::string from = analysis.cycle[i];
        std::string to = analysis.cycle[(i + 1) % analysis.cycle.size()];
        
        // 检查目标是否存在
        if (targets_.find(from) == targets_.end() || targets_.find(to) == targets_.end()) {
            continue;
        }
        
        // 代码级别分析
        auto code_level_deps = AnalyzeDependencyAtCodeLevel(from, to);
        all_removable_deps.insert(all_removable_deps.end(), 
                                 code_level_deps.begin(), code_level_deps.end());
        
        // Target级别分析
        auto target_level_deps = AnalyzeDependencyAtTargetLevel(from, to);
        all_removable_deps.insert(all_removable_deps.end(), 
                                 target_level_deps.begin(), target_level_deps.end());
    }
    
    // 计算置信度并过滤
    for (auto& dep : all_removable_deps) {
        dep.confidence = CalculateConfidence(dep);
    }
    
    // 只保留高置信度的建议
    analysis.removable_dependencies.clear();
    std::copy_if(all_removable_deps.begin(), all_removable_deps.end(),
                 std::back_inserter(analysis.removable_dependencies),
                 [](const RemovableDependency& dep) {
                     return dep.confidence == ConfidenceLevel::HIGH;
                 });
    
    // 如果有可移除的依赖，添加相应的修复建议
    if (!analysis.removable_dependencies.empty()) {
        analysis.suggested_fixes.emplace_back("可以安全删除以下依赖来打破循环:");
        for (const auto& removable : analysis.removable_dependencies) {
            std::string suggestion = "  - " + removable.from_target + " -> " + removable.to_target;
            if (!removable.reason.empty()) {
                suggestion += " (" + removable.reason + ")";
            }
            analysis.suggested_fixes.push_back(suggestion);
        }
    }
}

std::vector<RemovableDependency> CycleDetector::AnalyzeDependencyAtCodeLevel(const std::string& from, const std::string& to) const {
    std::vector<RemovableDependency> results;
    
    if (!source_analyzer_) {
        return results;
    }
    
    try {
        // 使用 SourceAnalyzer 分析依赖
        auto removable_deps = source_analyzer_->GetRemovableDependencies(from);
        
        // 过滤出当前分析的依赖
        for (const auto& dep : removable_deps) {
            if (dep.to_target == to) {
                results.push_back(dep);
            }
        }
        
    } catch (const std::exception& e) {
        // 源代码分析可能失败，记录错误但不中断流程
        std::cerr << "代码级别分析失败 (" << from << " -> " << to << "): " << e.what() << std::endl;
    }
    
    return results;
}

std::vector<RemovableDependency> CycleDetector::AnalyzeDependencyAtTargetLevel(
    const std::string& from, const std::string& to) const {
    
    std::vector<RemovableDependency> results;
    
    // 检查目标是否存在
    auto from_it = targets_.find(from);
    auto to_it = targets_.find(to);
    if (from_it == targets_.end() || to_it == targets_.end()) {
        return results;
    }
    
    const auto& from_target = from_it->second;
    const auto& to_target = to_it->second;
    
    // 检查依赖是否真的在deps列表中
    bool dep_exists = std::find(from_target.deps.begin(), from_target.deps.end(), to) != from_target.deps.end();
    if (!dep_exists) {
        return results;
    }
    
    // 规则类型分析
    if (from_target.rule_type == "cc_library" && to_target.rule_type == "cc_library") {
        // 库到库的依赖，检查是否必要
        if (!IsCriticalDependency(from, to)) {
            RemovableDependency dep;
            dep.from_target = from;
            dep.to_target = to;
            dep.reason = "Target级别：存在其他依赖路径";
            results.push_back(dep);
        }
    }
    
    // 检查测试依赖
    if (from_target.rule_type.find("test") != std::string::npos && 
        to_target.rule_type == "cc_library") {
        // 测试目标依赖库，通常是必要的，但可以检查是否有过度依赖
        RemovableDependency dep;
        dep.from_target = from;
        dep.to_target = to;
        dep.reason = "Target级别：测试依赖可能过度";
        dep.confidence = ConfidenceLevel::MEDIUM;
        results.push_back(dep);
    }
    
    // 检查二进制目标依赖
    if (from_target.rule_type == "cc_binary" && to_target.rule_type == "cc_library") {
        // 二进制文件依赖库，通常是必要的
        // 但可以检查是否有未使用的依赖
    }
    
    return results;
}

ConfidenceLevel CycleDetector::CalculateConfidence(const RemovableDependency& dep) const {
    // 根据证据数量和分析层面计算置信度
    
    if (!dep.unused_headers.empty() || !dep.unused_symbols.empty()) {
        // 有具体的代码级别证据
        return ConfidenceLevel::HIGH;
    }
    
    if (dep.reason.find("代码级别") != std::string::npos) {
        return ConfidenceLevel::HIGH;
    }
    
    if (dep.reason.find("Target级别") != std::string::npos) {
        // Target级别分析通常置信度较低
        return ConfidenceLevel::MEDIUM;
    }
    
    return ConfidenceLevel::LOW;
}

bool CycleDetector::IsCriticalDependency(const std::string& from, const std::string& to) const {
    // 获取from的所有依赖（除了to）
    auto deps = graph_.GetDirectDependencies(from);
    if (deps.empty()) {
        return true;
    }
    
    // 检查是否有其他路径可以到达to
    auto transitive_deps = graph_.GetTransitiveDependencies(from);
    
    // 如果to只能通过直接依赖到达，那么这个依赖可能是关键的
    bool has_alternative_path = false;
    for (const auto& dep : deps) {
        if (dep != to) {
            auto dep_transitive = graph_.GetTransitiveDependencies(dep);
            if (dep_transitive.find(to) != dep_transitive.end()) {
                has_alternative_path = true;
                break;
            }
        }
    }
    
    return !has_alternative_path;
}

CycleType CycleDetector::DetermineBaseCycleType(const std::vector<std::string>& cycle) const {
    if (IsDirectCycle(cycle)) {
        return CycleType::DIRECT_CYCLE;
    } else if (IsDiamondDependency(cycle)) {
        return CycleType::DIAMOND_DEPENDENCY;
    } else if (cycle.size() > 3) {
        return CycleType::COMPLEX_CYCLE;
    } else {
        return CycleType::SIMPLE_CYCLE;
    }
}

void CycleDetector::ApplyAdditionalClassifications(CycleAnalysis& analysis) const {
    if (analysis.contains_test_targets) {
        analysis.suggested_fixes.emplace_back("将测试依赖移到testonly目标");
        analysis.suggested_fixes.emplace_back("使用测试桩(stub)代替直接依赖");
    }
    
    if (analysis.contains_external_deps) {
        analysis.suggested_fixes.emplace_back("检查外部依赖版本兼容性");
        analysis.suggested_fixes.emplace_back("考虑使用不同的外部依赖版本");
    }
}

std::string CycleDetector::GenerateFixSuggestion(const CycleAnalysis& analysis) const {
    std::ostringstream ss;
    
    ss << "循环依赖分析报告:\n"
       << "循环类型: " << CycleTypeToString(analysis.cycle_type) << "\n"
       << "涉及目标 (" << analysis.cycle.size() << "个): ";
    
    for (size_t i = 0; i < analysis.cycle.size(); ++i) {
        ss << analysis.cycle[i];
        if (i < analysis.cycle.size() - 1) ss << " -> ";
    }
    ss << "\n\n";
    
    // 显示目标详细信息
    ss << "目标详情:\n";
    for (const auto& target_name : analysis.cycle) {
        auto it = targets_.find(target_name);
        if (it != targets_.end()) {
            const auto& target = it->second;
            ss << "  - " << target_name << " (" << target.rule_type << ")\n";
            ss << "    路径: " << target.path << "\n";
            ss << "    源文件: " << target.srcs.size() << " 个\n";
            ss << "    依赖: " << target.deps.size() << " 个\n";
        }
    }
    ss << "\n";
    
    ss << "建议修复方案:\n";
    for (size_t i = 0; i < analysis.suggested_fixes.size(); ++i) {
        ss << i + 1 << ". " << analysis.suggested_fixes[i] << "\n";
    }
    
    // 如果有可移除的依赖，特别强调
    if (!analysis.removable_dependencies.empty()) {
        ss << "\n🔧 快速修复 - 可以安全删除的依赖:\n";
        for (const auto& removable : analysis.removable_dependencies) {
            ss << "   - 删除依赖: " << removable.from_target << " -> " << removable.to_target;
            if (!removable.reason.empty()) {
                ss << " (" << removable.reason << ")";
            }
            ss << "\n";
            
            // 显示未使用的头文件
            if (!removable.unused_headers.empty()) {
                ss << "     未使用头文件: ";
                for (size_t i = 0; i < removable.unused_headers.size(); ++i) {
                    if (i > 0) ss << ", ";
                    ss << removable.unused_headers[i];
                }
                ss << "\n";
            }
            
            // 显示未使用的符号
            if (!removable.unused_symbols.empty()) {
                ss << "     未使用符号: ";
                for (size_t i = 0; i < removable.unused_symbols.size(); ++i) {
                    if (i > 0) ss << ", ";
                    ss << removable.unused_symbols[i];
                }
                ss << "\n";
            }
        }
        ss << "   删除上述任一依赖即可打破循环\n";
    }
    
    ss << "\n详细建议:\n";
    
    // 根据循环类型提供具体建议
    switch (analysis.cycle_type) {
        case CycleType::DIRECT_CYCLE:
            ss << "直接循环依赖通常可以通过以下方式解决:\n"
               << "- 将" << analysis.cycle[0] << "和" << analysis.cycle[1] 
               << "的公共部分提取到新库\n"
               << "- 使用接口抽象来解耦双向依赖\n";
            
            if (auto common_interface = ExtractCommonInterface(analysis.cycle); 
                !common_interface.empty()) {
                ss << "- 建议提取公共接口: " << common_interface << "\n";
            }
            break;
            
        case CycleType::DIAMOND_DEPENDENCY:
            ss << "菱形依赖解决方案:\n"
               << "- 识别公共依赖并提取基础模块\n"
               << "- 使用依赖注入模式\n";
            break;
            
        case CycleType::SIMPLE_CYCLE:
            if (analysis.cycle.size() == 2) {
                ss << "双目标循环的快速修复:\n"
                   << "1. 分析" << analysis.cycle[0] << "和" << analysis.cycle[1] 
                   << "的依赖关系\n"
                   << "2. 确定哪个依赖是不必要的\n"
                   << "3. 修改BUILD文件移除错误依赖\n";
            }
            break;
            
        case CycleType::COMPLEX_CYCLE:
            ss << "复杂循环建议:\n"
               << "- 分析依赖关系，识别核心问题节点\n"
               << "- 考虑模块重构\n"
               << "- 引入中介者模式\n";
            break;
            
        default:
            break;
    }
    
    return ss.str();
}

bool CycleDetector::IsDirectCycle(const std::vector<std::string>& cycle) const {
    if (cycle.size() != 2) return false;
    
    const auto& a = cycle[0];
    const auto& b = cycle[1];
    
    auto deps_a = graph_.GetTransitiveDependencies(a);
    auto deps_b = graph_.GetTransitiveDependencies(b);
    
    return deps_b.find(a) != deps_b.end() && deps_a.find(b) != deps_a.end();
}

bool CycleDetector::IsDiamondDependency(const std::vector<std::string>& cycle) const {
    if (cycle.size() < 4) return false;
    
    // 检查是否存在多个路径到达同一个节点
    for (const auto& node : cycle) {
        auto deps = graph_.GetTransitiveDependencies(node);
        int reachable_count = 0;
        
        for (const auto& other : cycle) {
            if (node != other && deps.find(other) != deps.end()) {
                ++reachable_count;
            }
        }
        
        // 如果某个节点能到达多个循环中的其他节点，可能是汇聚点
        if (reachable_count >= 2) {
            return true;
        }
    }
    
    return false;
}

std::string CycleDetector::ExtractCommonInterface(const std::vector<std::string>& targets) const {
    if (targets.empty()) return "";
    
    // 找出共同路径前缀
    std::string first_target = targets[0];
    size_t last_slash = first_target.find_last_of('/');
    
    if (last_slash == std::string::npos) 
        return "//common:interface";
    
    std::string base_path = first_target.substr(0, last_slash);
    size_t last_colon = base_path.find_last_of(':');
    
    std::string package_name = (last_colon != std::string::npos) 
        ? base_path.substr(last_colon + 1) 
        : base_path;
    
    return package_name.empty() 
        ? "//common:interface" 
        : base_path + ":" + package_name + "_interface";
}

bool CycleDetector::ContainsTestTargets(const std::vector<std::string>& cycle) const {
    return std::any_of(cycle.begin(), cycle.end(), [this](const std::string& target) {
        auto it = targets_.find(target);
        if (it != targets_.end()) {
            const auto& target_info = it->second;
            return target_info.rule_type.find("test") != std::string::npos ||
                   target_info.name.find("_test") != std::string::npos ||
                   target_info.name.find("test_") != std::string::npos;
        }
        return false;
    });
}

bool CycleDetector::ContainsExternalDeps(const std::vector<std::string>& cycle) const {
    return std::any_of(cycle.begin(), cycle.end(), [](const std::string& target) {
        return target.find('@') != std::string::npos;
    });
}

std::string CycleDetector::CycleTypeToString(CycleType type) const {
    switch (type) {
        case CycleType::DIRECT_CYCLE: return "DIRECT_CYCLE";
        case CycleType::DIAMOND_DEPENDENCY: return "DIAMOND_DEPENDENCY";
        case CycleType::COMPLEX_CYCLE: return "COMPLEX_CYCLE";
        case CycleType::SIMPLE_CYCLE: return "SIMPLE_CYCLE";
        default: return "UNKNOWN";
    }
}

void CycleDetector::AddTypeSpecificSuggestions(CycleAnalysis& analysis) const {
    switch (analysis.cycle_type) {
        case CycleType::DIRECT_CYCLE:
            analysis.suggested_fixes = {
                "提取公共接口到新库",
                "使用前向声明减少头文件依赖", 
                "重构代码消除双向依赖"
            };
            break;
            
        case CycleType::DIAMOND_DEPENDENCY:
            analysis.suggested_fixes = {
                "引入接口层抽象",
                "使用依赖倒置原则",
                "提取公共基础库"
            };
            break;
            
        case CycleType::COMPLEX_CYCLE:
            analysis.suggested_fixes = {
                "分析依赖关系，识别核心问题节点",
                "考虑模块重构", 
                "引入中介者模式"
            };
            break;
            
        case CycleType::SIMPLE_CYCLE:
            analysis.suggested_fixes = {
                "检查依赖声明是否正确",
                "移除不必要的依赖",
                "重新组织代码结构"
            };
            break;
            
        default:
            analysis.suggested_fixes = {"检查依赖关系"};
            break;
    }
}
