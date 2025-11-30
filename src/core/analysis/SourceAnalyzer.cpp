#include "SourceAnalyzer.h"


SourceAnalyzer::SourceAnalyzer(std::unordered_map<std::string, BazelTarget> targets) 
    : targets_(std::move(targets)) {
    AnalyzeAllTargets();
}

void SourceAnalyzer::AnalyzeTarget(const std::string& target_name) {
    auto target_it = targets_.find(target_name);
    if (target_it == targets_.end()) {
        return;
    }

    const auto& target = target_it->second;
    std::vector<SourceFile> source_files;
    std::vector<HeaderFile> header_files;
    std::unordered_set<std::string> used_symbols;
    std::unordered_set<std::string> provided_symbols;

    // 分析所有源文件和头文件
    for (const auto& src : target.srcs) {
        std::string full_path = target.path + "/" + src;
        
        if (IsSourceFile(src)) {
            SourceFile source_file;
            ParseSourceFile(full_path, source_file);
            source_files.push_back(source_file);
            
            // 合并使用的符号
            used_symbols.insert(source_file.used_symbols.begin(), source_file.used_symbols.end());
        } 
        else if (IsHeaderFile(src)) {
            HeaderFile header_file;
            ParseHeaderFile(full_path, header_file);
            header_files.push_back(header_file);
            
            // 合并提供的符号
            provided_symbols.insert(header_file.provided_symbols.begin(), header_file.provided_symbols.end());
        }
    }

    // 更新缓存
    target_sources_[target_name] = std::move(source_files);
    target_headers_[target_name] = std::move(header_files);
    target_used_symbols_[target_name] = std::move(used_symbols);
    target_provided_symbols_[target_name] = std::move(provided_symbols);
}

void SourceAnalyzer::AnalyzeAllTargets() {
    for (const auto& [name, target] : targets_) {
        AnalyzeTarget(name);
    }
}

std::vector<std::string> SourceAnalyzer::GetUnusedHeaders(
    const std::string& target_name, const std::string& dependency) const {
    
    std::vector<std::string> unused_headers;
    
    // 获取依赖提供的所有头文件
    auto dep_headers = GetDependencyHeaders(dependency);
    
    // 检查目标是否使用了这些头文件
    for (const auto& header : dep_headers) {
        if (!IsHeaderUsed(target_name, header)) {
            unused_headers.push_back(header);
        }
    }
    
    return unused_headers;
}

std::vector<std::string> SourceAnalyzer::GetUnusedSymbols(
    const std::string& target_name, const std::string& dependency) const {
    
    std::vector<std::string> unused_symbols;
    
    // 获取依赖提供的所有符号
    auto dep_symbols = GetDependencyProvidedSymbols(dependency);
    
    // 检查目标是否使用了这些符号
    for (const auto& symbol : dep_symbols) {
        if (!IsSymbolUsed(target_name, symbol)) {
            unused_symbols.push_back(symbol);
        }
    }
    
    return unused_symbols;
}

bool SourceAnalyzer::IsDependencyNeeded(const std::string& target_name, const std::string& dependency) const {
    auto unused_headers = GetUnusedHeaders(target_name, dependency);
    auto unused_symbols = GetUnusedSymbols(target_name, dependency);
    
    // 如果所有头文件和符号都未使用，则依赖不需要
    auto dep_headers = GetDependencyHeaders(dependency);
    auto dep_symbols = GetDependencyProvidedSymbols(dependency);
    
    return !(unused_headers.size() == dep_headers.size() && unused_symbols.size() == dep_symbols.size());
}

std::vector<RemovableDependency> SourceAnalyzer::GetRemovableDependencies(const std::string& target_name) const {
    std::vector<RemovableDependency> removable_deps;
    
    auto target_it = targets_.find(target_name);
    if (target_it == targets_.end()) {
        return removable_deps;
    }
    
    for (const auto& dep : target_it->second.deps) {
        if (!IsDependencyNeeded(target_name, dep)) {
            RemovableDependency removable;
            removable.from_target = target_name;
            removable.to_target = dep;
            removable.unused_headers = GetUnusedHeaders(target_name, dep);
            removable.unused_symbols = GetUnusedSymbols(target_name, dep);
            removable.reason = "All headers and symbols are unused";
            removable_deps.push_back(removable);
        }
    }
    
    return removable_deps;
}

// 文件类型判断
bool SourceAnalyzer::IsSourceFile(const std::string& file_path) const {
    std::string ext = std::filesystem::path(file_path).extension().string();
    return ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".c";
}

bool SourceAnalyzer::IsHeaderFile(const std::string& file_path) const {
    std::string ext = std::filesystem::path(file_path).extension().string();
    return ext == ".h" || ext == ".hpp" || ext == ".hxx" || ext == ".hh";
}

// 文件解析实现
void SourceAnalyzer::ParseSourceFile(const std::string& file_path, SourceFile& result) {
    result.path = file_path;
    
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    result.content = buffer.str();
    
    result.included_headers = ExtractIncludes(result.content);
    result.used_symbols = ExtractUsedSymbols(result.content);
}

