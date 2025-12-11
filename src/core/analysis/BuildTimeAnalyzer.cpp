#include "analysis/BuildTimeAnalyzer.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <regex>
#include <string>
#include <mutex>

namespace fs = std::filesystem;

// 静态成员初始化
const std::string BuildTimeAnalyzer::DEFAULT_PROFILE_OPTIONS = 
    "--profile=profile_detailed.json "
    "--record_full_profiler_data "
    "--noexperimental_inmemory_dotd_files "
    "--noexperimental_inmemory_jdeps_files "
    "--noshow_progress "
    "--noshow_loading_progress "
    "--color=no";

// 防止并发加载
static std::mutex profile_loading_mutex;

BuildTimeAnalyzer::BuildTimeAnalyzer(const std::string& bazel_binary, const std::string& workspace_path)
    : bazel_binary_(bazel_binary),
      workspace_path_(fs::absolute(workspace_path).string()),
      profile_file_path_(fs::absolute(workspace_path + "/profile_detailed.json").string()),
      profile_options_(DEFAULT_PROFILE_OPTIONS) {
    
    // 创建工作空间目录（如果不存在）
    if (!fs::exists(workspace_path_)) {
        LOG_ERROR("Workspace path does not exist:" + workspace_path_);
        throw std::runtime_error("Workspace path does not exist");
    }
    
    LOG_INFO("BuildTimeAnalyzer initialized. Workspace:" + workspace_path_);
}

BuildTimeAnalyzer::~BuildTimeAnalyzer() {
    // cleanupProfile();
}

bool BuildTimeAnalyzer::validateEnvironment() const {
    // 检查是否有足够的磁盘空间（至少100MB）
    auto space_info = fs::space(workspace_path_);
    if (space_info.available < 100 * 1024 * 1024) {
        LOG_WARN("Low disk space available: " + formatMemory(space_info.available));
    }
    
    return true;
}

std::string BuildTimeAnalyzer::constructBuildCommand(const std::string& target) const {
    std::string command = bazel_binary_;

    command += " build " + target + " " + profile_options_;
    
    return command;
}

bool BuildTimeAnalyzer::createProfile(const std::string& target) const {
    if (!validateEnvironment()) {
        return false;
    }
    
    LOG_INFO("Creating profile for target:" + target);
    LOG_INFO("Working directory:" + workspace_path_);
    
    // 清理旧的profile文件
    if (fs::exists(profile_file_path_)) {
        try {
            fs::remove(profile_file_path_);
            LOG_INFO("Removed old profile file:" + profile_file_path_);
        } catch (const fs::filesystem_error& e) {
            std::string err = e.what();
            LOG_WARN("Failed to remove old profile file: " + err);
        }
    }
    
    std::string command = constructBuildCommand(target);
    
    LOG_DEBUG("Executing command in workspace: " + workspace_path_);
    LOG_DEBUG("Command: " + command);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        // 在执行命令前保存当前工作目录
        fs::path original_cwd = fs::current_path();
        
        try {
            // 切换到工作空间目录
            fs::current_path(workspace_path_);
            LOG_DEBUG("Changed working directory to: " + workspace_path_);
            
            // 执行构建命令
            auto [output, exit_code] = PipeCommandExecutor::executeWithStatus(command);
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
            
            LOG_INFO("Build completed in " + std::to_string(duration) + " seconds. Exit code: " + std::to_string(exit_code));
            
            if (exit_code != 0) {
                LOG_ERROR("Build failed with exit code: " + std::to_string(exit_code));
                LOG_ERROR("Build output:\n" + output);
                // 恢复原始工作目录
                fs::current_path(original_cwd);
                return false;
            }
            
            // 检查profile文件是否生成
            if (!fs::exists(profile_file_path_)) {
                LOG_ERROR("Profile file was not generated: " + profile_file_path_);
                // 恢复原始工作目录
                fs::current_path(original_cwd);
                return false;
            }
            
            // 检查文件大小
            auto file_size = fs::file_size(profile_file_path_);
            if (file_size == 0) {
                LOG_ERROR("Profile file is empty: " + profile_file_path_);
                // 恢复原始工作目录
                fs::current_path(original_cwd);
                return false;
            }
            
            LOG_INFO("Profile file generated successfully: " + std::to_string(file_size) + " bytes");
            
            // 恢复原始工作目录
            fs::current_path(original_cwd);
            LOG_DEBUG("Restored working directory to: " + original_cwd.string());
            
            return true;
            
        } catch (...) {
            // 发生异常时恢复原始工作目录
            fs::current_path(original_cwd);
            throw;
        }
        
    } catch (const std::exception& e) {
        std::string err = e.what();
        LOG_ERROR("Exception during build: " + err);
        return false;
    }
}

