#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <ostream>

#include "analysis/CycleDetector.h"
#include "struct.h"

class OutputReport {
public:
    OutputReport() = default;
    
    void GenerateReport(const std::vector<CycleAnalysis>& cycles, const OutputFormat& format);
    void GenerateReport(const std::vector<CycleAnalysis>& cycles, const OutputFormat& format, std::ostream& output_stream);
    
    // 设置输出选项
    void SetIncludeDetails(bool include) { include_details_ = include; }
    void SetIncludeSuggestions(bool include) { include_suggestions_ = include; }
    void SetOutputPath(const std::string& path) { output_path_ = path; }

private:
    bool include_details_ = true;
    bool include_suggestions_ = true;
    std::string output_path_;
    
    // 不同格式的输出方法
    void GenerateConsoleReport(const std::vector<CycleAnalysis>& cycles, std::ostream& os);
    void GenerateMarkdownReport(const std::vector<CycleAnalysis>& cycles, std::ostream& os);
    void GenerateJsonReport(const std::vector<CycleAnalysis>& cycles, std::ostream& os);
    void GenerateHtmlReport(const std::vector<CycleAnalysis>& cycles, std::ostream& os);
    
    // 辅助方法
    std::string FormatCyclePath(const std::vector<std::string>& cycle) const;
    std::string EscapeJsonString(const std::string& str) const;
    std::string GetCurrentTimestamp() const;
};
