#include "OutputReport.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <iostream>

OutputReport OutputReport::getInstance() {
    static OutputReport instance;
    return instance;
}

void OutputReport::GenerateReport(const std::vector<CycleAnalysis>& cycles, const OutputFormat& format) {
    if (output_path_.empty()) {
        // 如果没有指定输出路径，输出到标准输出
        GenerateReport(cycles, format, std::cout);
    } else {
        // 输出到文件
        std::ofstream file(output_path_);
        if (file.is_open()) {
            GenerateReport(cycles, format, file);
            file.close();
        } else {
            std::cerr << "Error: Cannot open output file: " << output_path_ << std::endl;
            // 回退到标准输出
            GenerateReport(cycles, format, std::cout);
        }
    }
}

void OutputReport::GenerateReport(const std::vector<CycleAnalysis>& cycles, const OutputFormat& format, std::ostream& output_stream) {
    switch (format) {
        case OutputFormat::CONSOLE:
            GenerateConsoleReport(cycles, output_stream);
            break;
        case OutputFormat::MARKDOWN:
            GenerateMarkdownReport(cycles, output_stream);
            break;
        case OutputFormat::JSON:
            GenerateJsonReport(cycles, output_stream);
            break;
        case OutputFormat::HTML:
            GenerateHtmlReport(cycles, output_stream);
            break;
        default:
            GenerateConsoleReport(cycles, output_stream);
            break;
    }
}

void OutputReport::GenerateConsoleReport(const std::vector<CycleAnalysis>& cycles, std::ostream& os) {
    if (cycles.empty()) {
        os << "未发现循环依赖\n";
        return;
    }
    
    os << "========================================\n";
    os << "   循环依赖分析报告\n";
    os << "   生成时间: " << GetCurrentTimestamp() << "\n";
    os << "   发现循环数量: " << cycles.size() << "\n";
    os << "========================================\n\n";
    
    for (size_t i = 0; i < cycles.size(); ++i) {
        const auto& analysis = cycles[i];
        
        os << "循环 #" << (i + 1) << ":\n";
        os << "├─ 类型: " << analysis.cycle_type << "\n";
        os << "├─ 路径: " << FormatCyclePath(analysis.cycle) << "\n";
        os << "├─ 长度: " << analysis.cycle.size() << " 个目标\n";
        
        // 添加可移除依赖的输出
        if (!analysis.removable_dependencies.empty()) {
            os << "├─ 可安全移除的依赖:\n";
            for (size_t j = 0; j < analysis.removable_dependencies.size(); ++j) {
                const auto& dep = analysis.removable_dependencies[j];
                os << "   " << (j + 1) << ". " << dep.from_target << " → " << dep.to_target;
                if (!dep.reason.empty()) {
                    os << " (" << dep.reason << ")";
                }
                os << "\n";
            }
        }
        
        if (include_suggestions_ && !analysis.suggested_fixes.empty()) {
            os << "└─ 修复建议:\n";
            for (size_t j = 0; j < analysis.suggested_fixes.size(); ++j) {
                os << "   " << (j + 1) << ". " << analysis.suggested_fixes[j] << "\n";
            }
        } else {
            os << "└─ 无修复建议\n";
        }
        
        os << "\n";
        
        // 每5个循环后添加分隔符
        if ((i + 1) % 5 == 0 && i != cycles.size() - 1) {
            os << "---\n\n";
        }
    }
    
    os << "========================================\n";
    os << "总结:\n";
    
    // 添加关于可移除依赖的总结
    int total_removable = 0;
    for (const auto& cycle : cycles) {
        total_removable += cycle.removable_dependencies.size();
    }
    
    if (total_removable > 0) {
        os << "- 发现 " << total_removable << " 个可安全移除的依赖\n";
        os << "- 移除任一可安全依赖即可打破循环\n";
    }
    
    os << "- 建议优先处理小型循环（长度较短的）\n";
    os << "- 直接循环依赖通常更容易修复\n";
    os << "- 复杂循环可能需要重构模块结构\n";
    os << "========================================\n";
}

