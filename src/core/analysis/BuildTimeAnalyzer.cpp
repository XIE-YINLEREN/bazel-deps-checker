#include "BuildTimeAnalyzer.h"
#include "pipe.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <queue>
#include <stack>
#include <regex>
#include <sys/resource.h>
#include <unistd.h>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace bazel_analyzer {

// 静态常量定义
const std::string BuildTimeAnalyzer::DEFAULT_PROFILE_OPTIONS = 
    "--profile=profile_detailed.json "
    "--record_full_profiler_data "
    "--noexperimental_inmemory_dotd_files "
    "--noexperimental_inmemory_jdeps_files "
    "--noshow_progress "
    "--noshow_loading_progress "
    "--color=no";

const std::string BuildTimeAnalyzer::DEFAULT_PROFILE_FILE = "profile_detailed.json";

// 私有实现类
class BuildTimeAnalyzer::Impl {
private:
    // 核心数据
    std::vector<std::shared_ptr<BuildEvent>> events_;
    std::vector<CriticalPathNode> critical_path_;
    std::vector<OptimizationSuggestion> suggestions_;
    std::vector<TargetStats> target_stats_;
    BuildPhaseStats stats_;
    std::chrono::microseconds total_build_time_{0};
    std::chrono::microseconds total_analysis_time_{0};
    std::chrono::microseconds profile_generation_time_{0};
    
    // 配置参数
    std::string workspace_path_;
    std::string bazel_binary_;
    std::vector<std::string> build_targets_;
    std::string profile_options_;
    std::string profile_path_;
    std::string export_directory_;
    size_t max_critical_path_length_{10};
    double analysis_threshold_seconds_{1.0};
    
    // 内部数据结构
    std::unordered_map<std::string, std::vector<std::shared_ptr<BuildEvent>>> events_by_thread_;
    std::unordered_map<std::string, std::chrono::microseconds> rule_type_times_;
    std::unordered_map<std::string, std::chrono::microseconds> target_times_;
    std::unordered_map<std::string, std::shared_ptr<BuildEvent>> event_by_id_;
    std::unordered_map<std::string, std::vector<std::string>> dependencies_;
    std::unordered_map<std::string, std::vector<std::string>> dependents_;
    
    // 分析结果
    AnalysisResult analysis_result_;
    
    // 分析状态
    bool is_analyzed_{false};
    bool has_profile_{false};
    mutable std::string last_error_;
    
    // 线程安全
    mutable std::mutex analysis_mutex_;
    
public:
    Impl(const std::string& workspace_path, const std::string& bazel_binary)
        : workspace_path_(workspace_path),
          bazel_binary_(bazel_binary),
          profile_options_(BuildTimeAnalyzer::DEFAULT_PROFILE_OPTIONS),
          profile_path_(BuildTimeAnalyzer::DEFAULT_PROFILE_FILE),
          export_directory_("bazel_analysis_reports") {
        
        build_targets_.push_back("//...");
        
        if (!workspace_path_.empty() && workspace_path_ != ".") {
            try {
                workspace_path_ = fs::absolute(workspace_path_).string();
            } catch (const std::exception&) {
                // 保持原样
            }
        }
    }
    
    ~Impl() {
        ClearAnalysis();
    }
    
    void SetWorkspacePath(const std::string& workspace_path) {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        workspace_path_ = workspace_path;
        if (!workspace_path_.empty() && workspace_path_ != ".") {
            try {
                workspace_path_ = fs::absolute(workspace_path_).string();
            } catch (const std::exception&) {
            }
        }
    }
    
    void SetBazelBinary(const std::string& bazel_binary) {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        bazel_binary_ = bazel_binary;
    }
    
    void SetBuildTargets(const std::vector<std::string>& targets) {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        build_targets_ = targets;
        if (build_targets_.empty()) {
            build_targets_.push_back("//...");
        }
    }
    
    AnalysisResult RunFullAnalysis(const std::vector<std::string>& targets,
                                  bool force_regenerate) {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        
        analysis_result_ = AnalysisResult();
        
        std::vector<std::string> old_targets = build_targets_;
        
        try {
            if (!targets.empty()) {
                build_targets_ = targets;
            }
            
            auto total_start = std::chrono::high_resolution_clock::now();
            
            if (!EnsureProfileExists(force_regenerate)) {
                analysis_result_.success = false;
                analysis_result_.error_message = "Failed to ensure profile exists: " + last_error_;
                build_targets_ = old_targets;
                return analysis_result_;
            }
            
            auto after_generation = std::chrono::high_resolution_clock::now();
            analysis_result_.generation_time = std::chrono::duration_cast<std::chrono::microseconds>(
                after_generation - total_start);
            
            if (!LoadAndAnalyze(false)) {
                analysis_result_.success = false;
                analysis_result_.error_message = "Failed to load and analyze profile: " + last_error_;
                build_targets_ = old_targets;
                return analysis_result_;
            }
            
            auto after_analysis = std::chrono::high_resolution_clock::now();
            analysis_result_.analysis_time = std::chrono::duration_cast<std::chrono::microseconds>(
                after_analysis - after_generation);
            
            analysis_result_.stats = stats_;
            analysis_result_.suggestions = suggestions_;
            analysis_result_.critical_paths = critical_path_;
            
            ExportReports();
            
            analysis_result_.success = true;
            
            build_targets_ = old_targets;
            
            return analysis_result_;
            
        } catch (const std::exception& e) {
            analysis_result_.success = false;
            analysis_result_.error_message = "Exception during full analysis: " + std::string(e.what());
            build_targets_ = old_targets;
            return analysis_result_;
        }
    }
    