std::pair<bool, std::string> BuildTimeAnalyzer::executeBuild(const std::string& target) const {
    if (!validateEnvironment()) {
        return {false, "Environment validation failed"};
    }
    
    std::string command = bazel_binary_ + " build " + target;
    
    try {
        // 在执行命令前保存当前工作目录
        fs::path original_cwd = fs::current_path();
        
        try {
            // 切换到工作空间目录
            fs::current_path(workspace_path_);
            
            // 执行构建命令
            auto [output, exit_code] = PipeCommandExecutor::executeWithStatus(command);
            
            // 恢复原始工作目录
            fs::current_path(original_cwd);
            
            return {exit_code == 0, output};
        } catch (...) {
            // 发生异常时恢复原始工作目录
            fs::current_path(original_cwd);
            throw;
        }
    } catch (const std::exception& e) {
        return {false, std::string("Exception: ") + e.what()};
    }
}

json BuildTimeAnalyzer::loadProfileJson() const {
    std::lock_guard<std::mutex> lock(profile_loading_mutex);
    
    if (!fs::exists(profile_file_path_)) {
        throw std::runtime_error("Profile file not found: " + profile_file_path_);
    }
    
    try {
        LOG_INFO("Loading profile JSON from: " + profile_file_path_);
        
        // 检查文件大小
        auto file_size = fs::file_size(profile_file_path_);
        LOG_INFO("Profile file size: " + formatMemory(file_size));
        
        std::ifstream file(profile_file_path_);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open profile file: " + profile_file_path_);
        }
        
        LOG_INFO("Starting to parse JSON...");
        auto start_time = std::chrono::high_resolution_clock::now();
        
        json profile_data;
        file >> profile_data;
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        LOG_INFO("JSON parsing completed in " + std::to_string(duration) + "ms");
        
        // 检查解析后的数据结构
        if (profile_data.is_null()) {
            throw std::runtime_error("Parsed JSON is null");
        }
        
        if (profile_data.contains("traceEvents")) {
            LOG_INFO("Found " + std::to_string(profile_data["traceEvents"].size()) + " trace events");
        } else {
            LOG_WARN("No traceEvents found in profile");
        }
        
        return profile_data;
    } catch (const json::exception& e) {
        std::string error_msg = "Failed to parse JSON: " + std::string(e.what());
        LOG_ERROR(error_msg);
        throw std::runtime_error(error_msg);
    } catch (const std::exception& e) {
        std::string error_msg = "Failed to load profile: " + std::string(e.what());
        LOG_ERROR(error_msg);
        throw std::runtime_error(error_msg);
    }
}

// 添加缺失的成员函数实现
void BuildTimeAnalyzer::analyzeCriticalPath(const json& profile_data, json& result) const {
    json critical_path;
    
    if (profile_data.contains("otherData") && 
        profile_data["otherData"].contains("critical_path")) {
        
        const auto& critical_path_data = profile_data["otherData"]["critical_path"];
        
        critical_path["length"] = critical_path_data.size();
        
        json critical_path_entries = json::array();
        for (const auto& entry : critical_path_data) {
            json path_entry;
            if (entry.contains("description")) {
                path_entry["action"] = entry["description"];
            }
            if (entry.contains("duration")) {
                path_entry["duration_ms"] = entry["duration"];
                path_entry["duration"] = formatDuration(entry["duration"].get<double>() / 1000.0);
            }
            critical_path_entries.push_back(path_entry);
        }
        
        critical_path["entries"] = critical_path_entries;
    }
    
    result["critical_path"] = critical_path;
}