void OutputReport::GenerateMarkdownReport(const std::vector<CycleAnalysis>& cycles, std::ostream& os) {
    os << "# 循环依赖分析报告\n\n";
    os << "- **生成时间**: " << GetCurrentTimestamp() << "\n";
    os << "- **发现循环数量**: " << cycles.size() << "\n\n";
    
    if (cycles.empty()) {
        os << "未发现循环依赖\n";
        return;
    }
    
    os << "## 循环详情\n\n";
    
    for (size_t i = 0; i < cycles.size(); ++i) {
        const auto& analysis = cycles[i];
        
        os << "### 循环 #" << (i + 1) << "\n\n";
        os << "- **类型**: `" << analysis.cycle_type << "`\n";
        os << "- **路径**: `" << FormatCyclePath(analysis.cycle) << "`\n";
        os << "- **长度**: " << analysis.cycle.size() << " 个目标\n";
        
        // 添加可移除依赖
        if (!analysis.removable_dependencies.empty()) {
            os << "- **可安全移除的依赖**:\n";
            for (const auto& dep : analysis.removable_dependencies) {
                os << "  - `" << dep.from_target << "` → `" << dep.to_target << "`";
                if (!dep.reason.empty()) {
                    os << " (" << dep.reason << ")";
                }
                os << "\n";
            }
        }
        
        if (include_suggestions_ && !analysis.suggested_fixes.empty()) {
            os << "- **修复建议**:\n";
            for (const auto& fix : analysis.suggested_fixes) {
                os << "  - " << fix << "\n";
            }
        }
        
        os << "\n";
    }
    
    os << "## 处理优先级\n\n";
    
    // 按循环大小分组
    std::vector<const CycleAnalysis*> small_cycles;    // 2-3个节点
    std::vector<const CycleAnalysis*> medium_cycles;   // 4-5个节点  
    std::vector<const CycleAnalysis*> large_cycles;    // 6+个节点
    
    for (const auto& cycle : cycles) {
        if (cycle.cycle.size() <= 3) {
            small_cycles.push_back(&cycle);
        } else if (cycle.cycle.size() <= 5) {
            medium_cycles.push_back(&cycle);
        } else {
            large_cycles.push_back(&cycle);
        }
    }
    
    os << "| 优先级 | 循环大小 | 数量 | 建议 |\n";
    os << "|--------|----------|------|------|\n";
    os << "| 高 | 小型 (2-3个目标) | " << small_cycles.size() << " | 易于修复，建议优先处理 |\n";
    os << "| 中 | 中型 (4-5个目标) | " << medium_cycles.size() << " | 需要一些重构工作 |\n";
    os << "| 低 | 大型 (6+个目标) | " << large_cycles.size() << " | 可能涉及架构调整 |\n";
}

void OutputReport::GenerateJsonReport(const std::vector<CycleAnalysis>& cycles, std::ostream& os) {
    os << "{\n";
    os << "  \"report\": {\n";
    os << "    \"timestamp\": \"" << GetCurrentTimestamp() << "\",\n";
    os << "    \"total_cycles\": " << cycles.size() << ",\n";
    os << "    \"cycles\": [\n";
    
    for (size_t i = 0; i < cycles.size(); ++i) {
        const auto& analysis = cycles[i];
        
        os << "      {\n";
        os << "        \"id\": " << (i + 1) << ",\n";
        os << "        \"type\": \"" << analysis.cycle_type << "\",\n";
        os << "        \"length\": " << analysis.cycle.size() << ",\n";
        os << "        \"path\": [\n";
        
        for (size_t j = 0; j < analysis.cycle.size(); ++j) {
            os << "          \"" << EscapeJsonString(analysis.cycle[j]) << "\"";
            if (j < analysis.cycle.size() - 1) os << ",";
            os << "\n";
        }
        
        os << "        ]";
        
        // 添加可移除依赖
        if (!analysis.removable_dependencies.empty()) {
            os << ",\n        \"removable_dependencies\": [\n";
            for (size_t j = 0; j < analysis.removable_dependencies.size(); ++j) {
                const auto& dep = analysis.removable_dependencies[j];
                os << "          {\n";
                os << "            \"from\": \"" << EscapeJsonString(dep.from_target) << "\",\n";
                os << "            \"to\": \"" << EscapeJsonString(dep.to_target) << "\",\n";
                os << "            \"reason\": \"" << EscapeJsonString(dep.reason) << "\",\n";
                os << "            \"confidence\": \"" << ConfidenceLevelToString(dep.confidence) << "\"\n";
                os << "          }";
                if (j < analysis.removable_dependencies.size() - 1) os << ",";
                os << "\n";
            }
            os << "        ]";
        }
        
        if (include_suggestions_ && !analysis.suggested_fixes.empty()) {
            os << ",\n        \"suggestions\": [\n";
            for (size_t j = 0; j < analysis.suggested_fixes.size(); ++j) {
                os << "          \"" << EscapeJsonString(analysis.suggested_fixes[j]) << "\"";
                if (j < analysis.suggested_fixes.size() - 1) os << ",";
                os << "\n";
            }
            os << "        ]\n";
        } else {
            os << "\n";
        }
        
        os << "      }";
        if (i < cycles.size() - 1) os << ",";
        os << "\n";
    }
    
    os << "    ]\n";
    os << "  }\n";
    os << "}\n";
}