    std::vector<CriticalPathNode> GetCriticalPath() const {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        return critical_path_;
    }
    
    std::vector<OptimizationSuggestion> GetOptimizationSuggestions() const {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        return suggestions_;
    }
    
    BuildPhaseStats GetBuildStats() const {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        return stats_;
    }
    
    void ClearAnalysis() {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        
        events_.clear();
        critical_path_.clear();
        suggestions_.clear();
        target_stats_.clear();
        events_by_thread_.clear();
        rule_type_times_.clear();
        target_times_.clear();
        event_by_id_.clear();
        dependencies_.clear();
        dependents_.clear();
        
        stats_ = BuildPhaseStats{};
        total_build_time_ = std::chrono::microseconds{0};
        total_analysis_time_ = std::chrono::microseconds{0};
        profile_generation_time_ = std::chrono::microseconds{0};
        
        analysis_result_ = AnalysisResult{};
        is_analyzed_ = false;
        last_error_.clear();
    }
    
private:
    bool ValidateWorkspace() const {
        if (workspace_path_.empty()) {
            last_error_ = "Workspace path is empty";
            return false;
        }
        
        try {
            if (!fs::exists(workspace_path_)) {
                last_error_ = "Workspace path does not exist: " + workspace_path_;
                return false;
            }
            
            if (!fs::is_directory(workspace_path_)) {
                last_error_ = "Workspace path is not a directory: " + workspace_path_;
                return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            last_error_ = "Error validating workspace: " + std::string(e.what());
            return false;
        }
    }
    
    bool ValidateBazelBinary() const {
        if (bazel_binary_.empty()) {
            last_error_ = "Bazel binary path is empty";
            return false;
        }
        
        try {
            std::string test_command;
            if (workspace_path_ != "." && !workspace_path_.empty()) {
                test_command = "cd \"" + workspace_path_ + "\" && " + bazel_binary_ + " version";
            } else {
                test_command = bazel_binary_ + " version";
            }
            
            auto [output, exit_code] = PipeCommandExecutor::executeWithStatus(
                PipeCommandExecutor::setCommand(test_command));
            
            if (exit_code != 0) {
                last_error_ = "Bazel binary is not executable or not found: " + bazel_binary_;
                return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            last_error_ = "Error validating bazel binary: " + std::string(e.what());
            return false;
        }
    }
    
    bool ValidateConfiguration() const {
        if (!ValidateWorkspace()) {
            return false;
        }
        
        if (!ValidateBazelBinary()) {
            return false;
        }
        
        if (build_targets_.empty()) {
            last_error_ = "No build targets specified";
            return false;
        }
        
        return true;
    }
    
    std::pair<std::string, int> ExecuteBazelCommand(const std::string& command) const {
        std::string full_command;
        
        if (workspace_path_ != "." && !workspace_path_.empty()) {
            full_command = "cd \"" + workspace_path_ + "\" && " + command;
        } else {
            full_command = command;
        }
        
        return PipeCommandExecutor::executeWithStatus(
            PipeCommandExecutor::setCommand(full_command));
    }
    
    std::string BuildFullBazelCommand(const std::vector<std::string>& targets,
                                     const std::string& additional_args = "") const {
        std::stringstream cmd;
        
        cmd << bazel_binary_ << " build";
        
        const auto& actual_targets = targets.empty() ? build_targets_ : targets;
        for (const auto& target : actual_targets) {
            cmd << " " << target;
        }
        
        std::string profile_options = profile_options_;
        if (workspace_path_ != "." && !workspace_path_.empty()) {
            if (!fs::path(profile_path_).is_absolute()) {
                std::string absolute_profile_path = fs::absolute(fs::path(workspace_path_) / profile_path_).string();
                size_t pos = profile_options.find("--profile=");
                if (pos != std::string::npos) {
                    size_t end = profile_options.find(' ', pos);
                    if (end == std::string::npos) end = profile_options.length();
                    profile_options = profile_options.substr(0, pos) + 
                                     "--profile=" + absolute_profile_path + 
                                     profile_options.substr(end);
                }
            }
        }
        
        cmd << " " << profile_options;
        
        if (!additional_args.empty()) {
            cmd << " " << additional_args;
        }
        
        return cmd.str();
    }
    
    bool EnsureProfileExists(bool force_regenerate = false) {
        if (!ValidateConfiguration()) {
            return false;
        }
        
        if (force_regenerate) {
            CleanProfileFile();
        }
        
        if (ProfileFileExists() && ValidateProfileFile()) {
            has_profile_ = true;
            return true;
        }
        
        has_profile_ = GenerateProfile();
        return has_profile_;
    }
    
    bool ProfileFileExists() const {
        try {
            std::string actual_profile_path = profile_path_;
            
            if (!fs::path(actual_profile_path).is_absolute() && 
                workspace_path_ != "." && !workspace_path_.empty()) {
                actual_profile_path = fs::path(workspace_path_) / profile_path_;
            }
            
            return fs::exists(actual_profile_path) && fs::file_size(actual_profile_path) > 0;
        } catch (const std::exception&) {
            return false;
        }
    }
    
    bool ValidateProfileFile() const {
        std::string actual_profile_path = profile_path_;
        
        if (!fs::path(actual_profile_path).is_absolute() && 
            workspace_path_ != "." && !workspace_path_.empty()) {
            actual_profile_path = fs::path(workspace_path_) / profile_path_;
        }
        
        if (!ProfileFileExists()) {
            last_error_ = "Profile file does not exist: " + actual_profile_path;
            return false;
        }
        
        try {
            std::ifstream file(actual_profile_path);
            if (!file.is_open()) {
                last_error_ = "Failed to open profile file: " + actual_profile_path;
                return false;
            }
            
            json profile_data = json::parse(file);
            bool has_trace_events = profile_data.contains("traceEvents");
            bool has_other_data = profile_data.contains("otherData") || 
                                 profile_data.contains("displayTimeUnit") ||
                                 !profile_data.empty();
            
            file.close();
            
            if (!has_trace_events && !has_other_data) {
                last_error_ = "Profile file does not contain required fields";
                return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            last_error_ = "JSON parsing error: " + std::string(e.what());
            return false;
        }
    }
    
    bool CleanProfileFile() const {
        try {
            std::string actual_profile_path = profile_path_;
            if (!fs::path(actual_profile_path).is_absolute() && 
                workspace_path_ != "." && !workspace_path_.empty()) {
                actual_profile_path = fs::path(workspace_path_) / profile_path_;
            }
            
            if (fs::exists(actual_profile_path)) {
                fs::remove(actual_profile_path);
                return true;
            }
            return false;
        } catch (const std::exception& e) {
            last_error_ = "Failed to clean profile file: " + std::string(e.what());
            return false;
        }
    }
    
    bool GenerateProfile(const std::vector<std::string>& targets = {},
                        const std::string& additional_args = "") {
        if (!ValidateConfiguration()) {
            return false;
        }
        
        try {
            std::string full_command = BuildFullBazelCommand(targets, additional_args);
            
            auto start_time = std::chrono::high_resolution_clock::now();
            
            auto [output, exit_code] = ExecuteBazelCommand(full_command);
            
            auto end_time = std::chrono::high_resolution_clock::now();
            profile_generation_time_ = std::chrono::duration_cast<std::chrono::microseconds>(
                end_time - start_time);
            
            ParseGenerationOutput(output, exit_code);
            
            if (exit_code != 0) {
                last_error_ = "Bazel command failed with exit code: " + std::to_string(exit_code);
                return false;
            }
            
            if (!ValidateProfileFile()) {
                last_error_ = "Generated profile file is invalid";
                return false;
            }
            
            has_profile_ = true;
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Exception during profile generation: " + std::string(e.what());
            return false;
        }
    }
    
    void ParseGenerationOutput(const std::string& output, [[maybe_unused]] int exit_code) {
        // 简单解析输出，获取基本信息
        std::regex action_count_regex(R"((\d+)\s+action(s?)\s+completed)");
        std::smatch matches;
        
        if (std::regex_search(output, matches, action_count_regex)) {
            if (matches.size() >= 2) {
                // 记录操作数量
            }
        }
    }
    
    bool LoadAndAnalyze(bool auto_generate_if_missing = true) {
        ClearAnalysis();
        
        if (!LoadProfile(auto_generate_if_missing)) {
            return false;
        }
        
        is_analyzed_ = true;
        return true;
    }
    
    bool LoadProfile(bool auto_generate_if_missing = true) {
        std::string actual_profile_path = profile_path_;
        if (!fs::path(actual_profile_path).is_absolute() && 
            workspace_path_ != "." && !workspace_path_.empty()) {
            actual_profile_path = fs::path(workspace_path_) / profile_path_;
        }
        
        if (!ProfileFileExists()) {
            if (auto_generate_if_missing) {
                if (!GenerateProfile()) {
                    return false;
                }
            } else {
                last_error_ = "Profile file not found: " + actual_profile_path;
                return false;
            }
        }
        
        if (!ValidateProfileFile()) {
            if (auto_generate_if_missing) {
                if (!GenerateProfile()) {
                    return false;
                }
            } else {
                return false;
            }
        }
        
        std::ifstream file(actual_profile_path);
        if (!file.is_open()) {
            last_error_ = "Failed to open profile file: " + actual_profile_path;
            return false;
        }
        
        auto analysis_start = std::chrono::high_resolution_clock::now();
        
        try {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string json_content = buffer.str();
            file.close();
            
            if (!ParseProfileData(json_content)) {
                return false;
            }
            
            BuildDependencyGraph();
            
            CalculateCriticalPath();
            AnalyzeConcurrency();
            AnalyzeCacheEfficiency();
            AnalyzeMemoryUsage();
            AnalyzeVFSOperations();
            CalculateTargetStatistics();
            GenerateOptimizationSuggestions();
            
            auto analysis_end = std::chrono::high_resolution_clock::now();
            total_analysis_time_ = std::chrono::duration_cast<std::chrono::microseconds>(
                analysis_end - analysis_start);
            
            return true;
        } catch (const std::exception& e) {
            last_error_ = "Exception during profile loading: " + std::string(e.what());
            return false;
        }
    }
    
    bool ParseProfileData(const std::string& json_content) {
        try {
            json profile_data = json::parse(json_content);
            
            if (!profile_data.contains("traceEvents")) {
                last_error_ = "Profile file does not contain traceEvents";
                return false;
            }
            
            const auto& trace_events = profile_data["traceEvents"];
            
            for (const auto& event : trace_events) {
                if (!event.contains("name") || !event.contains("ts") || 
                    !event.contains("dur")) {
                    continue;
                }
                
                std::string name = event["name"];
                std::string cat = event.value("cat", "OTHER");
                std::string tid = event.value("tid", "0");
                std::string pid = event.value("pid", "0");
                
                std::string event_id = pid + ":" + tid + ":" + name;
                
                std::chrono::microseconds start(event["ts"].get<uint64_t>());
                std::chrono::microseconds duration(event["dur"].get<uint64_t>());
                
                if (duration.count() / 1000000.0 < analysis_threshold_seconds_) {
                    continue;
                }
                
                EventType type = ClassifyEvent(name, cat);
                
                auto build_event = std::make_shared<BuildEvent>(name, type, start, duration);
                build_event->category = cat;
                build_event->thread_id = tid;
                
                if (event.contains("args")) {
                    const auto& args = event["args"];
                    if (args.contains("outputs")) {
                        build_event->output = args["outputs"].dump();
                    }
                    
                    if (args.contains("inputs") && args["inputs"].is_array()) {
                        for (const auto& input : args["inputs"]) {
                            if (input.is_string()) {
                                build_event->dependencies.push_back(input.get<std::string>());
                            }
                        }
                    }
                }
                
                build_event->rule = ExtractRuleName(name);
                
                events_.push_back(build_event);
                events_by_thread_[tid].push_back(build_event);
                event_by_id_[event_id] = build_event;
                
                if (!build_event->rule.empty()) {
                    rule_type_times_[build_event->rule] += duration;
                }
                
                std::string target = ExtractTargetName(name);
                if (!target.empty()) {
                    target_times_[target] += duration;
                }
                
                stats_.total_duration += duration;
                switch (type) {
                    case EventType::PACKAGE_LOAD:
                        stats_.loading_duration += duration;
                        if (duration > stats_.longest_loading_time) {
                            stats_.longest_loading_time = duration;
                            stats_.longest_loading_event = name;
                        }
                        break;
                    case EventType::ANALYSIS:
                        stats_.analysis_duration += duration;
                        if (duration > stats_.longest_analysis_time) {
                            stats_.longest_analysis_time = duration;
                            stats_.longest_analysis_event = name;
                        }
                        break;
                    case EventType::EXECUTION:
                    case EventType::ACTION:
                        stats_.execution_duration += duration;
                        if (duration > stats_.longest_execution_time) {
                            stats_.longest_execution_time = duration;
                            stats_.longest_execution_event = name;
                        }
                        break;
                    case EventType::VFS:
                        stats_.vfs_duration += duration;
                        stats_.total_vfs_operations++;
                        if (name.find("read") != std::string::npos) {
                            stats_.vfs_read_operations++;
                        } else if (name.find("write") != std::string::npos) {
                            stats_.vfs_write_operations++;
                        }
                        break;
                    default:
                        stats_.other_duration += duration;
                        break;
                }
            }
            
            if (!events_.empty()) {
                auto min_iter = std::min_element(events_.begin(), events_.end(),
                    [](const auto& a, const auto& b) {
                        return a->start_time < b->start_time;
                    });
                auto min_time = (*min_iter)->start_time;

                auto max_iter = std::max_element(events_.begin(), events_.end(),
                    [](const auto& a, const auto& b) {
                        return a->start_time + a->duration < b->start_time + b->duration;
                    });
                
                total_build_time_ = ((*max_iter)->start_time + (*max_iter)->duration) - min_time;
                
                for (auto& event : events_) {
                    event->percentage_of_total = (event->duration.count() / 1000000.0) / 
                                                (total_build_time_.count() / 1000000.0) * 100.0;
                }
            }
            
            return true;
        } catch (const std::exception& e) {
            last_error_ = "JSON parsing error: " + std::string(e.what());
            return false;
        }
    }
    
    EventType ClassifyEvent(const std::string& name, const std::string& category) {
        std::string lower_name;
        std::transform(name.begin(), name.end(), std::back_inserter(lower_name),
                       [](unsigned char c) { return std::tolower(c); });
        
        std::string lower_cat;
        std::transform(category.begin(), category.end(), std::back_inserter(lower_cat),
                       [](unsigned char c) { return std::tolower(c); });
        
        if (lower_name.find("action") != std::string::npos ||
            lower_name.find("exec") != std::string::npos ||
            lower_cat.find("action") != std::string::npos) {
            return EventType::ACTION;
        }
        else if (lower_name.find("package") != std::string::npos ||
                 lower_name.find("load") != std::string::npos ||
                 lower_name.find("starlark") != std::string::npos) {
            return EventType::PACKAGE_LOAD;
        }
        else if (lower_name.find("analyze") != std::string::npos ||
                 lower_name.find("analysis") != std::string::npos ||
                 lower_name.find("configure") != std::string::npos) {
            return EventType::ANALYSIS;
        }
        else if (lower_name.find("vfs") != std::string::npos ||
                 lower_name.find("file") != std::string::npos ||
                 lower_name.find("stat") != std::string::npos ||
                 lower_name.find("read") != std::string::npos ||
                 lower_name.find("write") != std::string::npos) {
            return EventType::VFS;
        }
        else if (lower_cat.find("execution") != std::string::npos ||
                 lower_name.find("spawn") != std::string::npos ||
                 lower_name.find("worker") != std::string::npos) {
            return EventType::EXECUTION;
        }
        
        return EventType::OTHER;
    }
    
    std::string ExtractRuleName(const std::string& event_name) const {
        std::vector<std::string> rule_patterns = {
            "cc_", "java_", "py_", "go_", "rust_", "proto_", "genrule",
            "sh_", "test_", "binary_", "library_", "compiler_", "link_"
        };
        
        for (const auto& pattern : rule_patterns) {
            size_t pos = event_name.find(pattern);
            if (pos != std::string::npos) {
                size_t end = event_name.find(' ', pos);
                if (end != std::string::npos) {
                    return event_name.substr(pos, end - pos);
                }
                return event_name.substr(pos);
            }
        }
        
        std::regex rule_regex(R"((\w+_[\w_]+)\s+)");
        std::smatch matches;
        
        if (std::regex_search(event_name, matches, rule_regex)) {
            if (matches.size() >= 2) {
                return matches[1];
            }
        }
        
        return "";
    }
    
    std::string ExtractTargetName(const std::string& event_name) const {
        std::regex target_regex(R"((//[^:\s]+:[^\s]+))");
        std::smatch matches;
        
        if (std::regex_search(event_name, matches, target_regex)) {
            if (matches.size() >= 2) {
                return matches[1];
            }
        }
        
        size_t colon_pos = event_name.find_last_of(':');
        if (colon_pos != std::string::npos && colon_pos > 0) {
            size_t start = event_name.rfind(' ', colon_pos);
            if (start != std::string::npos) {
                return event_name.substr(start + 1, colon_pos - start);
            }
            return event_name.substr(0, colon_pos);
        }
        
        return "";
    }
    
    void BuildDependencyGraph() {
        for (const auto& event : events_) {
            std::string event_id = "0:" + event->thread_id + ":" + event->name;
            
            for (const auto& dep : event->dependencies) {
                dependencies_[event_id].push_back(dep);
                dependents_[dep].push_back(event_id);
            }
        }
    }
    
    void CalculateCriticalPath() {
        critical_path_.clear();
        
        std::vector<std::string> root_nodes;
        for (const auto& event_pair : event_by_id_) {
            const auto& event_id = event_pair.first;
            if (dependencies_.find(event_id) == dependencies_.end() || 
                dependencies_[event_id].empty()) {
                root_nodes.push_back(event_id);
            }
        }
        
        for (const auto& root_node : root_nodes) {
            std::vector<std::string> current_path;
            std::chrono::microseconds current_duration{0};
            
            std::string current_node = root_node;
            while (event_by_id_.find(current_node) != event_by_id_.end()) {
                const auto& event = event_by_id_[current_node];
                current_path.push_back(event->name);
                current_duration += event->duration;
                
                if (dependents_.find(current_node) == dependents_.end() || 
                    dependents_[current_node].empty()) {
                    break;
                }
                
                std::string next_node;
                std::chrono::microseconds max_duration{0};
                for (const auto& dependent_id : dependents_[current_node]) {
                    if (event_by_id_.find(dependent_id) != event_by_id_.end()) {
                        const auto& dep_event = event_by_id_[dependent_id];
                        if (dep_event->duration > max_duration) {
                            max_duration = dep_event->duration;
                            next_node = dependent_id;
                        }
                    }
                }
                
                if (next_node.empty()) break;
                current_node = next_node;
            }
            
            if (!current_path.empty()) {
                CriticalPathNode path_node;
                path_node.event_name = current_path.back();
                path_node.cumulative_duration = current_duration;
                path_node.path = current_path;
                path_node.rule_type = event_by_id_[root_node]->rule;
                critical_path_.push_back(path_node);
            }
        }
        
        std::sort(critical_path_.begin(), critical_path_.end());
        
        if (critical_path_.size() > max_critical_path_length_) {
            critical_path_.resize(max_critical_path_length_);
        }
    }
    
    void AnalyzeConcurrency() {
        struct TimelineEvent {
            std::chrono::microseconds time;
            bool is_start;
            const BuildEvent* event;
        };
        
        std::vector<TimelineEvent> timeline;
        for (const auto& event : events_) {
            if (event->type == EventType::ACTION || event->type == EventType::EXECUTION) {
                timeline.push_back({event->start_time, true, event.get()});
                timeline.push_back({event->start_time + event->duration, false, event.get()});
            }
        }
        
        std::sort(timeline.begin(), timeline.end(),
            [](const auto& a, const auto& b) {
                return a.time < b.time;
            });
        
        size_t current_concurrency = 0;
        std::chrono::microseconds last_time{0};
        std::chrono::microseconds total_weighted_concurrency{0};
        
        stats_.max_concurrent_actions = 0;
        
        for (const auto& tl_event : timeline) {
            if (last_time.count() > 0) {
                auto duration = tl_event.time - last_time;
                total_weighted_concurrency += duration * current_concurrency;
            }
            
            if (tl_event.is_start) {
                current_concurrency++;
                stats_.max_concurrent_actions = std::max(stats_.max_concurrent_actions, current_concurrency);
            } else {
                current_concurrency--;
            }
            
            last_time = tl_event.time;
        }
        
        if (total_build_time_.count() > 0) {
            stats_.average_concurrency = total_weighted_concurrency / total_build_time_;
        }
    }
    
    void AnalyzeCacheEfficiency() {
        size_t hits = 0;
        size_t misses = 0;
        
        for (const auto& event : events_) {
            if (IsCacheEvent(event->name)) {
                if (event->name.find("cache hit") != std::string::npos ||
                    event->name.find("remote cache hit") != std::string::npos ||
                    event->name.find("disk cache hit") != std::string::npos) {
                    hits++;
                } else if (event->name.find("cache miss") != std::string::npos ||
                           event->name.find("no cache") != std::string::npos ||
                           event->name.find("local execution") != std::string::npos) {
                    misses++;
                }
            }
        }
        
        stats_.cache_hits = hits;
        stats_.cache_misses = misses;
        
        if (hits + misses > 0) {
            stats_.cache_hit_rate = static_cast<double>(hits) / (hits + misses) * 100.0;
        }
    }
    
    bool IsCacheEvent(const std::string& event_name) const {
        std::string lower_name;
        std::transform(event_name.begin(), event_name.end(), std::back_inserter(lower_name),
                       [](unsigned char c) { return std::tolower(c); });
        
        return lower_name.find("cache") != std::string::npos ||
               lower_name.find("remote") != std::string::npos ||
               lower_name.find("disk") != std::string::npos ||
               lower_name.find("hit") != std::string::npos ||
               lower_name.find("miss") != std::string::npos;
    }
    
    void AnalyzeMemoryUsage() {
        size_t peak_memory = 0;
        size_t total_memory = 0;
        size_t memory_samples = 0;
        
        for (const auto& event : events_) {
            if (IsMemoryEvent(event->name)) {
                std::regex memory_regex(R"((\d+)\s*(KB|MB|GB))");
                std::smatch matches;
                std::string name_lower = event->name;
                std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
                
                if (std::regex_search(name_lower, matches, memory_regex)) {
                    if (matches.size() >= 3) {
                        size_t memory_value = std::stoull(matches[1]);
                        std::string unit = matches[2];
                        
                        if (unit == "MB") {
                            memory_value *= 1024;
                        } else if (unit == "GB") {
                            memory_value *= 1024 * 1024;
                        }
                        
                        peak_memory = std::max(peak_memory, memory_value);
                        total_memory += memory_value;
                        memory_samples++;
                    }
                }
            }
        }
        
        stats_.peak_memory_usage = peak_memory;
        if (memory_samples > 0) {
            stats_.average_memory_usage = total_memory / memory_samples;
        }
    }
    
    bool IsMemoryEvent(const std::string& event_name) const {
        std::string lower_name;
        std::transform(event_name.begin(), event_name.end(), std::back_inserter(lower_name),
                       [](unsigned char c) { return std::tolower(c); });
        
        return lower_name.find("memory") != std::string::npos ||
               lower_name.find("heap") != std::string::npos ||
               lower_name.find("ram") != std::string::npos ||
               lower_name.find("kb") != std::string::npos ||
               lower_name.find("mb") != std::string::npos ||
               lower_name.find("gb") != std::string::npos;
    }
    
    void AnalyzeVFSOperations() {
        // 已经在ParseProfileData中统计
    }
    
    void CalculateTargetStatistics() {
        // 简化实现
    }
    
    void GenerateOptimizationSuggestions() {
        suggestions_.clear();
        
        // 分析长时间运行的action
        std::vector<std::shared_ptr<BuildEvent>> long_actions;
        for (const auto& event : events_) {
            if ((event->type == EventType::ACTION || event->type == EventType::EXECUTION) &&
                event->duration.count() / 1000000.0 > 30.0) {
                long_actions.push_back(event);
            }
        }
        
        if (!long_actions.empty()) {
            OptimizationSuggestion suggestion;
            suggestion.issue = "发现长时间运行的构建action";
            suggestion.severity = OptimizationSuggestion::Severity::HIGH;
            suggestion.estimated_improvement = 15.0;
            
            std::stringstream ss;
            ss << "以下action执行时间超过30秒：\n";
            for (size_t i = 0; i < std::min(long_actions.size(), size_t(5)); ++i) {
                const auto& action = long_actions[i];
                ss << "  - " << action->name << ": " << action->duration.count() / 1000000.0 << "s\n";
                suggestion.affected_targets.push_back(action->name);
            }
            ss << "\n建议：\n";
            ss << "1. 考虑拆分大型目标为多个小目标\n";
            ss << "2. 检查是否有不必要的依赖\n";
            ss << "3. 考虑使用远程执行或缓存\n";
            
            suggestion.suggestion = ss.str();
            suggestions_.push_back(suggestion);
        }
        
        // 分析并行度
        if (stats_.max_concurrent_actions > 1 && 
            stats_.average_concurrency < stats_.max_concurrent_actions * 0.3) {
            OptimizationSuggestion suggestion;
            suggestion.issue = "构建并行度不足";
            suggestion.severity = OptimizationSuggestion::Severity::MEDIUM;
            suggestion.estimated_improvement = 10.0;
            
            std::stringstream ss;
            ss << "平均并发度: " << stats_.average_concurrency << "\n";
            ss << "最大并发度: " << stats_.max_concurrent_actions << "\n";
            ss << "并行度利用率: " 
               << (stats_.average_concurrency / static_cast<double>(stats_.max_concurrent_actions) * 100) 
               << "%\n\n";
            ss << "建议：\n";
            ss << "1. 增加--jobs参数\n";
            ss << "2. 优化目标间的依赖关系减少串行化\n";
            
            suggestion.suggestion = ss.str();
            suggestions_.push_back(suggestion);
        }
        
        // 分析缓存效率
        if (stats_.cache_hit_rate < 50.0 && stats_.cache_hits + stats_.cache_misses > 10) {
            OptimizationSuggestion suggestion;
            suggestion.issue = "缓存命中率较低";
            suggestion.severity = OptimizationSuggestion::Severity::MEDIUM;
            suggestion.estimated_improvement = 20.0;
            
            std::stringstream ss;
            ss << "缓存命中率: " << stats_.cache_hit_rate << "%\n";
            ss << "缓存命中: " << stats_.cache_hits << "\n";
            ss << "缓存未命中: " << stats_.cache_misses << "\n\n";
            ss << "建议：\n";
            ss << "1. 确保使用--remote_cache参数\n";
            ss << "2. 检查缓存服务器状态和网络连接\n";
            
            suggestion.suggestion = ss.str();
            suggestions_.push_back(suggestion);
        }
    }
    
    void ExportReports() {
        if (!is_analyzed_) {
            return;
        }
        
        try {
            export_directory_ = EnsureDirectoryExists(export_directory_);
            
            std::string timestamp = GenerateTimestamp();
            
            // 生成文本报告
            std::string report_filename = export_directory_ + "/build_analysis_" + timestamp + ".txt";
            std::ofstream report_file(report_filename);
            if (report_file.is_open()) {
                std::stringstream report;
                report << "===========================================================\n";
                report << "              BAZEL BUILD TIME ANALYSIS REPORT             \n";
                report << "===========================================================\n\n";
                report << "Workspace: " << workspace_path_ << "\n";
                report << "Bazel Binary: " << bazel_binary_ << "\n";
                report << "Profile File: " << profile_path_ << "\n";
                report << "Generated: " << timestamp << "\n\n";
                
                report << "Total Build Time: " << total_build_time_.count() / 1000000.0 << "s\n";
                report << "Total Events: " << events_.size() << "\n\n";
                
                report_file << report.str();
                report_file.close();
                analysis_result_.report_path = report_filename;
            }
            
            // 导出CSV
            std::string csv_filename = export_directory_ + "/build_events_" + timestamp + ".csv";
            if (ExportToCSV(csv_filename)) {
                analysis_result_.csv_path = csv_filename;
            }
            
        } catch (const std::exception&) {
            // 忽略导出错误
        }
    }
    
    std::string GenerateTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");
        return ss.str();
    }
    
    std::string EnsureDirectoryExists(const std::string& dir_path) const {
        try {
            fs::path path(dir_path);
            if (!fs::exists(path)) {
                fs::create_directories(path);
            }
            return fs::absolute(path).string();
        } catch (const std::exception&) {
            return dir_path;
        }
    }
    
    bool ExportToCSV(const std::string& csv_path) const {
        if (!is_analyzed_) {
            return false;
        }
        
        try {
            std::ofstream csv_file(csv_path);
            if (!csv_file.is_open()) {
                return false;
            }
            
            csv_file << "EventName,EventType,StartTime(us),Duration(us),"
                     << "ThreadID,Category,PercentageOfTotal,Rule\n";
            
            for (const auto& event : events_) {
                std::string type_str;
                switch (event->type) {
                    case EventType::ACTION: type_str = "ACTION"; break;
                    case EventType::PACKAGE_LOAD: type_str = "PACKAGE_LOAD"; break;
                    case EventType::ANALYSIS: type_str = "ANALYSIS"; break;
                    case EventType::EXECUTION: type_str = "EXECUTION"; break;
                    case EventType::VFS: type_str = "VFS"; break;
                    default: type_str = "OTHER"; break;
                }
                
                std::string escaped_name = event->name;
                std::replace(escaped_name.begin(), escaped_name.end(), ',', ';');
                std::replace(escaped_name.begin(), escaped_name.end(), '\"', '\'');
                
                csv_file << "\"" << escaped_name << "\","
                         << type_str << ","
                         << event->start_time.count() << ","
                         << event->duration.count() << ","
                         << event->thread_id << ","
                         << event->category << ","
                         << std::fixed << std::setprecision(2) << event->percentage_of_total << ","
                         << "\"" << event->rule << "\"\n";
            }
            
            csv_file.close();
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
};

// BuildTimeAnalyzer 公共接口实现
BuildTimeAnalyzer::BuildTimeAnalyzer(const std::string& workspace_path,
                                     const std::string& bazel_binary)
    : impl_(std::make_unique<Impl>(workspace_path, bazel_binary)) {}

BuildTimeAnalyzer::~BuildTimeAnalyzer() = default;

void BuildTimeAnalyzer::SetWorkspacePath(const std::string& workspace_path) {
    impl_->SetWorkspacePath(workspace_path);
}

void BuildTimeAnalyzer::SetBazelBinary(const std::string& bazel_binary) {
    impl_->SetBazelBinary(bazel_binary);
}

void BuildTimeAnalyzer::SetBuildTargets(const std::vector<std::string>& targets) {
    impl_->SetBuildTargets(targets);
}

AnalysisResult BuildTimeAnalyzer::RunFullAnalysis(const std::vector<std::string>& targets,
                                                 bool force_regenerate) {
    return impl_->RunFullAnalysis(targets, force_regenerate);
}

std::vector<CriticalPathNode> BuildTimeAnalyzer::GetCriticalPath() const {
    return impl_->GetCriticalPath();
}

std::vector<OptimizationSuggestion> BuildTimeAnalyzer::GetOptimizationSuggestions() const {
    return impl_->GetOptimizationSuggestions();
}

BuildPhaseStats BuildTimeAnalyzer::GetBuildStats() const {
    return impl_->GetBuildStats();
}

void BuildTimeAnalyzer::ClearAnalysis() {
    impl_->ClearAnalysis();
}

} // namespace bazel_analyzer
