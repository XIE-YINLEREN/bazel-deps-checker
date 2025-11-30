#include "AdvancedBazelQueryParser.h"

AdvancedBazelQueryParser::AdvancedBazelQueryParser(
    const std::string& workspace_path, const std::string& bazel_binary)
    : workspace_path(workspace_path), bazel_binary(bazel_binary) {
    original_dir = std::filesystem::current_path().string();
}

std::unordered_map<std::string, BazelTarget> AdvancedBazelQueryParser::ParseWorkspace() {
    std::unordered_map<std::string, BazelTarget> targets;
    
    try {
        ChangeToWorkspaceDirectory();
        
        if (!ValidateBazelEnvironment()) {
            throw std::runtime_error("Bazel environment validation failed");
        }
        
        // 优先尝试一次性查询
        targets = ParseWithComprehensiveQuery();
        
    } catch (const std::exception& e) {
        std::cerr << "Comprehensive query failed: " << e.what() 
                  << ", falling back to individual queries" << std::endl;
        
        // 回退到逐个查询
        targets = ParseWithIndividualQueries();
    }
    
    RestoreOriginalDirectory();
    return targets;
}

void AdvancedBazelQueryParser::ChangeToWorkspaceDirectory() {
    if (!workspace_path.empty() && std::filesystem::exists(workspace_path)) {
        std::filesystem::current_path(workspace_path);
        LOG_INFO("Changed to workspace directory: " + workspace_path);
    }
}

void AdvancedBazelQueryParser::RestoreOriginalDirectory() {
    std::filesystem::current_path(original_dir);
    LOG_INFO("Restored original directory: " + original_dir);
}

bool AdvancedBazelQueryParser::ValidateBazelEnvironment() {
    try {
        std::string version_output = ExecuteBazelCommand("--version");
        LOG_INFO("Bazel version: " + version_output);
        
        std::string workspace_info = ExecuteBazelCommand("info workspace");
        LOG_INFO("Workspace info: " + workspace_info);
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Bazel environment validation failed: " + std::string(e.what()));
        return false;
    }
}

std::unordered_map<std::string, BazelTarget> AdvancedBazelQueryParser::ParseWithComprehensiveQuery() {
    std::unordered_map<std::string, BazelTarget> targets;
    
    std::string query = "query 'kind(\"cc_.* rule\", //...)' --output=label_kind";
    std::string output = ExecuteBazelCommand(query);
    
    std::vector<std::string> lines = SplitLines(output);
    
    for (const auto& line : lines) {
        if (line.empty()) continue;
        
        BazelTarget target = ParseTargetFromLabelKind(line);
        if (!target.empty()) {
            // 依赖源文件查询
            QueryTargetDetails(target);
        }
        targets.insert({target.full_label, target});
    }
    
    LOG_INFO("Comprehensive query found " + std::to_string(targets.size()) + " targets");
    return targets;
}

BazelTarget AdvancedBazelQueryParser::ParseTargetFromLabelKind(const std::string& line) {
    BazelTarget target;
    
    std::istringstream iss(line);
    std::string rule_type, rule_word, target_label;
    
    if (iss >> rule_type >> rule_word >> target_label) {
        target.rule_type = rule_type;
        
        // 解析目标标签
        size_t last_colon = target_label.find_last_of(':');
        if (last_colon != std::string::npos) {
            target.name = target_label.substr(last_colon + 1);
            target.path = target_label.substr(0, last_colon + 1);
        }
    }
    
    target.full_label = target_label;
 
    return target;
}

