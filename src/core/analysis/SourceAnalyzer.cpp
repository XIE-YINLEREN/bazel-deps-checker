#include "SourceAnalyzer.h"
#include <fstream>
#include <algorithm>
#include <stack>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

std::string GetFileName(const std::string& file_path) {
    return fs::path(file_path).filename().string();
}

void MergeIncludes(const std::unordered_set<std::string>& includes, TargetAnalysis& analysis) {
    analysis.included_headers.insert(includes.begin(), includes.end());
    for (const auto& include : includes) {
        analysis.included_header_names.insert(GetFileName(include));
    }
}

bool IsLikelyHeaderInclude(const std::string& include_name) {
    const auto extension_pos = include_name.find_last_of('.');
    if (extension_pos == std::string::npos) {
        return false;
    }
    const std::string ext = include_name.substr(extension_pos);
    return ext == ".h" || ext == ".hh" || ext == ".hpp" || ext == ".hxx" || ext == ".inl" || ext == ".inc";
}

}  // namespace

SourceAnalyzer::SourceAnalyzer(const std::unordered_map<std::string, BazelTarget>& targets, const std::string workspace_path) 
    : workspace_path_(workspace_path), targets_(targets) {
    for (const auto& [target_name, target] : targets_) {
        for (const auto& hdr : target.hdrs) {
            const std::string extension = GetFileExtension(hdr);
            if (IsHeaderFileExtension(extension)) {
                provided_header_to_targets_[GetFileName(hdr)].insert(target_name);
            }
        }
        for (const auto& src : target.srcs) {
            const std::string extension = GetFileExtension(src);
            if (IsHeaderFileExtension(extension)) {
                provided_header_to_targets_[GetFileName(src)].insert(target_name);
            }
        }
    }
}

SourceAnalyzer::~SourceAnalyzer() {

}

void SourceAnalyzer::AnalyzeTarget(const std::string& target_name) {
    auto target_it = targets_.find(target_name);
    if (target_it == targets_.end()) {
        LOG_WARN("Target not found: " + target_name);
        return;
    }

    const auto& target = target_it->second;
    TargetAnalysis analysis;
    
    // 首先收集目标提供的头文件
    for (const auto& hdrs : target.hdrs) {
        std::string extension = GetFileExtension(hdrs);
        if (IsHeaderFileExtension(extension)) {
            analysis.provided_headers.insert(GetFileName(hdrs));

            HeaderInfo hdr_info;
            if (ParseHeaderFile(hdrs, hdr_info)) {
                MergeIncludes(hdr_info.includes, analysis);
                RecursivelyAnalyzeHeaderIncludes(hdrs, hdr_info.includes, analysis);
            }
        }
    }
    
    // 分析所有源文件
    for (const auto& src : target.srcs) {
        std::string extension = GetFileExtension(src);
        if (IsSourceFileExtension(extension)) {
            SourceInfo src_info;
            if (ParseSourceFile(src, src_info)) {
                MergeIncludes(src_info.includes, analysis);
                RecursivelyAnalyzeHeaderIncludes(src, src_info.includes, analysis);
            }
        }
        else if (IsHeaderFileExtension(extension)) {
            HeaderInfo hdr_info;
            if (ParseHeaderFile(src, hdr_info)) {
                analysis.provided_headers.insert(GetFileName(src));
                MergeIncludes(hdr_info.includes, analysis);
                RecursivelyAnalyzeHeaderIncludes(src, hdr_info.includes, analysis);
            }
        }
    }
    
    // 缓存分析结果
    std::lock_guard<std::mutex> lock(analysis_mutex_);
    target_analysis_[target_name] = std::move(analysis);
    analyzed_targets_.insert(target_name);
}

void SourceAnalyzer::RecursivelyAnalyzeHeaderIncludes([[maybe_unused]] const std::string& source_file,
                                                    const std::unordered_set<std::string>& direct_includes,
                                                    TargetAnalysis& analysis) {
    for (const auto& header : direct_includes) {
        if (!IsLikelyHeaderInclude(header)) {
            continue;
        }
        const auto& recursive_includes = GetRecursiveHeaderIncludes(header);
        MergeIncludes(recursive_includes, analysis);
    }
}