void SourceAnalyzer::ParseHeaderFile(const std::string& file_path, HeaderFile& result) {
    result.path = file_path;
    
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    result.content = buffer.str();
    
    result.provided_symbols = ExtractProvidedSymbols(result.content);
}

// 内容分析实现
std::vector<std::string> SourceAnalyzer::ExtractIncludes(const std::string& content) {
    std::vector<std::string> includes;
    std::regex include_regex(R"(#\s*include\s*[<\"]([^>\"]+)[>\"])");
    
    auto begin = std::sregex_iterator(content.begin(), content.end(), include_regex);
    auto end = std::sregex_iterator();
    
    for (std::sregex_iterator i = begin; i != end; ++i) {
        std::smatch match = *i;
        includes.push_back(match[1].str());
    }
    
    return includes;
}

std::unordered_set<std::string> SourceAnalyzer::ExtractUsedSymbols(const std::string& content) {
    std::unordered_set<std::string> symbols;
    ExtractFunctionsAndClasses(content, symbols);
    ExtractVariables(content, symbols);
    ExtractTypeDefs(content, symbols);
    return symbols;
}

std::unordered_set<std::string> SourceAnalyzer::ExtractProvidedSymbols(const std::string& content) {
    return ExtractUsedSymbols(content); // 简化实现，实际应该更精确
}

// 符号提取具体实现
void SourceAnalyzer::ExtractFunctionsAndClasses(const std::string& content, std::unordered_set<std::string>& symbols) {
    // 匹配函数和类定义
    std::regex func_regex(R"((\w+)\s+\w+\s*\([^)]*\)\s*\{)");
    std::regex class_regex(R"(class\s+(\w+))");
    
    auto func_begin = std::sregex_iterator(content.begin(), content.end(), func_regex);
    auto func_end = std::sregex_iterator();
    for (auto i = func_begin; i != func_end; ++i) {
        symbols.insert((*i)[1].str());
    }
    
    auto class_begin = std::sregex_iterator(content.begin(), content.end(), class_regex);
    auto class_end = std::sregex_iterator();
    for (auto i = class_begin; i != class_end; ++i) {
        symbols.insert((*i)[1].str());
    }
}

void SourceAnalyzer::ExtractVariables(const std::string& content, std::unordered_set<std::string>& symbols) {
    std::regex var_regex(R"(\b(\w+)\s*=\s*[^;]+;)");
    auto begin = std::sregex_iterator(content.begin(), content.end(), var_regex);
    auto end = std::sregex_iterator();
    for (auto i = begin; i != end; ++i) {
        symbols.insert((*i)[1].str());
    }
}

void SourceAnalyzer::ExtractTypeDefs(const std::string& content, std::unordered_set<std::string>& symbols) {
    std::regex typedef_regex(R"(typedef\s+[^;]+\s+(\w+)\s*;)");
    auto begin = std::sregex_iterator(content.begin(), content.end(), typedef_regex);
    auto end = std::sregex_iterator();
    for (auto i = begin; i != end; ++i) {
        symbols.insert((*i)[1].str());
    }
}

// 工具方法实现
bool SourceAnalyzer::IsHeaderUsed(const std::string& target_name, const std::string& header_path) const {
    auto target_it = target_sources_.find(target_name);
    if (target_it == target_sources_.end()) {
        return false;
    }
    
    std::string header_name = GetFileName(header_path);
    
    for (const auto& source_file : target_it->second) {
        for (const auto& included_header : source_file.included_headers) {
            if (GetFileName(included_header) == header_name) {
                return true;
            }
        }
    }
    
    return false;
}

bool SourceAnalyzer::IsSymbolUsed(const std::string& target_name, const std::string& symbol) const {
    auto symbol_it = target_used_symbols_.find(target_name);
    if (symbol_it == target_used_symbols_.end()) {
        return false;
    }
    
    return symbol_it->second.find(symbol) != symbol_it->second.end();
}

std::string SourceAnalyzer::GetFileName(const std::string& file_path) const {
    size_t last_slash = file_path.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        return file_path.substr(last_slash + 1);
    }
    return file_path;
}

// 获取目标文件列表
std::vector<std::string> SourceAnalyzer::GetTargetSourceFiles(const std::string& target_name) const {
    std::vector<std::string> paths;
    auto it = target_sources_.find(target_name);
    if (it != target_sources_.end()) {
        for (const auto& source_file : it->second) {
            paths.push_back(source_file.path);
        }
    }
    return paths;
}

std::vector<std::string> SourceAnalyzer::GetTargetHeaders(const std::string& target_name) const {
    std::vector<std::string> paths;
    auto it = target_headers_.find(target_name);
    if (it != target_headers_.end()) {
        for (const auto& header_file : it->second) {
            paths.push_back(header_file.path);
        }
    }
    return paths;
}

std::vector<std::string> SourceAnalyzer::GetDependencyHeaders(const std::string& dependency) const {
    return GetTargetHeaders(dependency);
}

std::unordered_set<std::string> SourceAnalyzer::GetDependencyProvidedSymbols(const std::string& dependency) const {
    auto it = target_provided_symbols_.find(dependency);
    if (it != target_provided_symbols_.end()) {
        return it->second;
    }
    return {};
}