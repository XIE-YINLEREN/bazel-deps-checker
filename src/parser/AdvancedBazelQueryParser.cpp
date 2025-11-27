#include "AdvancedBazelQueryParser.h"



AdvancedBazelQueryParser::AdvancedBazelQueryParser(const std::string& workspace_path,
                           const std::string& bazel_binary)
        : workspace_path(workspace_path), bazel_binary(bazel_binary) {
    original_dir = std::filesystem::current_path().string();
}


std::unordered_map<std::string, BazelTarget> AdvancedBazelQueryParser::ParseWorkspace() {
    std::unordered_map<std::string, BazelTarget> targets;

    if (!workspace_path.empty() && std::filesystem::exists(workspace_path)) {
        std::filesystem::current_path(workspace_path);
    }

    LOG_INFO("Executing Bazel command from: " + workspace_path);

    // 一次性查询所有需要的信息
    std::string comprehensive_query{};
    
    comprehensive_query += "//... --output=package,label,kind,deps,srcs 2>/dev/null || "
        "bazel query \"//...\" --output=label_kind";
        
    try {
        std::string output = ExecuteBazelCommand(comprehensive_query);
        targets = ParseComprehensiveOutput(output);
    } catch (const std::exception& e) {
        std::cerr << "Comprehensive query failed, falling back to individual queries: " 
                    << e.what() << std::endl;
        // 回退到逐个查询
        targets = ParseWithIndividualQueries();
    }
    
    std::filesystem::current_path(original_dir);

    return targets;
}


std::string AdvancedBazelQueryParser::ExecuteBazelCommand(const std::string& command) {
    std::string full_command = bazel_binary + " " + command + " 2>&1";
    
    std::promise<std::string> promise;
    auto future = promise.get_future();
    
    std::thread([&promise, full_command]() {
        try {
            std::string result = ExecuteCommand(full_command);
            promise.set_value(result);
        } catch (...) {
            promise.set_exception(std::current_exception());
        }
    }).detach();
    
    // 等待结果，支持超时
    auto status = future.wait_for(std::chrono::seconds(timeout_seconds));
    if (status == std::future_status::ready) {
        return future.get();
    } else {
        throw std::runtime_error("Bazel command timeout: " + command);
    }
}


std::string AdvancedBazelQueryParser::ExecuteCommand(const std::string& command) {
    std::array<char, 128> buffer{};
    std::string result;
    
    #ifdef _WIN32
        std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
    #else
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    #endif
    
    if (!pipe) {
        throw std::runtime_error("popen() failed for command: " + command);
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    // 检查命令是否成功执行（简单检查）
    if (result.find("ERROR") != std::string::npos || 
        result.find("command not found") != std::string::npos) {
        throw std::runtime_error("Bazel command failed: " + result);
    }
    
    return result;
}

std::unordered_map<std::string, BazelTarget> AdvancedBazelQueryParser::ParseComprehensiveOutput(const std::string& output) {
    std::unordered_map<std::string, BazelTarget> targets;
    std::istringstream iss(output);
    std::string line;
    
    BazelTarget current_target;
    
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        
        // 解析不同格式的输出
        if (line.find("cc_library") != std::string::npos ||
            line.find("cc_binary") != std::string::npos ||
            line.find("cc_test") != std::string::npos) {
            
            // 处理规则行
            if (!current_target.name.empty()) {
                targets[current_target.name] = current_target;
            }
            
            current_target = ParseTargetFromOutput(line);
        } else if (!current_target.name.empty()) {
            // 处理依赖或源文件行
            if (line.find("//") == 0) {
                current_target.deps.push_back(ExtractTargetName(line));
            } else if (line.find(".cpp") != std::string::npos ||
                        line.find(".cc") != std::string::npos ||
                        line.find(".h") != std::string::npos) {
                current_target.srcs.push_back(line);
            }
        }
    }
    
    // 添加最后一个目标
    if (!current_target.name.empty()) {
        targets[current_target.name] = current_target;
    }
    
    return targets;
}

BazelTarget AdvancedBazelQueryParser::ParseTargetFromOutput(const std::string& line) {
    BazelTarget target;
    
    // 解析类似 "cc_library //path/to:target" 的格式
    std::istringstream iss(line);
    std::string rule_type, target_label;
    
    if (iss >> rule_type >> target_label) {
        target.rule_type = rule_type;
        
        // 解析目标标签
        size_t last_colon = target_label.find_last_of(':');
        if (last_colon != std::string::npos) {
            target.name = target_label.substr(last_colon + 1);
            target.path = target_label.substr(2, last_colon - 2); // 移除 "//" 前缀
        }
    }
    
    return target;
}

std::string AdvancedBazelQueryParser::ExtractTargetName(const std::string& target_label) {
    size_t last_colon = target_label.find_last_of(':');
    if (last_colon != std::string::npos) {
        return target_label.substr(last_colon + 1);
    }
    return target_label;
}

std::unordered_map<std::string, BazelTarget> AdvancedBazelQueryParser::ParseWithIndividualQueries() {
    std::unordered_map<std::string, BazelTarget> targets;
    
    // 获取所有目标标签
    std::string targets_output = ExecuteBazelCommand("query '//...' --output=label");
    std::vector<std::string> target_labels = SplitOutput(targets_output);
    
    for (const auto& label : target_labels) {
        BazelTarget target = QuerySingleTargetDetails(label);
        if (!target.name.empty()) {
            targets[target.name] = target;
        }
    }
    
    return targets;
}

BazelTarget AdvancedBazelQueryParser::QuerySingleTargetDetails(const std::string& target_label) {
    BazelTarget target;
    
    // 设置基本目标信息
    size_t last_colon = target_label.find_last_of(':');
    if (last_colon != std::string::npos) {
        target.name = target_label.substr(last_colon + 1);
        target.path = target_label.substr(2, last_colon - 2);
    }
    
    // 查询规则类型
    std::string kind_query = "query 'kind(rule, " + target_label + ")' --output=label_kind";
    std::string kind_output = ExecuteBazelCommand(kind_query);
    target.rule_type = ExtractRuleTypeFromKind(kind_output);
    
    // 查询直接依赖
    std::string deps_query = "query 'deps(" + target_label + ", 1)' --output=label";
    std::string deps_output = ExecuteBazelCommand(deps_query);
    target.deps = ExtractDependencies(deps_output, target_label);
    
    return target;
}

std::string AdvancedBazelQueryParser::ExtractRuleTypeFromKind(const std::string& kind_output) {
    std::istringstream iss(kind_output);
    std::string line;
    if (std::getline(iss, line)) {
        std::istringstream line_stream(line);
        std::string rule_type, label;
        if (line_stream >> rule_type >> label) {
            return rule_type;
        }
    }
    return "unknown";
}

std::vector<std::string> AdvancedBazelQueryParser::ExtractDependencies(const std::string& deps_output, 
                                            const std::string& current_target) {
    std::vector<std::string> deps;
    std::vector<std::string> lines = SplitOutput(deps_output);
    
    for (const auto& line : lines) {
        if (line != current_target && !line.empty()) {
            deps.push_back(ExtractTargetName(line));
        }
    }
    
    return deps;
}

std::vector<std::string> AdvancedBazelQueryParser::SplitOutput(const std::string& output) {
    std::vector<std::string> lines;
    std::istringstream iss(output);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    
    return lines;
}