const std::unordered_set<std::string>& SourceAnalyzer::GetRecursiveHeaderIncludes(
    const std::string& header_name) {
    const std::string header_path = FindHeaderPath(header_name);
    if (header_path.empty()) {
        static const std::unordered_set<std::string> empty_set;
        return empty_set;
    }

    {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        auto cache_it = recursive_header_includes_cache_.find(header_path);
        if (cache_it != recursive_header_includes_cache_.end()) {
            return cache_it->second;
        }
    }

    std::unordered_set<std::string> visited_headers;
    std::unordered_set<std::string> aggregate_includes;
    std::stack<std::string> to_analyze;
    to_analyze.push(header_path);

    while (!to_analyze.empty()) {
        std::string current_header_path = to_analyze.top();
        to_analyze.pop();

        if (!visited_headers.insert(current_header_path).second) {
            continue;
        }

        HeaderInfo hdr_info;
        if (!ParseHeaderFile(current_header_path, hdr_info)) {
            continue;
        }

        for (const auto& include : hdr_info.includes) {
            if (!aggregate_includes.insert(include).second) {
                continue;
            }
            if (!IsLikelyHeaderInclude(include)) {
                continue;
            }
            const std::string include_path = FindHeaderPath(include);
            if (!include_path.empty()) {
                to_analyze.push(include_path);
            }
        }
    }

    std::lock_guard<std::mutex> lock(analysis_mutex_);
    auto [inserted_it, _] =
        recursive_header_includes_cache_.emplace(header_path, std::move(aggregate_includes));
    return inserted_it->second;
}

std::string SourceAnalyzer::FindHeaderPath(const std::string& header_name) {
    {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        auto cache_it = header_path_cache_.find(header_name);
        if (cache_it != header_path_cache_.end()) {
            return cache_it->second;
        }
    }

    const std::string resolved_path = ResolveWorkspacePath(header_name);
    {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        header_path_cache_[header_name] = resolved_path;
    }
    return resolved_path;
}

std::string SourceAnalyzer::ResolveWorkspacePath(const std::string& file_path) const {
    if (file_path.empty()) {
        return "";
    }

    {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        const auto cache_it = resolved_path_cache_.find(file_path);
        if (cache_it != resolved_path_cache_.end()) {
            return cache_it->second;
        }
    }

    const fs::path input_path(file_path);
    std::string resolved_path;
    if (input_path.is_absolute() && fs::exists(input_path)) {
        resolved_path = input_path.string();
    } else {
        const fs::path workspace_candidate = fs::path(workspace_path_) / input_path;
        if (fs::exists(workspace_candidate)) {
            resolved_path = workspace_candidate.string();
        } else if (fs::exists(input_path)) {
            resolved_path = input_path.string();
        }
    }

    {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        resolved_path_cache_[file_path] = resolved_path;
    }
    return resolved_path;
}

bool SourceAnalyzer::ParseSourceFile(const std::string& file_path, SourceInfo& result) {
    const std::string resolved_path = ResolveWorkspacePath(file_path);
    result.file_name = GetFileName(file_path);
    result.path = resolved_path.empty() ? file_path : resolved_path;
    return ParseIncludesFromFile(result.path, result.includes, result.file_name);
}

bool SourceAnalyzer::ParseHeaderFile(const std::string& file_path, HeaderInfo& result) {
    const std::string resolved_path = ResolveWorkspacePath(file_path);
    result.file_name = GetFileName(file_path);
    result.path = resolved_path.empty() ? file_path : resolved_path;
    return ParseIncludesFromFile(result.path, result.includes, result.file_name);
}

bool SourceAnalyzer::ParseIncludesFromFile(
    const std::string& resolved_path,
    std::unordered_set<std::string>& includes,
    std::string& file_name) {
    if (resolved_path.empty()) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        auto cache_it = parsed_includes_cache_.find(resolved_path);
        if (cache_it != parsed_includes_cache_.end()) {
            includes = cache_it->second;
            if (file_name.empty()) {
                file_name = GetFileName(resolved_path);
            }
            return true;
        }
    }

    std::ifstream file(resolved_path);
    if (!file.is_open()) {
        bool should_warn = false;
        {
            std::lock_guard<std::mutex> lock(analysis_mutex_);
            should_warn = warned_unreadable_files_.insert(resolved_path).second;
        }
        if (should_warn) {
            LOG_WARN("Cannot open file: " + resolved_path);
        }
        return false;
    }

    std::unordered_set<std::string> parsed_includes;
    std::string line;
    while (std::getline(file, line)) {
        ExtractIncludesFromLine(line, parsed_includes);
    }

    {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        parsed_includes_cache_[resolved_path] = parsed_includes;
    }

    includes = std::move(parsed_includes);
    if (file_name.empty()) {
        file_name = GetFileName(resolved_path);
    }
    return true;
}

void SourceAnalyzer::ExtractIncludesFromLine(const std::string& line, std::unordered_set<std::string>& includes) {
    const size_t include_pos = line.find("#include");
    if (include_pos == std::string::npos) {
        return;
    }

    const size_t first_quote = line.find('"', include_pos);
    if (first_quote == std::string::npos) {
        return;
    }

    const size_t second_quote = line.find('"', first_quote + 1);
    if (second_quote == std::string::npos || second_quote <= first_quote + 1) {
        return;
    }

    includes.insert(line.substr(first_quote + 1, second_quote - first_quote - 1));
}