void BuildTimeAnalyzer::analyzePhaseTimes(const json& profile_data, json& result) const {
    json phases;
    std::map<std::string, double> phase_times;
    
    if (profile_data.contains("traceEvents")) {
        const auto& events = profile_data["traceEvents"];
        
        for (const auto& event : events) {
            if (event.contains("name") && event.contains("dur")) {
                std::string name = event["name"];
                double duration = event["dur"].get<double>();
                
                // 提取阶段名称（如 "execution phase" 等）
                if (name.find("phase") != std::string::npos) {
                    phase_times[name] += duration;
                }
            }
        }
    }
    
    for (const auto& [phase, time] : phase_times) {
        json phase_info;
        phase_info["time_ms"] = time;
        phase_info["time"] = formatDuration(time / 1000.0);
        phases[phase] = phase_info;
    }
    
    result["build_phases"] = phases;
}

void BuildTimeAnalyzer::analyzeActionCounts(const json& profile_data, json& result) const {
    json action_counts;
    std::map<std::string, int> actions;
    
    if (profile_data.contains("traceEvents")) {
        const auto& events = profile_data["traceEvents"];
        
        for (const auto& event : events) {
            if (event.contains("cat")) {
                std::string category = event["cat"];
                actions[category]++;
            }
        }
    }
    
    int total_actions = 0;
    for (const auto& [category, count] : actions) {
        action_counts[category] = count;
        total_actions += count;
    }
    
    action_counts["total"] = total_actions;
    result["action_counts"] = action_counts;
}

void BuildTimeAnalyzer::analyzeCachePerformance(const json& profile_data, json& result) const {
    json cache_stats;
    
    if (profile_data.contains("otherData") && 
        profile_data["otherData"].contains("action_cache")) {
        
        const auto& action_cache = profile_data["otherData"]["action_cache"];
        cache_stats["hits"] = action_cache.value("hits", 0);
        cache_stats["misses"] = action_cache.value("misses", 0);
        
        int total = cache_stats["hits"].get<int>() + cache_stats["misses"].get<int>();
        if (total > 0) {
            double hit_rate = (cache_stats["hits"].get<double>() / total) * 100;
            cache_stats["hit_rate_percent"] = hit_rate;
        }
    }
    
    result["cache_performance"] = cache_stats;
}

json BuildTimeAnalyzer::analyzeProfile() const {
    LOG_INFO("Starting profile analysis...");
    
    try {
        auto profile_data = loadProfileJson();
        json analysis_result;
        
        // 1. 基本信息
        analysis_result["profile_file"] = profile_file_path_;
        auto file_size = fs::file_size(profile_file_path_);
        analysis_result["file_size_bytes"] = file_size;
        analysis_result["file_size_human"] = formatMemory(file_size);
        
        // 2. 总构建时间
        if (profile_data.contains("traceEvents")) {
            const auto& events = profile_data["traceEvents"];
            if (!events.empty() && events.back().contains("ts")) {
                double total_time_ms = events.back()["ts"].get<double>();
                analysis_result["total_build_time_ms"] = total_time_ms;
                analysis_result["total_build_time"] = formatDuration(total_time_ms / 1000.0);
            }
        }
        
        // 3. 分析关键路径
        analyzeCriticalPath(profile_data, analysis_result);
        
        // 4. 分析各阶段时间
        analyzePhaseTimes(profile_data, analysis_result);
        
        // 5. 分析action计数
        analyzeActionCounts(profile_data, analysis_result);
        
        // 6. 分析缓存性能
        analyzeCachePerformance(profile_data, analysis_result);
        
        LOG_INFO("Profile analysis completed");
        return analysis_result;
        
    } catch (const std::exception& e) {
        std::string err = e.what();
        LOG_ERROR("Failed to analyze profile: " + err);
        throw;
    }
}

std::map<std::string, double> BuildTimeAnalyzer::getBuildTimeBreakdown() const {
    std::map<std::string, double> breakdown;
    
    try {
        auto profile_data = loadProfileJson();
        
        // 直接从原始数据中分析，不调用 analyzeProfile()
        if (profile_data.contains("traceEvents")) {
            const auto& events = profile_data["traceEvents"];
            std::map<std::string, double> phase_times;
            
            for (const auto& event : events) {
                if (event.contains("name") && event.contains("dur")) {
                    std::string name = event["name"];
                    double duration = event["dur"].get<double>();
                    
                    if (name.find("phase") != std::string::npos) {
                        phase_times[name] += duration;
                    }
                }
            }
            
            for (const auto& [phase, time] : phase_times) {
                breakdown[phase] = time;
            }
        }
    } catch (const std::exception& e) {
        std::string err = e.what();
        LOG_ERROR("Failed to get build time breakdown: " + err);
    }
    
    return breakdown;
}