void OutputReport::GenerateHtmlReport(const std::vector<CycleAnalysis>& cycles, std::ostream& os) {
    os << "<!DOCTYPE html>\n";
    os << "<html lang=\"zh-CN\">\n";
    os << "<head>\n";
    os << "  <meta charset=\"UTF-8\">\n";
    os << "  <title>循环依赖分析报告</title>\n";
    os << "  <style>\n";
    os << "    body { font-family: Arial, sans-serif; margin: 20px; }\n";
    os << "    .header { background: #f5f5f5; padding: 20px; border-radius: 5px; }\n";
    os << "    .cycle { border: 1px solid #ddd; margin: 10px 0; padding: 15px; border-radius: 5px; }\n";
    os << "    .cycle.small { border-left: 4px solid #e74c3c; }\n";
    os << "    .cycle.medium { border-left: 4px solid #f39c12; }\n";
    os << "    .cycle.large { border-left: 4px solid #27ae60; }\n";
    os << "    .removable-dep { background: #e8f5e8; padding: 8px; margin: 5px 0; border-radius: 3px; border-left: 3px solid #2ecc71; }\n";
    os << "    .suggestion { background: #f8f9fa; padding: 8px; margin: 5px 0; border-radius: 3px; }\n";
    os << "    .path { font-family: monospace; background: #f1f1f1; padding: 5px; }\n";
    os << "  </style>\n";
    os << "</head>\n";
    os << "<body>\n";
    os << "  <div class=\"header\">\n";
    os << "    <h1>循环依赖分析报告</h1>\n";
    os << "    <p><strong>生成时间:</strong> " << GetCurrentTimestamp() << "</p>\n";
    os << "    <p><strong>发现循环数量:</strong> " << cycles.size() << "</p>\n";
    os << "  </div>\n";
    
    if (cycles.empty()) {
        os << "  <p>未发现循环依赖</p>\n";
    } else {
        for (size_t i = 0; i < cycles.size(); ++i) {
            const auto& analysis = cycles[i];
            std::string cycle_class = "cycle ";
            if (analysis.cycle.size() <= 3) cycle_class += "small";
            else if (analysis.cycle.size() <= 5) cycle_class += "medium";
            else cycle_class += "large";
            
            os << "  <div class=\"" << cycle_class << "\">\n";
            os << "    <h3>循环 #" << (i + 1) << " - " << analysis.cycle_type << "</h3>\n";
            os << "    <p><strong>路径:</strong> <span class=\"path\">" << FormatCyclePath(analysis.cycle) << "</span></p>\n";
            os << "    <p><strong>长度:</strong> " << analysis.cycle.size() << " 个目标</p>\n";
            
            // 添加可移除依赖
            if (!analysis.removable_dependencies.empty()) {
                os << "    <div>\n";
                os << "      <strong>可安全移除的依赖:</strong>\n";
                for (const auto& dep : analysis.removable_dependencies) {
                    os << "      <div class=\"removable-dep\">" << dep.from_target << " → " << dep.to_target;
                    if (!dep.reason.empty()) {
                        os << " (" << dep.reason << ")";
                    }
                    os << "</div>\n";
                }
                os << "    </div>\n";
            }
            
            if (include_suggestions_ && !analysis.suggested_fixes.empty()) {
                os << "    <div>\n";
                os << "      <strong>修复建议:</strong>\n";
                for (const auto& fix : analysis.suggested_fixes) {
                    os << "      <div class=\"suggestion\">" << fix << "</div>\n";
                }
                os << "    </div>\n";
            }
            
            os << "  </div>\n";
        }
    }
    
    os << "</body>\n";
    os << "</html>\n";
}

std::string OutputReport::FormatCyclePath(const std::vector<std::string>& cycle) const {
    std::ostringstream ss;
    for (size_t i = 0; i < cycle.size(); ++i) {
        ss << cycle[i];
        if (i < cycle.size() - 1) {
            ss << " → ";
        }
    }

    return ss.str();
}

std::string OutputReport::ConfidenceLevelToString(const ConfidenceLevel level) {
    switch (level) {
        case ConfidenceLevel::HIGH: return "HIGH";
        case ConfidenceLevel::MEDIUM: return "MEDIUM";
        case ConfidenceLevel::LOW: return "LOW";
        default: return "UNKNOWN";
    }
}

std::string OutputReport::EscapeJsonString(const std::string& str) const {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

std::string OutputReport::GetCurrentTimestamp() const {
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    
    std::ostringstream ss;
    ss << std::put_time(now, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}