bool SourceAnalyzer::IsHeaderUsed(const std::string& target_name, const std::string& header_path) {
    EnsureTargetAnalyzed(target_name);
    
    std::lock_guard<std::mutex> lock(analysis_mutex_);
    auto it = target_analysis_.find(target_name);
    if (it == target_analysis_.end()) {
        return false;
    }

    return it->second.included_header_names.find(GetFileName(header_path)) !=
           it->second.included_header_names.end();
}

bool SourceAnalyzer::IsDependencyNeeded(const std::string& target_name, const std::string& dependency) {
    if (target_name == dependency) {
        LOG_DEBUG("Self-dependency detected: " + target_name);
        // 自依赖不应该存在
        return false;
    }

    const std::string cache_key = target_name + '\n' + dependency;
    {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        auto cache_it = dependency_needed_cache_.find(cache_key);
        if (cache_it != dependency_needed_cache_.end()) {
            return cache_it->second;
        }
    }
    
    EnsureTargetAnalyzed(target_name);

    std::lock_guard<std::mutex> lock(analysis_mutex_);
    const auto target_it = target_analysis_.find(target_name);
    if (target_it == target_analysis_.end()) {
        dependency_needed_cache_[cache_key] = false;
        return false;
    }

    const auto& target_header_names = target_it->second.included_header_names;
    if (target_header_names.empty()) {
        LOG_DEBUG("Target " + target_name + " includes no headers");
        dependency_needed_cache_[cache_key] = false;
        return false;
    }

    LOG_DEBUG("Checking if " + target_name + " needs " + dependency);
    LOG_DEBUG("Target includes " + std::to_string(target_header_names.size()) + " headers");
    for (const auto& included_header : target_header_names) {
        const auto provider_it = provided_header_to_targets_.find(included_header);
        if (provider_it != provided_header_to_targets_.end() &&
            provider_it->second.find(dependency) != provider_it->second.end()) {
            LOG_DEBUG("Target " + target_name + " uses header " + included_header + " from " + dependency);
            dependency_needed_cache_[cache_key] = true;
            return true;
        }
    }

    LOG_DEBUG("Dependency " + dependency + " is NOT needed by " + target_name);
    dependency_needed_cache_[cache_key] = false;
    return false;
}

std::vector<RemovableDependency> SourceAnalyzer::GetRemovableDependencies(const std::string& target_name) {
    {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        auto cache_it = removable_dependencies_cache_.find(target_name);
        if (cache_it != removable_dependencies_cache_.end()) {
            return cache_it->second;
        }
    }

    std::vector<RemovableDependency> removable_deps;
    
    auto target_it = targets_.find(target_name);
    if (target_it == targets_.end()) {
        LOG_WARN("Target not found: " + target_name);
        return removable_deps;
    }
    
    const auto& target = target_it->second;
    
    LOG_DEBUG("Checking removable dependencies for target: " + target_name);
    LOG_DEBUG("Target has " + std::to_string(target.deps.size()) + " dependencies");
    
    for (const auto& dep : target.deps) {
        LOG_DEBUG("Checking dependency: " + dep);
        
        // 避免自依赖检查
        if (dep == target_name) {
            LOG_DEBUG("Found self-dependency: " + target_name + " -> " + dep);
            removable_deps.push_back({
                target_name,
                dep,
                "Self-dependency should not exist",
                ConfidenceLevel::HIGH
            });
            continue;
        }
        
        if (!IsDependencyNeeded(target_name, dep)) {
            LOG_INFO("Found removable dependency: " + target_name + " -> " + dep);
            removable_deps.push_back({
                target_name,
                dep,
                "No headers from this dependency are used",
                ConfidenceLevel::HIGH
            });
        } else {
            LOG_DEBUG("Dependency " + dep + " is needed by " + target_name);
        }
    }

    {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        removable_dependencies_cache_[target_name] = removable_deps;
    }
    return removable_deps;
}