std::vector<std::pair<std::string, double>> BuildTimeAnalyzer::getTopTimeConsumingTargets(int top_n) const {
    std::vector<std::pair<std::string, double>> top_targets;
    
    try {
        auto profile_data = loadProfileJson();
        
        if (profile_data.contains("traceEvents")) {
            std::map<std::string, double> target_times;
            
            for (const auto& event : profile_data["traceEvents"]) {
                if (event.contains("args") && event["args"].contains("target")) {
                    std::string target = event["args"]["target"];
                    if (event.contains("dur")) {
                        target_times[target] += event["dur"].get<double>();
                    }
                }
            }
            
            // 转换为vector并排序
            for (const auto& [target, time] : target_times) {
                top_targets.emplace_back(target, time);
            }
            
            // 按耗时降序排序
            std::sort(top_targets.begin(), top_targets.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });
            
            // 限制返回数量
            if (top_n > 0 && top_targets.size() > static_cast<size_t>(top_n)) {
                top_targets.resize(top_n);
            }
        }
    } catch (const std::exception& e) {
        std::string err = e.what();
        LOG_ERROR("Failed to get top time-consuming targets: " + err);
    }
    
    return top_targets;
}

std::map<std::string, size_t> BuildTimeAnalyzer::getMemoryUsage() const {
    std::map<std::string, size_t> memory_usage;
    
    try {
        // 只返回文件大小信息，避免循环调用
        auto file_size = fs::file_size(profile_file_path_);
        memory_usage["profile_file_size"] = file_size;
        
    } catch (const std::exception& e) {
        std::string err = e.what();
        LOG_ERROR("Failed to get memory usage: " + err);
    }
    
    return memory_usage;
}

std::string BuildTimeAnalyzer::generateBuildReport() const {
    std::stringstream report;
    
    try {
        // 只调用一次 analyzeProfile()
        auto analysis = analyzeProfile();
        
        report << "========================================\n";
        report << "         BUILD ANALYSIS REPORT         \n";
        report << "========================================\n\n";
        
        // 基本信息
        report << "1. BASIC INFORMATION:\n";
        report << "   Workspace: " << workspace_path_ << "\n";
        report << "   Profile file: " << analysis["profile_file"] << "\n";
        report << "   File size: " << analysis["file_size_human"] << "\n\n";
        
        // 构建时间
        if (analysis.contains("total_build_time")) {
            report << "2. BUILD TIME:\n";
            report << "   Total time: " << analysis["total_build_time"] << "\n\n";
        }
        
        // 关键路径
        if (analysis.contains("critical_path") && 
            analysis["critical_path"].contains("length")) {
            
            report << "3. CRITICAL PATH:\n";
            report << "   Length: " << analysis["critical_path"]["length"] << " actions\n";
            
            if (analysis["critical_path"].contains("entries")) {
                int i = 1;
                for (const auto& entry : analysis["critical_path"]["entries"]) {
                    report << "   " << i++ << ". " << entry["action"] << " (" 
                           << entry["duration"] << ")\n";
                    if (i > 10) {  // 只显示前10个
                        report << "   ... (and " << (analysis["critical_path"]["length"].get<int>() - 10) << " more)\n";
                        break;
                    }
                }
            }
            report << "\n";
        }
        
        // 阶段时间
        if (analysis.contains("build_phases")) {
            report << "4. BUILD PHASES:\n";
            for (const auto& [phase, phase_info] : analysis["build_phases"].items()) {
                report << "   " << phase << ": " << phase_info["time"] << "\n";
            }
            report << "\n";
        }
        
        // Action统计
        if (analysis.contains("action_counts")) {
            report << "5. ACTION COUNTS:\n";
            for (const auto& [action, count] : analysis["action_counts"].items()) {
                if (action != "total") {
                    report << "   " << action << ": " << count << "\n";
                }
            }
            report << "   Total actions: " << analysis["action_counts"]["total"] << "\n\n";
        }
        
        // 缓存性能
        if (analysis.contains("cache_performance")) {
            const auto& cache = analysis["cache_performance"];
            report << "6. CACHE PERFORMANCE:\n";
            
            if (cache.contains("hits") && cache.contains("misses")) {
                int hits = cache["hits"];
                int misses = cache["misses"];
                int total = hits + misses;
                
                report << "   Hits: " << hits << "\n";
                report << "   Misses: " << misses << "\n";
                report << "   Total: " << total << "\n";
                
                if (cache.contains("hit_rate_percent") && total > 0) {
                    report << "   Hit rate: " << std::fixed << std::setprecision(2)
                           << cache["hit_rate_percent"].get<double>() << "%\n";
                }
            }
            report << "\n";
        }
        
        // 最耗时的目标
        auto top_targets = getTopTimeConsumingTargets(5);
        if (!top_targets.empty()) {
            report << "7. TOP TIME-CONSUMING TARGETS:\n";
            for (size_t i = 0; i < top_targets.size(); ++i) {
                report << "   " << (i + 1) << ". " << top_targets[i].first << " (" 
                       << formatDuration(top_targets[i].second / 1000.0) << ")\n";
            }
        }
        
        report << "\n========================================\n";
        report << "           END OF REPORT              \n";
        report << "========================================\n";
        
    } catch (const std::exception& e) {
        report << "Error generating report: " << e.what() << "\n";
    }
    
    return report.str();
}

