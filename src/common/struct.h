#pragma once

#include <string>
#include <vector>


struct BazelTarget {
    std::string name;
    std::string path;
    std::string full_label;
    std::string rule_type;
    std::vector<std::string> deps;
    std::vector<std::string> srcs;
    std::vector<std::string> hdrs;
    bool empty() const { return name.empty(); }
};


enum class OutputFormat {
    CONSOLE,    // 控制台输出
    MARKDOWN,   // Markdown格式
    JSON,       // JSON格式
    HTML        // HTML格式
};


enum class ConfidenceLevel {
    HIGH,   // 明确的未使用依赖
    MEDIUM, // 可能未使用的依赖
    LOW     // 需要进一步验证的依赖
};