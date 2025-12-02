#include "SourceAnalyzer.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <stack>

SourceAnalyzer::SourceAnalyzer(const std::unordered_map<std::string, BazelTarget>& targets) 
    : targets_(targets) {
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
    for (const auto& src : target.srcs) {
        std::string extension = GetFileExtension(src);
        if (IsHeaderFileExtension(extension)) {
            // 提取头文件名
            size_t last_slash = src.find_last_of('/');
            std::string header_name = (last_slash != std::string::npos) ? 
                src.substr(last_slash + 1) : src;
            analysis.provided_headers.insert(header_name);
        }
    }
    
    // 分析所有源文件
    for (const auto& src : target.srcs) {
        std::string extension = GetFileExtension(src);
        if (IsSourceFileExtension(extension)) {
            SourceInfo src_info;
            if (ParseSourceFile(src, src_info)) {
                analysis.source_files.push_back(src_info);
                
                // 收集所有包含的头文件
                analysis.included_headers.insert(
                    src_info.includes.begin(),
                    src_info.includes.end()
                );
                
                // 递归分析包含的头文件
                RecursivelyAnalyzeHeaderIncludes(src, src_info.includes, analysis);
            }
        }
        else if (IsHeaderFileExtension(extension)) {
            HeaderInfo hdr_info;
            if (ParseHeaderFile(src, hdr_info)) {
                analysis.header_files.push_back(hdr_info);
            }
        }
    }
    
    // 缓存分析结果
    std::lock_guard<std::mutex> lock(analysis_mutex_);
    target_analysis_[target_name] = std::move(analysis);
    analyzed_targets_.insert(target_name);
}

void SourceAnalyzer::RecursivelyAnalyzeHeaderIncludes(const std::string& source_file, 
                                                    const std::unordered_set<std::string>& direct_includes,
                                                    TargetAnalysis& analysis) {
    std::unordered_set<std::string> visited_headers;
    std::stack<std::string> to_analyze;
    
    // 初始化栈
    for (const auto& header : direct_includes) {
        to_analyze.push(header);
    }
    
    while (!to_analyze.empty()) {
        std::string header_name = to_analyze.top();
        to_analyze.pop();
        
        // 防止循环包含
        if (visited_headers.find(header_name) != visited_headers.end()) {
            continue;
        }
        visited_headers.insert(header_name);
        
        // 查找头文件路径
        std::string header_path = FindHeaderPath(header_name);
        if (header_path.empty()) {
            LOG_DEBUG("Cannot find header file: " + header_name);
            continue;
        }
        
        // 解析头文件
        HeaderInfo hdr_info;
        if (ParseHeaderFile(header_path, hdr_info)) {
            // 将新发现的头文件加入待分析栈
            for (const auto& include : hdr_info.includes) {
                if (visited_headers.find(include) == visited_headers.end()) {
                    to_analyze.push(include);
                }
            }
        }
    }
}

std::string SourceAnalyzer::FindHeaderPath(const std::string& header_name) {
    // 简单的查找逻辑 @TODO
    
    // 检查是否已经是完整路径
    if (header_name.find('/') != std::string::npos) {
        // 尝试直接打开
        std::ifstream test(header_name);
        if (test.is_open()) {
            test.close();
            return header_name;
        }
    }
    
    // 在当前目录中查找
    std::ifstream test(header_name);
    if (test.is_open()) {
        test.close();
        return header_name;
    }
    
    return "";
}

bool SourceAnalyzer::ParseSourceFile(const std::string& file_path, SourceInfo& result) {
    // 提取文件名
    size_t last_slash = file_path.find_last_of('/');
    if (last_slash != std::string::npos) {
        result.file_name = file_path.substr(last_slash + 1);
    } else {
        result.file_name = file_path;
    }
    result.path = file_path;
    
    std::ifstream file(file_path);
    if (!file.is_open()) {
        LOG_WARN("Cannot open source file: " + file_path);
        return false;
    }
    
    std::string line;
    
    while (std::getline(file, line)) {
        // 提取包含的头文件
        ExtractIncludesFromLine(line, result.includes);
    }
    
    return true;
}