std::set<std::string> BuildTimeAnalyzer::getAllTargets() const {
    std::set<std::string> targets;
    
    try {
        auto profile_data = loadProfileJson();
        
        if (profile_data.contains("traceEvents")) {
            for (const auto& event : profile_data["traceEvents"]) {
                if (event.contains("args") && event["args"].contains("target")) {
                    targets.insert(event["args"]["target"].get<std::string>());
                }
            }
        }
    } catch (const std::exception& e) {
        std::string err = e.what();
        LOG_ERROR("Failed to get all targets: " + err);
    }
    
    return targets;
}

std::map<std::string, std::set<std::string>> BuildTimeAnalyzer::getTargetDependencies() const {
    std::map<std::string, std::set<std::string>> dependencies;
    
    try {
        auto profile_data = loadProfileJson();
        
        // TODO 这里可以添加从profile中提取依赖关系的逻辑
        // 这需要分析事件之间的父子关系
        
        LOG_WARN("Target dependency extraction not fully implemented");
        
    } catch (const std::exception& e) {
        std::string err = e.what();
        LOG_ERROR("Failed to get target dependencies: " + err);
    }
    
    return dependencies;
}

void BuildTimeAnalyzer::cleanupProfile() const {
    try {
        if (fs::exists(profile_file_path_)) {
            fs::remove(profile_file_path_);
            LOG_INFO("Cleaned up profile file: " + profile_file_path_);
        }
    } catch (const fs::filesystem_error& e) {
        std::string err = e.what();
        LOG_WARN("Failed to cleanup profile file: " + err);
    }
}

void BuildTimeAnalyzer::setCustomProfileOptions(const std::string& options) {
    profile_options_ = options;
}

std::string BuildTimeAnalyzer::getProfilePath() const {
    return profile_file_path_;
}

std::string BuildTimeAnalyzer::formatDuration(double seconds) const {
    if (seconds < 1.0) {
        return std::to_string(static_cast<int>(seconds * 1000)) + " ms";
    } else if (seconds < 60.0) {
        return std::to_string(static_cast<int>(seconds)) + " s";
    } else if (seconds < 3600.0) {
        int minutes = static_cast<int>(seconds / 60);
        int secs = static_cast<int>(seconds) % 60;
        return std::to_string(minutes) + " m " + std::to_string(secs) + " s";
    } else {
        int hours = static_cast<int>(seconds / 3600);
        int minutes = static_cast<int>((seconds - hours * 3600) / 60);
        return std::to_string(hours) + " h " + std::to_string(minutes) + " m";
    }
}

std::string BuildTimeAnalyzer::formatMemory(size_t bytes) const {
    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    
    if (bytes >= GB) {
        ss << (bytes / GB) << " GB";
    } else if (bytes >= MB) {
        ss << (bytes / MB) << " MB";
    } else if (bytes >= KB) {
        ss << (bytes / KB) << " KB";
    } else {
        ss << bytes << " B";
    }
    
    return ss.str();
}