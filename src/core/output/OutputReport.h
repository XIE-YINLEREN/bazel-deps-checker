#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <ostream>

#include "analysis/CycleDetector.h"
#include "struct.h"

class OutputReport {
public:
    static OutputReport* instance;
    static OutputReport* getInstance();

    ~OutputReport ();

    // 设置输出路径
    void SetOutputPath(const std::string& path) { output_path_ = path; }
    
    // 设置是否包含建议
    void SetIncludeSuggestions(bool include) { include_suggestions_ = include; }
    
    // 生成循环依赖分析报告
    void GenerateCycleReport(const std::vector<CycleAnalysis>& cycles, const OutputFormat& format);
    
    // 生成未使用依赖报告
    void GenerateUnusedDependenciesReport(const std::vector<RemovableDependency>& unused_dependencies, const OutputFormat& format);
    
private:
    // 内部实现方法
    void GenerateReport(const std::vector<CycleAnalysis>& cycles, const OutputFormat& format, std::ostream& output_stream);
    void GenerateUnusedDependenciesReport(const std::vector<RemovableDependency>& unused_dependencies, const OutputFormat& format, std::ostream& output_stream);
    
    // 循环依赖报告生成方法
    void GenerateConsoleReport(const std::vector<CycleAnalysis>& cycles, std::ostream& os);
    void GenerateMarkdownReport(const std::vector<CycleAnalysis>& cycles, std::ostream& os);
    void GenerateJsonReport(const std::vector<CycleAnalysis>& cycles, std::ostream& os);
    void GenerateHtmlReport(const std::vector<CycleAnalysis>& cycles, std::ostream& os);
    
    // 未使用依赖报告生成方法
    void GenerateUnusedDependenciesConsoleReport(const std::vector<RemovableDependency>& unused_dependencies, std::ostream& os);
    void GenerateUnusedDependenciesMarkdownReport(const std::vector<RemovableDependency>& unused_dependencies, std::ostream& os);
    void GenerateUnusedDependenciesJsonReport(const std::vector<RemovableDependency>& unused_dependencies, std::ostream& os);
    void GenerateUnusedDependenciesHtmlReport(const std::vector<RemovableDependency>& unused_dependencies, std::ostream& os);
    
    // 辅助方法
    std::string FormatCyclePath(const std::vector<std::string>& cycle) const;
    std::string ConfidenceLevelToString(ConfidenceLevel level);
    std::string EscapeJsonString(const std::string& str) const;
    std::string GetCurrentTimestamp() const;
    
private:
    std::string output_path_;
    bool include_suggestions_ = true;
};
