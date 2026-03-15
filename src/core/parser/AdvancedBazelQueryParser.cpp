#include "AdvancedBazelQueryParser.h"
#include "pipe.h"
#include <filesystem>
#include <future>
#include <mutex>
#include <functional>
#include <memory>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct ParsedWorkspaceCacheEntry {
    std::shared_ptr<std::unordered_map<std::string, BazelTarget>> targets;
    std::string fingerprint;
};

std::mutex& GetParsedWorkspaceCacheMutex() {
    static std::mutex mutex;
    return mutex;
}

std::unordered_map<std::string, ParsedWorkspaceCacheEntry>& GetParsedWorkspaceCache() {
    static std::unordered_map<std::string, ParsedWorkspaceCacheEntry> cache;
    return cache;
}

std::string BuildWorkspaceFingerprint(const std::string& workspace_path) {
    fs::path workspace(workspace_path);
    if (!fs::exists(workspace) || !fs::is_directory(workspace)) {
        return "missing";
    }

    uintmax_t file_count = 0;
    long long latest_write = 0;
    auto update_from_path = [&](const fs::path& path) {
        std::error_code ec;
        const auto time = fs::last_write_time(path, ec);
        if (ec) {
            return;
        }
        const auto raw = time.time_since_epoch().count();
        if (raw > latest_write) {
            latest_write = raw;
        }
        ++file_count;
    };

    for (const auto& name : {"WORKSPACE", "WORKSPACE.bazel", "MODULE.bazel"}) {
        fs::path path = workspace / name;
        if (fs::exists(path)) {
            update_from_path(path);
        }
    }

    std::error_code ec;
    for (fs::recursive_directory_iterator it(workspace, ec), end; it != end; it.increment(ec)) {
        if (ec) {
            break;
        }
        if (!it->is_regular_file()) {
            continue;
        }

        const std::string filename = it->path().filename().string();
        if (filename == "BUILD" || filename == "BUILD.bazel") {
            update_from_path(it->path());
        }
    }

    return std::to_string(file_count) + ":" + std::to_string(latest_write);
}

std::string BuildParserCacheKey(const std::string& workspace_path, const std::string& bazel_binary) {
    return workspace_path + '\n' + bazel_binary;
}

}  // namespace

void AdvancedBazelQueryParser::ClearWorkspaceCache() {
    std::lock_guard<std::mutex> lock(GetParsedWorkspaceCacheMutex());
    GetParsedWorkspaceCache().clear();
}

size_t AdvancedBazelQueryParser::GetWorkspaceCacheSize() {
    std::lock_guard<std::mutex> lock(GetParsedWorkspaceCacheMutex());
    return GetParsedWorkspaceCache().size();
}

AdvancedBazelQueryParser::AdvancedBazelQueryParser(
    const std::string& workspace_path, const std::string& bazel_binary)
    : workspace_path(workspace_path), bazel_binary(bazel_binary) {
    original_dir = fs::current_path().string();
    query_etr_command = " --keep_going --incompatible_disallow_empty_glob=false ";
}

std::unordered_map<std::string, BazelTarget> AdvancedBazelQueryParser::ParseWorkspace() {
    std::unordered_map<std::string, BazelTarget> targets;
    const std::string cache_key = BuildParserCacheKey(workspace_path, bazel_binary);
    const std::string fingerprint = BuildWorkspaceFingerprint(workspace_path);

    {
        std::lock_guard<std::mutex> lock(GetParsedWorkspaceCacheMutex());
        auto& cache = GetParsedWorkspaceCache();
        auto it = cache.find(cache_key);
        if (it != cache.end() && it->second.fingerprint == fingerprint && it->second.targets) {
            LOG_INFO("Reusing cached parsed workspace targets");
            return *it->second.targets;
        }
    }
    
    try {
        ChangeToWorkspaceDirectory();
        
        if (!ValidateBazelEnvironment()) {
            throw std::runtime_error("Bazel environment validation failed");
        }
        
        // 优先尝试一次性查询
        targets = ParseWithComprehensiveQuery();
        
    } catch (const std::exception& e) {
        LOG_WARN("Comprehensive query failed: " + std::string(e.what()) +
                 ", falling back to concurrent queries");
        
        // 回退到并发查询
        targets = ParseWithConcurrentQueries();
    }

    RestoreOriginalDirectory();

    {
        std::lock_guard<std::mutex> lock(GetParsedWorkspaceCacheMutex());
        GetParsedWorkspaceCache()[cache_key] =
            ParsedWorkspaceCacheEntry{
                std::make_shared<std::unordered_map<std::string, BazelTarget>>(targets),
                fingerprint};
    }

    return targets;
}