std::string SourceAnalyzer::Trim(const std::string& str) const {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

std::string SourceAnalyzer::GetFileExtension(const std::string& file_path) const {
    if (file_path.empty()) {
        return "";
    }
    
    size_t dot_pos = file_path.find_last_of('.');
    if (dot_pos == std::string::npos || dot_pos == file_path.length() - 1) {
        // 没有扩展名
        return ""; 
    }
    
    // 提取扩展名
    std::string ext = file_path.substr(dot_pos);
    
    // 转换为小写
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    
    return ext;
}

bool SourceAnalyzer::IsSourceFileExtension(const std::string& ext) const {
    static const std::unordered_set<std::string> source_extensions = {
        ".c", ".cc", ".cpp", ".cxx", ".c++",
        ".m", ".mm"
    };
    return source_extensions.find(ext) != source_extensions.end();
}

bool SourceAnalyzer::IsHeaderFileExtension(const std::string& ext) const {
    static const std::unordered_set<std::string> header_extensions = {
        ".h", ".hh", ".hpp", ".hxx", ".h++",
        ".inc", ".inl"
    };
    return header_extensions.find(ext) != header_extensions.end();
}

void SourceAnalyzer::EnsureTargetAnalyzed(const std::string& target_name) {
    bool should_analyze = false;
    {
        std::unique_lock<std::mutex> lock(analysis_mutex_);
        while (analyzed_targets_.find(target_name) == analyzed_targets_.end()) {
            if (analyzing_targets_.insert(target_name).second) {
                should_analyze = true;
                break;
            }
            analysis_cv_.wait(lock, [&]() {
                return analyzed_targets_.find(target_name) != analyzed_targets_.end() ||
                       analyzing_targets_.find(target_name) == analyzing_targets_.end();
            });
        }
        if (!should_analyze) {
            return;
        }
    }

    try {
        AnalyzeTarget(target_name);
    } catch (...) {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        analyzing_targets_.erase(target_name);
        analysis_cv_.notify_all();
        throw;
    }

    {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        analyzing_targets_.erase(target_name);
    }
    analysis_cv_.notify_all();
}

const std::unordered_set<std::string>& SourceAnalyzer::GetTargetIncludedHeaders(const std::string& target_name) {
    EnsureTargetAnalyzed(target_name);
    
    std::lock_guard<std::mutex> lock(analysis_mutex_);
    auto it = target_analysis_.find(target_name);
    if (it != target_analysis_.end()) {
        return it->second.included_headers;
    }
    
    // 返回空集合的引用
    static const std::unordered_set<std::string> empty_set;
    return empty_set;
}

const std::unordered_set<std::string>& SourceAnalyzer::GetTargetProvidedHeaders(const std::string& target_name) {
    EnsureTargetAnalyzed(target_name);
    
    std::lock_guard<std::mutex> lock(analysis_mutex_);
    auto it = target_analysis_.find(target_name);
    if (it != target_analysis_.end()) {
        return it->second.provided_headers;
    }
    
    // 返回空集合的引用
    static const std::unordered_set<std::string> empty_set;
    return empty_set;
}

std::vector<std::string> SourceAnalyzer::GetTargetSourceFiles(const std::string& target_name) const {
    std::vector<std::string> paths;
    auto it = targets_.find(target_name);
    if (it != targets_.end()) {
        for (const auto& src : it->second.srcs) {
            std::string ext = GetFileExtension(src);
            if (IsSourceFileExtension(ext)) {
                paths.push_back(src);
            }
        }
    }
    return paths;
}

std::vector<std::string> SourceAnalyzer::GetTargetHeaderFiles(const std::string& target_name) const {
    std::vector<std::string> paths;
    auto it = targets_.find(target_name);
    if (it != targets_.end()) {
        for (const auto& hdr : it->second.hdrs) {
            std::string ext = GetFileExtension(hdr);
            if (IsHeaderFileExtension(ext)) {
                paths.push_back(hdr);
            }
        }
    }
    return paths;
}

void SourceAnalyzer::ClearCache() {
    std::lock_guard<std::mutex> lock(analysis_mutex_);
    target_analysis_.clear();
    analyzed_targets_.clear();
    analyzing_targets_.clear();
    header_path_cache_.clear();
    resolved_path_cache_.clear();
    parsed_includes_cache_.clear();
    warned_unreadable_files_.clear();
    recursive_header_includes_cache_.clear();
    dependency_needed_cache_.clear();
    removable_dependencies_cache_.clear();
    analysis_cv_.notify_all();
}

void SourceAnalyzer::ClearTargetCache(const std::string& target_name) {
    std::lock_guard<std::mutex> lock(analysis_mutex_);
    target_analysis_.erase(target_name);
    analyzed_targets_.erase(target_name);
    analyzing_targets_.erase(target_name);
    removable_dependencies_cache_.erase(target_name);

    const std::string target_prefix = target_name + '\n';
    for (auto it = dependency_needed_cache_.begin(); it != dependency_needed_cache_.end();) {
        if (it->first.rfind(target_prefix, 0) == 0) {
            it = dependency_needed_cache_.erase(it);
        } else {
            ++it;
        }
    }
    analysis_cv_.notify_all();
}