bool SourceAnalyzer::ParseHeaderFile(const std::string& file_path, HeaderInfo& result) {
    // 提取文件名
    size_t last_slash = file_path.find_last_of('/');
    if (last_slash != std::string::npos) {
        result.file_name = file_path.substr(last_slash + 1);
    } else {
        result.file_name = file_path;
    }
    result.path = file_path;
    
    std::ifstream file(file_path);
    if (!file.is_open()) {
        LOG_WARN("Cannot open header file: " + file_path);
        return false;
    }
    
    std::string line;
    
    while (std::getline(file, line)) {
        // 提取包含的头文件
        ExtractIncludesFromLine(line, result.includes);
    }
    
    return true;
}

void SourceAnalyzer::ExtractIncludesFromLine(const std::string& line, std::unordered_set<std::string>& includes) {
    // 匹配 #include
    std::regex include_regex(R"(#\s*include\s*[<\"]([^>\"]+)[>\"])");
    std::smatch match;
    
    if (std::regex_search(line, match, include_regex)) {
        std::string include_path = match[1].str();
        
        // 提取文件名
        size_t last_slash = include_path.find_last_of('/');
        if (last_slash != std::string::npos) {
            includes.insert(include_path.substr(last_slash + 1));
        } else {
            includes.insert(include_path);
        }
    }
}

bool SourceAnalyzer::IsHeaderUsed(const std::string& target_name, const std::string& header_path) {
    EnsureTargetAnalyzed(target_name);
    
    std::lock_guard<std::mutex> lock(analysis_mutex_);
    auto it = target_analysis_.find(target_name);
    if (it == target_analysis_.end()) {
        return false;
    }
    
    const auto& analysis = it->second;
    // 提取头文件名
    size_t last_slash = header_path.find_last_of('/');
    std::string header_name = (last_slash != std::string::npos) ? 
        header_path.substr(last_slash + 1) : header_path;
    
    // 检查是否包含了这个头文件
    return analysis.included_headers.find(header_name) != analysis.included_headers.end();
}

bool SourceAnalyzer::IsDependencyNeeded(const std::string& target_name, const std::string& dependency) {
    if (target_name == dependency) {
        LOG_DEBUG("Self-dependency detected: " + target_name);
        // 自依赖不应该存在
        return false;
    }
    
    EnsureTargetAnalyzed(target_name);
    EnsureTargetAnalyzed(dependency);
    
    // 获取目标包含的所有头文件
    const std::unordered_set<std::string>& target_headers = GetTargetIncludedHeaders(target_name);
    if (target_headers.empty()) {
        LOG_DEBUG("Target " + target_name + " includes no headers");
        return false;
    }
    
    // 获取依赖提供的所有头文件
    const std::unordered_set<std::string>& dep_headers = GetTargetProvidedHeaders(dependency);
    if (dep_headers.empty()) {
        LOG_DEBUG("Dependency " + dependency + " provides no headers");
        return false;
    }
    
    LOG_DEBUG("Checking if " + target_name + " needs " + dependency);
    LOG_DEBUG("Target includes " + std::to_string(target_headers.size()) + " headers");
    LOG_DEBUG("Dependency provides " + std::to_string(dep_headers.size()) + " headers");
    
    // 检查目标是否使用了依赖提供的头文件
    for (const auto& header : dep_headers) {
        if (target_headers.find(header) != target_headers.end()) {
            LOG_DEBUG("Target " + target_name + " uses header " + header + " from " + dependency);
            return true;
        }
    }
    
    LOG_DEBUG("Dependency " + dependency + " is NOT needed by " + target_name);
    return false;
}

std::vector<RemovableDependency> SourceAnalyzer::GetRemovableDependencies(const std::string& target_name) {
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
    {
        std::lock_guard<std::mutex> lock(analysis_mutex_);
        if (analyzed_targets_.find(target_name) != analyzed_targets_.end()) {
            // 已经分析过
            return;
        }
    }
    
    // 分析目标
    AnalyzeTarget(target_name);
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
        for (const auto& src : it->second.srcs) {
            std::string ext = GetFileExtension(src);
            if (IsHeaderFileExtension(ext)) {
                paths.push_back(src);
            }
        }
    }
    return paths;
}

void SourceAnalyzer::ClearCache() {
    std::lock_guard<std::mutex> lock(analysis_mutex_);
    target_analysis_.clear();
    analyzed_targets_.clear();
}

void SourceAnalyzer::ClearTargetCache(const std::string& target_name) {
    std::lock_guard<std::mutex> lock(analysis_mutex_);
    target_analysis_.erase(target_name);
    analyzed_targets_.erase(target_name);
}