void AdvancedBazelQueryParser::ChangeToWorkspaceDirectory() {
    if (!workspace_path.empty() && fs::exists(workspace_path)) {
        fs::current_path(workspace_path);
        LOG_INFO("Changed to workspace directory: " + workspace_path);
    }
}

void AdvancedBazelQueryParser::RestoreOriginalDirectory() {
    fs::current_path(original_dir);
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
    
    std::string query = "query 'kind(\"cc_.* rule\", //...)' --output=label_kind" + query_etr_command;
    std::string output = ExecuteBazelCommand(query);
    
    std::vector<std::string> lines = SplitLines(output);
    
    for (const auto& line : lines) {
        if (line.empty()) continue;
        
        BazelTarget target = ParseTargetFromLabelKind(line);
        if (target.empty()) {
            continue;
        }

        QueryTargetDetails(target);
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
            std::string target_path = target_label.substr(0, last_colon);
            
            target.path = ConvertBazelLabelToPath(target_path);
        } else {
            target.path = ConvertBazelLabelToPath(target_label);
            target.name = target_label;
        }
    }
    
    target.full_label = target_label;
 
    return target;
}

void AdvancedBazelQueryParser::QueryTargetDetails(BazelTarget& target) {
    try {
        std::string target_label = target.full_label.empty() ? 
            target.path + target.name : target.full_label;
        try {
            std::string unified_query =
                "query 'kind(\".* rule\", " + target_label + ") "
                "union kind(\".* rule\", deps(" + target_label + ", 1)) "
                "union labels(srcs, " + target_label + ") "
                "union labels(hdrs, " + target_label + ")' "
                "--output=label_kind" + query_etr_command;

            std::string unified_output = ExecuteBazelCommand(unified_query);
            std::vector<std::string> lines = SplitLines(unified_output);

            for (const auto& line : lines) {
                if (line.empty()) continue;

                std::istringstream iss(line);
                std::string kind_word;
                std::string type_word;
                std::string label;

                if (!(iss >> kind_word >> type_word >> label)) {
                    continue;
                }

                if (type_word == "rule") {
                    if (label == target_label) {
                        if (target.rule_type.empty()) {
                            target.rule_type = kind_word;
                        }
                        continue;
                    }

#ifndef CHECK_EXTERN_DEPS
                    if (!label.empty() && label.find("@") != 0) {
                        target.deps.push_back(label);
                    }
#endif // CHECK_EXTERN_DEPS
                    continue;
                }

                std::string file_path = ConvertBazelLabelToPath(label);
                if (file_path.empty()) {
                    continue;
                }

                auto lower_ext_pos = file_path.find_last_of('.');
                std::string ext = (lower_ext_pos != std::string::npos)
                                      ? file_path.substr(lower_ext_pos)
                                      : "";

                if (ext == ".h" || ext == ".hpp" || ext == ".hh" || ext == ".hxx") {
                    target.hdrs.push_back(file_path);
                } else {
                    target.srcs.push_back(file_path);
                }
            }

            // 未拿到规则类型
            if (target.rule_type.empty()) {
                target.rule_type = "unknown";
            }

        } catch (const std::exception& e) {
            LOG_WARN("Failed to query unified details for " + target_label + ": " + std::string(e.what()));
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Comprehensive query failed for " + target.full_label + ": " + std::string(e.what()));
    }
}

std::unordered_map<std::string, BazelTarget> AdvancedBazelQueryParser::ParseWithIndividualQueries() {
    std::unordered_map<std::string, BazelTarget> targets;
    
    try {
        // 获取所有C++相关的目标，而不是所有目标，提高效率
        std::string targets_output = ExecuteBazelCommand("query 'kind(\"cc_.* rule\", //...)' --output=label" + query_etr_command);
        std::vector<std::string> target_labels = SplitLines(targets_output);
        
        LOG_INFO("Found " + std::to_string(target_labels.size()) + " C++ targets to query individually");
        
        for (const auto& label : target_labels) {
            try {
                // 创建基础目标对象
                BazelTarget target;
                target.full_label = label;
                
                // 解析路径和名称
                size_t last_colon = label.find_last_of(':');
                std::string target_path;
                if (last_colon != std::string::npos) {
                    target_path = label.substr(0, last_colon);
                    target.name = label.substr(last_colon + 1);
                } else {
                    target_path = label;
                    size_t last_slash = label.find_last_of('/');
                    target.name = (last_slash != std::string::npos) ? label.substr(last_slash + 1) : label;
                }
                
                target.path = ConvertBazelLabelToPath(target_path);
                
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

std::unordered_map<std::string, BazelTarget> AdvancedBazelQueryParser::ParseWithConcurrentQueries() {
    std::unordered_map<std::string, BazelTarget> targets;
    
    try {
        // 获取所有C++相关的目标
        std::string targets_output = ExecuteBazelCommand("query 'kind(\"cc_.* rule\", //...)' --output=label" + query_etr_command);
        std::vector<std::string> target_labels = SplitLines(targets_output);
        
        LOG_INFO("Found " + std::to_string(target_labels.size()) + " C++ targets to query concurrently");
        
        // 使用并发处理目标
        QueryTargetDetailsBatch(target_labels, targets);
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to get target list: " + std::string(e.what()));
        // 如果C++目标查询失败，回退到并发查询所有目标
        return ParseAllTargetsConcurrentFallback();
    }
    
    return targets;
}

void AdvancedBazelQueryParser::QueryTargetDetailsBatch(const std::vector<std::string>& target_labels,
                                                      std::unordered_map<std::string, BazelTarget>& targets) {
    const size_t worker_count = std::max<size_t>(1, std::thread::hardware_concurrency());
    const size_t batch_size = std::max<size_t>(1, std::min(target_labels.size(), worker_count * 4));
    std::vector<std::future<std::vector<BazelTarget>>> futures;
    
    // 分批处理目标
    for (size_t i = 0; i < target_labels.size(); i += batch_size) {
        size_t end = std::min(i + batch_size, target_labels.size());
        std::vector<std::string> batch(target_labels.begin() + i, target_labels.begin() + end);
        
        // 异步处理每个批次
        futures.push_back(std::async(std::launch::async, [this, batch]() {
            return ProcessTargetBatch(batch);
        }));
    }
    
    // 收集结果
    std::mutex targets_mutex;
    for (auto& future : futures) {
        try {
            auto batch_results = future.get();
            {
                std::lock_guard<std::mutex> lock(targets_mutex);
                for (auto& target : batch_results) {
                    if (!target.empty()) {
                        targets[target.full_label] = std::move(target);
                    }
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Batch processing failed: " + std::string(e.what()));
        }
    }
}

std::vector<BazelTarget> AdvancedBazelQueryParser::ProcessTargetBatch(const std::vector<std::string>& batch_labels) {
    std::vector<BazelTarget> batch_results;
    batch_results.reserve(batch_labels.size());
    
    for (const auto& label : batch_labels) {
        try {
            auto target = ProcessSingleTarget(label);
            if (!target.empty()) {
                batch_results.push_back(std::move(target));
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to process target " + label + ": " + std::string(e.what()));
        }
    }
    
    return batch_results;
}

BazelTarget AdvancedBazelQueryParser::ProcessSingleTarget(const std::string& label) {
    BazelTarget target;
    target.full_label = label;
    
    // 解析路径和名称
    size_t last_colon = label.find_last_of(':');
    std::string target_path;
    if (last_colon != std::string::npos) {
        target_path = label.substr(0, last_colon);
        target.name = label.substr(last_colon + 1);
    } else {
        target_path = label;
        size_t last_slash = label.find_last_of('/');
        target.name = (last_slash != std::string::npos) ? label.substr(last_slash + 1) : label;
    }
    
    target.path = ConvertBazelLabelToPath(target_path);
    
    QueryTargetDetails(target);
    
    return target;
}

std::unordered_map<std::string, BazelTarget> AdvancedBazelQueryParser::ParseAllTargetsFallback() {
    std::unordered_map<std::string, BazelTarget> targets;
    
    std::string targets_output = ExecuteBazelCommand("query '//...' --output=label" + query_etr_command);
    std::vector<std::string> target_labels = SplitLines(targets_output);
    
    LOG_INFO("Fallback: Found " + std::to_string(target_labels.size()) + " total targets to query");
    
    int processed = 0;
    for (const auto& label : target_labels) {
        try {
            if (label.find("cc_") == std::string::npos) {
                continue;
            }
            
            BazelTarget target;
            target.full_label = label;
            
            size_t last_colon = label.find_last_of(':');
            std::string target_path;
            if (last_colon != std::string::npos) {
                target_path = label.substr(0, last_colon);
                target.name = label.substr(last_colon + 1);
            } else {
                target_path = label;
                target.name = label;
            }
            
            target.path = ConvertBazelLabelToPath(target_path);
            
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

std::unordered_map<std::string, BazelTarget> AdvancedBazelQueryParser::ParseAllTargetsConcurrentFallback() {
    std::unordered_map<std::string, BazelTarget> targets;
    
    std::string targets_output = ExecuteBazelCommand("query '//...' --output=label" + query_etr_command);
    std::vector<std::string> target_labels = SplitLines(targets_output);
    
    LOG_INFO("Concurrent fallback: Found " + std::to_string(target_labels.size()) + " total targets to query");
    
    // 过滤只保留C++相关目标
    std::vector<std::string> cpp_targets;
    for (const auto& label : target_labels) {
        if (label.find("cc_") != std::string::npos) {
            cpp_targets.push_back(label);
        }
    }
    
    LOG_INFO("Filtered to " + std::to_string(cpp_targets.size()) + " C++ targets");
    
    // 使用并发处理
    QueryTargetDetailsBatch(cpp_targets, targets);
    
    return targets;
}

std::string AdvancedBazelQueryParser::ExecuteBazelCommand(const std::string& command) {
    std::string full_command = bazel_binary + " " + command;
    LOG_DEBUG("Executing Bazel command: " + full_command);
    
    return PipeCommandExecutor::execute(full_command);
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
#ifndef CHECK_EXTERN_DEPS
        if (!line.empty()) {
            if (line.find("@") != 0) {
                deps.push_back(line);
            }
        }
#endif // CHECK_EXTERN_DEPS
    }
    
    return deps;
}

std::vector<std::string> AdvancedBazelQueryParser::SplitLines(const std::string& input) {
    std::vector<std::string> lines;
    std::istringstream iss(input);
    std::string line;
    
    while (std::getline(iss, line)) {
        if(line.find("Loading:") != std::string::npos || 
            line.find("INFO:") != std::string::npos ) {
            continue;
        }

        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    
    return lines;
}

std::string AdvancedBazelQueryParser::ConvertBazelLabelToPath(const std::string& bazel_label) {
    if (bazel_label.empty()) {
        return "";
    }
    
    if (bazel_label.find("//") != 0) {
        return bazel_label;
    }
    
    // 移除开头的"//"
    std::string label = bazel_label.substr(2);
    std::string package_path;
    std::string target_name;
    
    // 查找冒号分隔符
    size_t colon_pos = label.find(':');
    
    if (colon_pos != std::string::npos) {
        package_path = label.substr(0, colon_pos);
        target_name = label.substr(colon_pos + 1);
    } else {
        package_path = label;
        size_t last_slash = package_path.find_last_of('/');
        if (last_slash != std::string::npos) {
            target_name = package_path.substr(last_slash + 1);
        } else {
            target_name = package_path;
        }
    }
    
    // 处理根包情况
    if (package_path.empty()) {
        package_path = ".";
    }
    
    // 构建完整路径
    fs::path full_path;
    
    if (target_name.empty()) {
        full_path = fs::path(workspace_path) / package_path;
    } else {
        full_path = fs::path(workspace_path) / package_path / target_name;
    }
    
    return full_path.string();
}