std::unordered_map<std::string, BazelTarget> AdvancedBazelQueryParser::ParseWithIndividualQueries() {
    std::unordered_map<std::string, BazelTarget> targets;
    
    try {
        // 获取所有C++相关的目标，而不是所有目标，提高效率
        std::string targets_output = ExecuteBazelCommand("query 'kind(\"cc_.* rule\", //...)' --output=label");
        std::vector<std::string> target_labels = SplitLines(targets_output);
        
        LOG_INFO("Found " + std::to_string(target_labels.size()) + " C++ targets to query individually");
        
        for (const auto& label : target_labels) {
            try {
                // 创建基础目标对象
                BazelTarget target;
                target.full_label = label;
                
                // 解析路径和名称
                size_t last_colon = label.find_last_of(':');
                if (last_colon != std::string::npos) {
                    target.path = label.substr(0, last_colon);
                    target.name = label.substr(last_colon + 1);
                } else {
                    target.path = label;
                    size_t last_slash = label.find_last_of('/');
                    target.name = (last_slash != std::string::npos) ? label.substr(last_slash + 1) : label;
                }
                
                // 查询目标详细信息
                QueryTargetDetails(target);
                
                if (!target.empty()) {
                    // 使用完整标签作为key，避免名称冲突
                    targets[target.full_label] = target;
                }
                
                // 添加小延迟避免过多请求
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to query target " + label + ": " + std::string(e.what()));
                continue;
            }
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to get target list: " + std::string(e.what()));
        // 如果C++目标查询失败，回退到查询所有目标
        return ParseAllTargetsFallback();
    }
    
    return targets;
}

std::unordered_map<std::string, BazelTarget> AdvancedBazelQueryParser::ParseAllTargetsFallback() {
    std::unordered_map<std::string, BazelTarget> targets;
    
    std::string targets_output = ExecuteBazelCommand("query '//...' --output=label");
    std::vector<std::string> target_labels = SplitLines(targets_output);
    
    LOG_INFO("Fallback: Found " + std::to_string(target_labels.size()) + " total targets to query");
    
    int processed = 0;
    for (const auto& label : target_labels) {
        try {
            // 只处理C++相关的目标以提高效率
            if (label.find("cc_") == std::string::npos) {
                continue;
            }
            
            BazelTarget target;
            target.full_label = label;
            
            size_t last_colon = label.find_last_of(':');
            if (last_colon != std::string::npos) {
                target.path = label.substr(0, last_colon);
                target.name = label.substr(last_colon + 1);
            }
            
            QueryTargetDetails(target);
            
            if (!target.empty()) {
                targets[target.full_label] = target;
            }
            
            processed++;
            if (processed % 50 == 0) {
                LOG_INFO("Processed " + std::to_string(processed) + " targets");
            }
            
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to query target " + label + ": " + std::string(e.what()));
            continue;
        }
    }
    
    return targets;
}

void AdvancedBazelQueryParser::QueryTargetDetails(BazelTarget& target) {
    try {
        std::string target_label = target.full_label.empty() ? 
            target.path + target.name : target.full_label;
        
        // 1. 查询规则类型
        if (target.rule_type.empty()) {
            try {
                std::string kind_query = "query 'kind(rule, " + target_label + ")' --output=label_kind";
                std::string kind_output = ExecuteBazelCommand(kind_query);
                target.rule_type = ExtractRuleType(kind_output);
            } catch (const std::exception& e) {
                LOG_WARN("Failed to query rule type for " + target_label + ": " + std::string(e.what()));
                target.rule_type = "unknown";
            }
        }
        
        // 2. 查询源文件
        try {
            std::string srcs_query = "query 'labels(srcs, " + target_label + ")' --output=label";
            std::string srcs_output = ExecuteBazelCommand(srcs_query);
            target.srcs = SplitLines(srcs_output);
            
            // 查询头文件
            std::string hdrs_query = "query 'labels(hdrs, " + target_label + ")' --output=label";
            std::string hdrs_output = ExecuteBazelCommand(hdrs_query);
            auto hdrs = SplitLines(hdrs_output);
            target.srcs.insert(target.srcs.end(), hdrs.begin(), hdrs.end());
        } catch (const std::exception& e) {
            LOG_WARN("Failed to query sources for " + target_label + ": " + std::string(e.what()));
        }
        
        // 3. 查询直接依赖
        try {
            std::string deps_query = "query 'kind(rule, deps(" + target_label + "))' --output=label";
            std::string deps_output = ExecuteBazelCommand(deps_query);
            target.deps = ExtractDependencies(target_label, deps_output);
        } catch (const std::exception& e) {
            LOG_WARN("Failed to query dependencies for " + target_label + ": " + std::string(e.what()));
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Comprehensive query failed for " + target.full_label + ": " + std::string(e.what()));
    }
}

std::string AdvancedBazelQueryParser::ExtractRuleType(const std::string& kind_output) {
    std::vector<std::string> lines = SplitLines(kind_output);
    for (const auto& line : lines) {
        std::istringstream iss(line);
        std::string rule_type, rule_word, target_label;
        if (iss >> rule_type >> rule_word >> target_label) {
            if (rule_word == "rule") {
                return rule_type;
            }
        }
    }
    return "unknown";
}

std::vector<std::string> AdvancedBazelQueryParser::ExtractDependencies(const std::string& target_label, const std::string& deps_output) {
    std::vector<std::string> deps;
    std::vector<std::string> lines = SplitLines(deps_output);
    
    for (const auto& line : lines) {
        if (line.find(target_label) != std::string::npos) {
            continue;
        }

        if (!line.empty()) {
            deps.push_back(line);
        }
    }
    
    return deps;
}

std::string AdvancedBazelQueryParser::ExtractTargetName(const std::string& target_label) {
    size_t last_colon = target_label.find_last_of(':');
    if (last_colon != std::string::npos) {
        return target_label.substr(last_colon + 1);
    }
    return target_label;
}

std::vector<std::string> AdvancedBazelQueryParser::SplitLines(const std::string& input) {
    std::vector<std::string> lines;
    std::istringstream iss(input);
    std::string line;
    
    while (std::getline(iss, line)) {
        if(line.find("Loading:") != std::string::npos) {
            continue;
        }

        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    
    return lines;
}

std::string AdvancedBazelQueryParser::ExecuteBazelCommand(const std::string& command) {
    std::string full_command = bazel_binary + " " + command;
    LOG_INFO("Executing Bazel command: " + full_command);
    return ExecuteSystemCommand(full_command);
}

std::string AdvancedBazelQueryParser::ExecuteSystemCommand(const std::string& command) {
    std::string full_command = command + " 2>&1";
    
    std::promise<std::string> promise;
    auto future = promise.get_future();
    
    std::thread([&promise, full_command]() {
        try {
            std::array<char, 128> buffer{};
            std::string result;
            
            #ifdef _WIN32
                std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(full_command.c_str(), "r"), _pclose);
            #else
                std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(full_command.c_str(), "r"), pclose);
            #endif
            
            if (!pipe) {
                throw std::runtime_error("popen() failed for command: " + full_command);
            }
            
            while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                result += buffer.data();
            }
            
            // 检查退出状态
            int exit_code = 0;
            #ifdef _WIN32
                exit_code = _pclose(pipe.release());
            #else
                exit_code = pclose(pipe.release());
            #endif
            
            if (exit_code != 0) {
                throw std::runtime_error("Command failed with exit code " + 
                                       std::to_string(exit_code) + ": " + result);
            }
            
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
        throw std::runtime_error("Command timeout: " + command);
    }
}