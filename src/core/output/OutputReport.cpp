#include "OutputReport.h"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

namespace {

template <typename Writer>
void WriteToConfiguredOutput(const std::string& output_path, Writer&& writer) {
    if (output_path.empty()) {
        writer(std::cout);
        return;
    }

    std::ofstream file(output_path);
    if (file.is_open()) {
        writer(file);
        return;
    }

    std::cerr << "Error: Cannot open output file: " << output_path << std::endl;
    writer(std::cout);
}

using GroupedDependencies = std::map<std::string, std::vector<const RemovableDependency*>>;

GroupedDependencies GroupDependencies(const std::vector<RemovableDependency>& unused_dependencies) {
    GroupedDependencies grouped;
    for (const auto& dep : unused_dependencies) {
        grouped[dep.from_target].push_back(&dep);
    }
    return grouped;
}

struct ConfidenceSummary {
    int high{0};
    int medium{0};
    int low{0};
};

ConfidenceSummary SummarizeConfidence(const std::vector<RemovableDependency>& unused_dependencies) {
    ConfidenceSummary summary;
    for (const auto& dep : unused_dependencies) {
        switch (dep.confidence) {
            case ConfidenceLevel::HIGH:
                ++summary.high;
                break;
            case ConfidenceLevel::MEDIUM:
                ++summary.medium;
                break;
            case ConfidenceLevel::LOW:
                ++summary.low;
                break;
        }
    }
    return summary;
}

double ToSeconds(std::chrono::microseconds duration) {
    return static_cast<double>(duration.count()) / 1000000.0;
}

std::string CycleTypeToString(CycleType type) {
    std::ostringstream os;
    os << type;
    return os.str();
}

std::vector<std::pair<std::string, std::string>> CollectArtifacts(
    const bazel_analyzer::AnalysisResult& result) {
    std::vector<std::pair<std::string, std::string>> artifacts;
    if (!result.report_path.empty()) {
        artifacts.emplace_back("report", result.report_path);
    }
    if (!result.csv_path.empty()) {
        artifacts.emplace_back("csv", result.csv_path);
    }
    if (!result.json_path.empty()) {
        artifacts.emplace_back("json", result.json_path);
    }
    if (!result.dot_path.empty()) {
        artifacts.emplace_back("dot", result.dot_path);
    }
    return artifacts;
}

}  // namespace

std::string OutputReport::RenderCycleReport(
    const std::vector<CycleAnalysis>& cycles,
    const OutputFormat& format) const {
    std::ostringstream os;
    GenerateCycleReport(cycles, format, os);
    return os.str();
}

std::string OutputReport::RenderUnusedDependenciesReport(
    const std::vector<RemovableDependency>& unused_dependencies,
    const OutputFormat& format) const {
    std::ostringstream os;
    GenerateUnusedDependenciesReport(unused_dependencies, format, os);
    return os.str();
}

std::string OutputReport::RenderBuildTimeReport(
    const bazel_analyzer::AnalysisResult& result,
    const OutputFormat& format) const {
    std::ostringstream os;
    GenerateBuildTimeReport(result, format, os);
    return os.str();
}

void OutputReport::GenerateCycleReport(
    const std::vector<CycleAnalysis>& cycles,
    const OutputFormat& format) const {
    WriteToConfiguredOutput(output_path_, [this, &cycles, &format](std::ostream& os) {
        GenerateCycleReport(cycles, format, os);
    });
}

void OutputReport::GenerateUnusedDependenciesReport(
    const std::vector<RemovableDependency>& unused_dependencies,
    const OutputFormat& format) const {
    WriteToConfiguredOutput(output_path_, [this, &unused_dependencies, &format](std::ostream& os) {
        GenerateUnusedDependenciesReport(unused_dependencies, format, os);
    });
}

void OutputReport::GenerateBuildTimeReport(
    const bazel_analyzer::AnalysisResult& result,
    const OutputFormat& format) const {
    WriteToConfiguredOutput(output_path_, [this, &result, &format](std::ostream& os) {
        GenerateBuildTimeReport(result, format, os);
    });
}

void OutputReport::GenerateCycleReport(
    const std::vector<CycleAnalysis>& cycles,
    const OutputFormat& format,
    std::ostream& output_stream) const {
    switch (format) {
        case OutputFormat::CONSOLE:
            GenerateCycleConsoleReport(cycles, output_stream);
            break;
        case OutputFormat::MARKDOWN:
            GenerateCycleMarkdownReport(cycles, output_stream);
            break;
        case OutputFormat::JSON:
            GenerateCycleJsonReport(cycles, output_stream);
            break;
        case OutputFormat::HTML:
            GenerateCycleHtmlReport(cycles, output_stream);
            break;
    }
}

void OutputReport::GenerateUnusedDependenciesReport(
    const std::vector<RemovableDependency>& unused_dependencies,
    const OutputFormat& format,
    std::ostream& output_stream) const {
    switch (format) {
        case OutputFormat::CONSOLE:
            GenerateUnusedDependenciesConsoleReport(unused_dependencies, output_stream);
            break;
        case OutputFormat::MARKDOWN:
            GenerateUnusedDependenciesMarkdownReport(unused_dependencies, output_stream);
            break;
        case OutputFormat::JSON:
            GenerateUnusedDependenciesJsonReport(unused_dependencies, output_stream);
            break;
        case OutputFormat::HTML:
            GenerateUnusedDependenciesHtmlReport(unused_dependencies, output_stream);
            break;
    }
}

void OutputReport::GenerateBuildTimeReport(
    const bazel_analyzer::AnalysisResult& result,
    const OutputFormat& format,
    std::ostream& output_stream) const {
    switch (format) {
        case OutputFormat::CONSOLE:
            GenerateBuildTimeConsoleReport(result, output_stream);
            break;
        case OutputFormat::MARKDOWN:
            GenerateBuildTimeMarkdownReport(result, output_stream);
            break;
        case OutputFormat::JSON:
            GenerateBuildTimeJsonReport(result, output_stream);
            break;
        case OutputFormat::HTML:
            GenerateBuildTimeHtmlReport(result, output_stream);
            break;
    }
}

void OutputReport::GenerateUnusedDependenciesConsoleReport(
    const std::vector<RemovableDependency>& unused_dependencies,
    std::ostream& os) const {
    if (unused_dependencies.empty()) {
        os << "✓ 未发现可移除的依赖\n";
        return;
    }

    const auto grouped_deps = GroupDependencies(unused_dependencies);
    const auto summary = SummarizeConfidence(unused_dependencies);

    os << "========================================\n";
    os << "   未使用依赖分析报告\n";
    os << "   生成时间: " << GetCurrentTimestamp() << "\n";
    os << "   发现未使用依赖数量: " << unused_dependencies.size() << "\n";
    os << "========================================\n\n";

    for (const auto& [from_target, deps] : grouped_deps) {
        os << "目标: " << from_target << "\n";
        os << "├─ 可移除依赖数量: " << deps.size() << "\n";
        os << "├─ 可移除依赖列表:\n";

        for (size_t index = 0; index < deps.size(); ++index) {
            const auto& dep = *deps[index];
            os << "   " << (index + 1) << ". " << dep.to_target;
            if (!dep.reason.empty()) {
                os << " (" << dep.reason << ")";
            }
            os << " [置信度: " << ConfidenceLevelToString(dep.confidence) << "]\n";
        }
        os << "\n";
    }

    os << "========================================\n";
    os << "统计信息:\n";
    os << "- 高置信度依赖: " << summary.high << " 个\n";
    os << "- 中置信度依赖: " << summary.medium << " 个\n";
    os << "- 低置信度依赖: " << summary.low << " 个\n\n";
    os << "操作建议:\n";
    os << "1. 高置信度依赖可以安全移除\n";
    os << "2. 中置信度依赖建议进一步验证\n";
    os << "3. 低置信度依赖需要谨慎处理\n";
    os << "========================================\n";
}

void OutputReport::GenerateUnusedDependenciesMarkdownReport(
    const std::vector<RemovableDependency>& unused_dependencies,
    std::ostream& os) const {
    os << "# 未使用依赖分析报告\n\n";
    os << "- **生成时间**: " << GetCurrentTimestamp() << "\n";
    os << "- **发现未使用依赖数量**: " << unused_dependencies.size() << "\n\n";

    if (unused_dependencies.empty()) {
        os << "✓ 未发现可移除的依赖\n";
        return;
    }

    const auto grouped_deps = GroupDependencies(unused_dependencies);
    const auto summary = SummarizeConfidence(unused_dependencies);

    os << "## 依赖详情\n\n";
    for (const auto& [from_target, deps] : grouped_deps) {
        os << "### " << from_target << "\n\n";
        os << "**可移除依赖数量**: " << deps.size() << "\n\n";
        os << "| 依赖目标 | 移除原因 | 置信度 |\n";
        os << "|----------|----------|--------|\n";

        for (const auto* dep : deps) {
            os << "| " << dep->to_target << " | " << dep->reason << " | "
               << ConfidenceLevelToString(dep->confidence) << " |\n";
        }
        os << "\n";
    }

    os << "## 统计信息\n\n";
    os << "- **高置信度依赖**: " << summary.high << " 个\n";
    os << "- **中置信度依赖**: " << summary.medium << " 个\n";
    os << "- **低置信度依赖**: " << summary.low << " 个\n\n";
    os << "## 操作建议\n\n";
    os << "1. **高置信度依赖**：可以安全移除，移除后应进行编译测试\n";
    os << "2. **中置信度依赖**：建议进一步验证，检查是否存在间接依赖关系\n";
    os << "3. **低置信度依赖**：需要谨慎处理，可能需要深入分析源代码\n";
}

void OutputReport::GenerateUnusedDependenciesJsonReport(
    const std::vector<RemovableDependency>& unused_dependencies,
    std::ostream& os) const {
    const auto grouped_deps = GroupDependencies(unused_dependencies);
    const auto summary = SummarizeConfidence(unused_dependencies);

    os << "{\n";
    os << "  \"unused_dependencies_report\": {\n";
    os << "    \"timestamp\": \"" << EscapeJsonString(GetCurrentTimestamp()) << "\",\n";
    os << "    \"total_unused_dependencies\": " << unused_dependencies.size() << ",\n";
    os << "    \"statistics\": {\n";
    os << "      \"high_confidence\": " << summary.high << ",\n";
    os << "      \"medium_confidence\": " << summary.medium << ",\n";
    os << "      \"low_confidence\": " << summary.low << "\n";
    os << "    },\n";
    os << "    \"grouped_dependencies\": [\n";

    bool first_group = true;
    for (const auto& [from_target, deps] : grouped_deps) {
        if (!first_group) {
            os << ",\n";
        }
        first_group = false;

        os << "      {\n";
        os << "        \"from_target\": \"" << EscapeJsonString(from_target) << "\",\n";
        os << "        \"count\": " << deps.size() << ",\n";
        os << "        \"dependencies\": [\n";

        for (size_t index = 0; index < deps.size(); ++index) {
            const auto& dep = *deps[index];
            if (index > 0) {
                os << ",\n";
            }
            os << "          {\n";
            os << "            \"to_target\": \"" << EscapeJsonString(dep.to_target) << "\",\n";
            os << "            \"reason\": \"" << EscapeJsonString(dep.reason) << "\",\n";
            os << "            \"confidence\": \"" << ConfidenceLevelToString(dep.confidence) << "\"\n";
            os << "          }";
        }

        os << "\n        ]\n";
        os << "      }";
    }

    os << "\n    ]\n";
    os << "  }\n";
    os << "}\n";
}

void OutputReport::GenerateUnusedDependenciesHtmlReport(
    const std::vector<RemovableDependency>& unused_dependencies,
    std::ostream& os) const {
    const auto grouped_deps = GroupDependencies(unused_dependencies);
    const auto summary = SummarizeConfidence(unused_dependencies);

    WriteHtmlDocumentStart(os, "未使用依赖分析报告");
    WriteHtmlHeader(os,
                    "未使用依赖分析报告",
                    {{"生成时间", GetCurrentTimestamp()},
                     {"发现未使用依赖数量", std::to_string(unused_dependencies.size())}});

    os << "  <section class=\"panel\">\n";
    os << "    <div class=\"panel-header\">\n";
    os << "      <h2>统计概览</h2>\n";
    os << "      <p>快速查看不同置信度下的可移除依赖数量。</p>\n";
    os << "    </div>\n";
    os << "    <div class=\"metric-grid\">\n";
    WriteHtmlMetricCard(os, "高置信度", std::to_string(summary.high), "success");
    WriteHtmlMetricCard(os, "中置信度", std::to_string(summary.medium), "warning");
    WriteHtmlMetricCard(os, "低置信度", std::to_string(summary.low), "danger");
    WriteHtmlMetricCard(os, "目标分组数", std::to_string(grouped_deps.size()));
    os << "    </div>\n";
    os << "  </section>\n";

    if (unused_dependencies.empty()) {
        os << "  <section class=\"panel empty-state\">\n";
        os << "    <h2>没有发现可移除依赖</h2>\n";
        os << "    <p>当前工作区中没有命中未使用依赖规则。</p>\n";
        os << "  </section>\n";
        WriteHtmlDocumentEnd(os);
        return;
    }

    os << "  <section class=\"panel\">\n";
    os << "    <div class=\"panel-header panel-header-inline\">\n";
    os << "      <div>\n";
    os << "        <h2>依赖详情</h2>\n";
    os << "        <p>支持按目标名或依赖名搜索，点击分组可展开详细条目。</p>\n";
    os << "      </div>\n";
    os << "      <label class=\"search-box\">\n";
    os << "        <span>搜索</span>\n";
    os << "        <input id=\"unused-search\" type=\"search\" placeholder=\"输入 target 或 dependency\" oninput=\"filterUnusedGroups()\">\n";
    os << "      </label>\n";
    os << "    </div>\n";

    for (const auto& [from_target, deps] : grouped_deps) {
        os << "    <details class=\"group-card unused-group\" data-search=\""
           << EscapeHtmlString(from_target);
        for (const auto* dep : deps) {
            os << " " << EscapeHtmlString(dep->to_target) << " " << EscapeHtmlString(dep->reason);
        }
        os << "\" open>\n";
        os << "      <summary>\n";
        os << "        <div>\n";
        os << "          <strong>" << EscapeHtmlString(from_target) << "</strong>\n";
        os << "          <span class=\"muted\">" << deps.size() << " 个可移除依赖</span>\n";
        os << "        </div>\n";
        os << "        <span class=\"chip\">Target</span>\n";
        os << "      </summary>\n";
        os << "      <div class=\"stack-list\">\n";

        for (const auto* dep : deps) {
            std::string tone = "default";
            switch (dep->confidence) {
                case ConfidenceLevel::HIGH:
                    tone = "success";
                    break;
                case ConfidenceLevel::MEDIUM:
                    tone = "warning";
                    break;
                case ConfidenceLevel::LOW:
                    tone = "danger";
                    break;
            }

            os << "        <article class=\"item-card tone-" << tone << "\">\n";
            os << "          <div class=\"item-main\">\n";
            os << "            <h3>" << EscapeHtmlString(dep->to_target) << "</h3>\n";
            os << "            <p>" << EscapeHtmlString(dep->reason) << "</p>\n";
            os << "          </div>\n";
            os << "          <div class=\"item-side\">\n";
            os << "            <span class=\"chip chip-" << tone << "\">"
               << EscapeHtmlString(ConfidenceLevelToString(dep->confidence)) << "</span>\n";
            os << "          </div>\n";
            os << "        </article>\n";
        }

        os << "      </div>\n";
        os << "    </details>\n";
    }
    os << "  </section>\n";

    os << "  <script>\n";
    os << "    function filterUnusedGroups() {\n";
    os << "      const query = document.getElementById('unused-search').value.toLowerCase();\n";
    os << "      document.querySelectorAll('.unused-group').forEach((group) => {\n";
    os << "        const haystack = (group.dataset.search || '').toLowerCase();\n";
    os << "        group.style.display = !query || haystack.includes(query) ? '' : 'none';\n";
    os << "      });\n";
    os << "    }\n";
    os << "  </script>\n";
    WriteHtmlDocumentEnd(os);
}

void OutputReport::GenerateCycleConsoleReport(
    const std::vector<CycleAnalysis>& cycles,
    std::ostream& os) const {
    if (cycles.empty()) {
        os << "未发现循环依赖\n";
        return;
    }

    os << "========================================\n";
    os << "   循环依赖分析报告\n";
    os << "   生成时间: " << GetCurrentTimestamp() << "\n";
    os << "   发现循环数量: " << cycles.size() << "\n";
    os << "========================================\n\n";

    for (size_t index = 0; index < cycles.size(); ++index) {
        const auto& analysis = cycles[index];

        os << "循环 #" << (index + 1) << ":\n";
        os << "├─ 类型: " << analysis.cycle_type << "\n";
        os << "├─ 路径: " << FormatCyclePath(analysis.cycle) << "\n";
        os << "├─ 长度: " << analysis.cycle.size() << " 个目标\n";

        if (!analysis.removable_dependencies.empty()) {
            os << "├─ 可安全移除的依赖:\n";
            for (size_t dep_index = 0; dep_index < analysis.removable_dependencies.size(); ++dep_index) {
                const auto& dep = analysis.removable_dependencies[dep_index];
                os << "   " << (dep_index + 1) << ". " << dep.from_target << " → " << dep.to_target;
                if (!dep.reason.empty()) {
                    os << " (" << dep.reason << ")";
                }
                os << "\n";
            }
        }

        if (include_suggestions_ && !analysis.suggested_fixes.empty()) {
            os << "└─ 修复建议:\n";
            for (size_t fix_index = 0; fix_index < analysis.suggested_fixes.size(); ++fix_index) {
                os << "   " << (fix_index + 1) << ". " << analysis.suggested_fixes[fix_index] << "\n";
            }
        } else {
            os << "└─ 无修复建议\n";
        }

        os << "\n";
    }
}

void OutputReport::GenerateCycleMarkdownReport(
    const std::vector<CycleAnalysis>& cycles,
    std::ostream& os) const {
    os << "# 循环依赖分析报告\n\n";
    os << "- **生成时间**: " << GetCurrentTimestamp() << "\n";
    os << "- **发现循环数量**: " << cycles.size() << "\n\n";

    if (cycles.empty()) {
        os << "未发现循环依赖\n";
        return;
    }

    os << "## 循环详情\n\n";
    for (size_t index = 0; index < cycles.size(); ++index) {
        const auto& analysis = cycles[index];
        os << "### 循环 #" << (index + 1) << "\n\n";
        os << "- **类型**: `" << analysis.cycle_type << "`\n";
        os << "- **路径**: `" << FormatCyclePath(analysis.cycle) << "`\n";
        os << "- **长度**: " << analysis.cycle.size() << " 个目标\n";

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
}

void OutputReport::GenerateCycleJsonReport(
    const std::vector<CycleAnalysis>& cycles,
    std::ostream& os) const {
    os << "{\n";
    os << "  \"report\": {\n";
    os << "    \"timestamp\": \"" << EscapeJsonString(GetCurrentTimestamp()) << "\",\n";
    os << "    \"total_cycles\": " << cycles.size() << ",\n";
    os << "    \"cycles\": [\n";

    for (size_t index = 0; index < cycles.size(); ++index) {
        const auto& analysis = cycles[index];
        os << "      {\n";
        os << "        \"id\": " << (index + 1) << ",\n";
        os << "        \"type\": \"" << CycleTypeToString(analysis.cycle_type) << "\",\n";
        os << "        \"length\": " << analysis.cycle.size() << ",\n";
        os << "        \"path\": [\n";

        for (size_t path_index = 0; path_index < analysis.cycle.size(); ++path_index) {
            os << "          \"" << EscapeJsonString(analysis.cycle[path_index]) << "\"";
            if (path_index + 1 < analysis.cycle.size()) {
                os << ",";
            }
            os << "\n";
        }
        os << "        ]";

        if (!analysis.removable_dependencies.empty()) {
            os << ",\n        \"removable_dependencies\": [\n";
            for (size_t dep_index = 0; dep_index < analysis.removable_dependencies.size(); ++dep_index) {
                const auto& dep = analysis.removable_dependencies[dep_index];
                os << "          {\n";
                os << "            \"from\": \"" << EscapeJsonString(dep.from_target) << "\",\n";
                os << "            \"to\": \"" << EscapeJsonString(dep.to_target) << "\",\n";
                os << "            \"reason\": \"" << EscapeJsonString(dep.reason) << "\",\n";
                os << "            \"confidence\": \"" << ConfidenceLevelToString(dep.confidence) << "\"\n";
                os << "          }";
                if (dep_index + 1 < analysis.removable_dependencies.size()) {
                    os << ",";
                }
                os << "\n";
            }
            os << "        ]";
        }

        if (include_suggestions_ && !analysis.suggested_fixes.empty()) {
            os << ",\n        \"suggestions\": [\n";
            for (size_t fix_index = 0; fix_index < analysis.suggested_fixes.size(); ++fix_index) {
                os << "          \"" << EscapeJsonString(analysis.suggested_fixes[fix_index]) << "\"";
                if (fix_index + 1 < analysis.suggested_fixes.size()) {
                    os << ",";
                }
                os << "\n";
            }
            os << "        ]\n";
        } else {
            os << "\n";
        }

        os << "      }";
        if (index + 1 < cycles.size()) {
            os << ",";
        }
        os << "\n";
    }

    os << "    ]\n";
    os << "  }\n";
    os << "}\n";
}

void OutputReport::GenerateCycleHtmlReport(
    const std::vector<CycleAnalysis>& cycles,
    std::ostream& os) const {
    size_t total_removable = 0;
    for (const auto& cycle : cycles) {
        total_removable += cycle.removable_dependencies.size();
    }

    WriteHtmlDocumentStart(os, "循环依赖分析报告");
    WriteHtmlHeader(os,
                    "循环依赖分析报告",
                    {{"生成时间", GetCurrentTimestamp()},
                     {"发现循环数量", std::to_string(cycles.size())}});

    os << "  <section class=\"panel\">\n";
    os << "    <div class=\"panel-header\">\n";
    os << "      <h2>概览</h2>\n";
    os << "      <p>从循环数量、可删除边数和复杂度角度快速评估问题规模。</p>\n";
    os << "    </div>\n";
    os << "    <div class=\"metric-grid\">\n";
    WriteHtmlMetricCard(os, "循环数量", std::to_string(cycles.size()), "danger");
    WriteHtmlMetricCard(os, "可移除依赖", std::to_string(total_removable), "success");
    WriteHtmlMetricCard(os, "最长循环", cycles.empty() ? "0" : std::to_string(cycles.front().cycle.size()));
    os << "    </div>\n";
    os << "  </section>\n";

    if (cycles.empty()) {
        os << "  <section class=\"panel empty-state\">\n";
        os << "    <h2>没有发现循环依赖</h2>\n";
        os << "    <p>当前工作区未检测到循环链路。</p>\n";
        os << "  </section>\n";
        WriteHtmlDocumentEnd(os);
        return;
    }

    os << "  <section class=\"panel\">\n";
    os << "    <div class=\"panel-header panel-header-inline\">\n";
    os << "      <div>\n";
    os << "        <h2>循环详情</h2>\n";
    os << "        <p>按循环类型、路径和修复建议浏览具体问题。</p>\n";
    os << "      </div>\n";
    os << "      <label class=\"search-box\">\n";
    os << "        <span>搜索</span>\n";
    os << "        <input id=\"cycle-search\" type=\"search\" placeholder=\"输入 target / type / suggestion\" oninput=\"filterCycles()\">\n";
    os << "      </label>\n";
    os << "    </div>\n";

    for (size_t index = 0; index < cycles.size(); ++index) {
        const auto& analysis = cycles[index];
        const std::string cycle_type = CycleTypeToString(analysis.cycle_type);
        const std::string path = FormatCyclePath(analysis.cycle);

        os << "    <details class=\"group-card cycle-card\" data-search=\""
           << EscapeHtmlString(cycle_type) << " " << EscapeHtmlString(path);
        for (const auto& fix : analysis.suggested_fixes) {
            os << " " << EscapeHtmlString(fix);
        }
        os << "\" open>\n";
        os << "      <summary>\n";
        os << "        <div>\n";
        os << "          <strong>循环 #" << (index + 1) << "</strong>\n";
        os << "          <span class=\"muted\">" << EscapeHtmlString(cycle_type) << " · "
           << analysis.cycle.size() << " 个目标</span>\n";
        os << "        </div>\n";
        os << "        <span class=\"chip chip-danger\">Cycle</span>\n";
        os << "      </summary>\n";
        os << "      <div class=\"stack-list\">\n";
        os << "        <article class=\"item-card\">\n";
        os << "          <div class=\"item-main\">\n";
        os << "            <h3>依赖路径</h3>\n";
        os << "            <p class=\"path-block\">" << EscapeHtmlString(path) << "</p>\n";
        os << "          </div>\n";
        os << "        </article>\n";

        if (!analysis.removable_dependencies.empty()) {
            os << "        <article class=\"item-card tone-success\">\n";
            os << "          <div class=\"item-main\">\n";
            os << "            <h3>可安全移除的依赖</h3>\n";
            os << "            <div class=\"pill-list\">\n";
            for (const auto& dep : analysis.removable_dependencies) {
                os << "              <span class=\"pill\">"
                   << EscapeHtmlString(dep.from_target + " → " + dep.to_target);
                if (!dep.reason.empty()) {
                    os << " · " << EscapeHtmlString(dep.reason);
                }
                os << "</span>\n";
            }
            os << "            </div>\n";
            os << "          </div>\n";
            os << "        </article>\n";
        }

        if (include_suggestions_ && !analysis.suggested_fixes.empty()) {
            os << "        <article class=\"item-card tone-warning\">\n";
            os << "          <div class=\"item-main\">\n";
            os << "            <h3>修复建议</h3>\n";
            os << "            <ul class=\"bullet-list\">\n";
            for (const auto& fix : analysis.suggested_fixes) {
                os << "              <li>" << EscapeHtmlString(fix) << "</li>\n";
            }
            os << "            </ul>\n";
            os << "          </div>\n";
            os << "        </article>\n";
        }

        os << "      </div>\n";
        os << "    </details>\n";
    }
    os << "  </section>\n";

    os << "  <script>\n";
    os << "    function filterCycles() {\n";
    os << "      const query = document.getElementById('cycle-search').value.toLowerCase();\n";
    os << "      document.querySelectorAll('.cycle-card').forEach((card) => {\n";
    os << "        const haystack = (card.dataset.search || '').toLowerCase();\n";
    os << "        card.style.display = !query || haystack.includes(query) ? '' : 'none';\n";
    os << "      });\n";
    os << "    }\n";
    os << "  </script>\n";
    WriteHtmlDocumentEnd(os);
}

void OutputReport::GenerateBuildTimeConsoleReport(
    const bazel_analyzer::AnalysisResult& result,
    std::ostream& os) const {
    os << "========================================\n";
    os << "   构建耗时分析报告\n";
    os << "   生成时间: " << GetCurrentTimestamp() << "\n";
    os << "   状态: " << (result.success ? "成功" : "失败") << "\n";
    os << "========================================\n\n";

    if (!result.success) {
        os << "错误信息: " << result.error_message << "\n";
        return;
    }

    os << "摘要:\n";
    os << "- 总构建耗时: " << FormatDuration(result.stats.total_duration) << "\n";
    os << "- Profile 生成耗时: " << FormatDuration(result.generation_time) << "\n";
    os << "- 分析耗时: " << FormatDuration(result.analysis_time) << "\n\n";

    os << "阶段统计:\n";
    os << "- loading: " << FormatDuration(result.stats.loading_duration) << "\n";
    os << "- analysis: " << FormatDuration(result.stats.analysis_duration) << "\n";
    os << "- execution: " << FormatDuration(result.stats.execution_duration) << "\n";
    os << "- vfs: " << FormatDuration(result.stats.vfs_duration) << "\n";
    os << "- other: " << FormatDuration(result.stats.other_duration) << "\n\n";

    os << "关键路径:\n";
    if (result.critical_paths.empty()) {
        os << "- 无关键路径数据\n";
    } else {
        for (size_t index = 0; index < result.critical_paths.size(); ++index) {
            const auto& path = result.critical_paths[index];
            os << (index + 1) << ". " << path.event_name << " ["
               << (path.rule_type.empty() ? "unknown" : path.rule_type) << "] - "
               << FormatDuration(path.cumulative_duration) << "\n";
            if (!path.path.empty()) {
                os << "   路径: ";
                for (size_t path_index = 0; path_index < path.path.size(); ++path_index) {
                    os << path.path[path_index];
                    if (path_index + 1 < path.path.size()) {
                        os << " -> ";
                    }
                }
                os << "\n";
            }
        }
    }
    os << "\n";

    os << "优化建议:\n";
    if (result.suggestions.empty()) {
        os << "- 无优化建议\n";
    } else {
        for (size_t index = 0; index < result.suggestions.size(); ++index) {
            const auto& suggestion = result.suggestions[index];
            os << (index + 1) << ". [" << SeverityToString(suggestion.severity) << "] "
               << suggestion.issue << "\n";
            if (!suggestion.suggestion.empty()) {
                os << "   建议: " << suggestion.suggestion << "\n";
            }
            if (!suggestion.affected_targets.empty()) {
                os << "   影响目标: ";
                for (size_t target_index = 0; target_index < suggestion.affected_targets.size(); ++target_index) {
                    os << suggestion.affected_targets[target_index];
                    if (target_index + 1 < suggestion.affected_targets.size()) {
                        os << ", ";
                    }
                }
                os << "\n";
            }
        }
    }

    const auto artifacts = CollectArtifacts(result);
    if (!artifacts.empty()) {
        os << "\n导出文件:\n";
        for (const auto& [name, path] : artifacts) {
            os << "- " << name << ": " << path << "\n";
        }
    }
}

void OutputReport::GenerateBuildTimeMarkdownReport(
    const bazel_analyzer::AnalysisResult& result,
    std::ostream& os) const {
    os << "# 构建耗时分析报告\n\n";
    os << "- **生成时间**: " << GetCurrentTimestamp() << "\n";
    os << "- **状态**: " << (result.success ? "成功" : "失败") << "\n\n";

    if (!result.success) {
        os << "## 错误信息\n\n";
        os << result.error_message << "\n";
        return;
    }

    os << "## 摘要\n\n";
    os << "- **总构建耗时**: " << FormatDuration(result.stats.total_duration) << "\n";
    os << "- **Profile 生成耗时**: " << FormatDuration(result.generation_time) << "\n";
    os << "- **分析耗时**: " << FormatDuration(result.analysis_time) << "\n\n";

    os << "## 阶段统计\n\n";
    os << "| 阶段 | 耗时 |\n";
    os << "|------|------|\n";
    os << "| loading | " << FormatDuration(result.stats.loading_duration) << " |\n";
    os << "| analysis | " << FormatDuration(result.stats.analysis_duration) << " |\n";
    os << "| execution | " << FormatDuration(result.stats.execution_duration) << " |\n";
    os << "| vfs | " << FormatDuration(result.stats.vfs_duration) << " |\n";
    os << "| other | " << FormatDuration(result.stats.other_duration) << " |\n\n";

    os << "## 关键路径\n\n";
    if (result.critical_paths.empty()) {
        os << "- 无关键路径数据\n";
    } else {
        for (const auto& path : result.critical_paths) {
            os << "- **" << path.event_name << "** ("
               << (path.rule_type.empty() ? "unknown" : path.rule_type) << ") - "
               << FormatDuration(path.cumulative_duration) << "\n";
            if (!path.path.empty()) {
                os << "  - 路径: `";
                for (size_t path_index = 0; path_index < path.path.size(); ++path_index) {
                    os << path.path[path_index];
                    if (path_index + 1 < path.path.size()) {
                        os << " -> ";
                    }
                }
                os << "`\n";
            }
        }
    }
    os << "\n";

    os << "## 优化建议\n\n";
    if (result.suggestions.empty()) {
        os << "- 无优化建议\n";
    } else {
        for (const auto& suggestion : result.suggestions) {
            os << "- **[" << SeverityToString(suggestion.severity) << "] " << suggestion.issue << "**\n";
            if (!suggestion.suggestion.empty()) {
                os << "  - 建议: " << suggestion.suggestion << "\n";
            }
            if (!suggestion.affected_targets.empty()) {
                os << "  - 影响目标: ";
                for (size_t target_index = 0; target_index < suggestion.affected_targets.size(); ++target_index) {
                    os << "`" << suggestion.affected_targets[target_index] << "`";
                    if (target_index + 1 < suggestion.affected_targets.size()) {
                        os << ", ";
                    }
                }
                os << "\n";
            }
        }
    }

    const auto artifacts = CollectArtifacts(result);
    if (!artifacts.empty()) {
        os << "\n## 导出文件\n\n";
        for (const auto& [name, path] : artifacts) {
            os << "- **" << name << "**: `" << path << "`\n";
        }
    }
}

void OutputReport::GenerateBuildTimeJsonReport(
    const bazel_analyzer::AnalysisResult& result,
    std::ostream& os) const {
    const auto artifacts = CollectArtifacts(result);

    os << "{\n";
    os << "  \"build_time_report\": {\n";
    os << "    \"timestamp\": \"" << EscapeJsonString(GetCurrentTimestamp()) << "\",\n";
    os << "    \"success\": " << (result.success ? "true" : "false") << ",\n";
    os << "    \"error_message\": \"" << EscapeJsonString(result.error_message) << "\"";

    if (!result.success) {
        os << "\n  }\n";
        os << "}\n";
        return;
    }

    os << ",\n    \"summary\": {\n";
    os << "      \"total_duration_seconds\": " << ToSeconds(result.stats.total_duration) << ",\n";
    os << "      \"generation_time_seconds\": " << ToSeconds(result.generation_time) << ",\n";
    os << "      \"analysis_time_seconds\": " << ToSeconds(result.analysis_time) << "\n";
    os << "    },\n";
    os << "    \"phase_stats\": {\n";
    os << "      \"loading_seconds\": " << ToSeconds(result.stats.loading_duration) << ",\n";
    os << "      \"analysis_seconds\": " << ToSeconds(result.stats.analysis_duration) << ",\n";
    os << "      \"execution_seconds\": " << ToSeconds(result.stats.execution_duration) << ",\n";
    os << "      \"vfs_seconds\": " << ToSeconds(result.stats.vfs_duration) << ",\n";
    os << "      \"other_seconds\": " << ToSeconds(result.stats.other_duration) << "\n";
    os << "    },\n";
    os << "    \"critical_paths\": [\n";

    for (size_t index = 0; index < result.critical_paths.size(); ++index) {
        const auto& path = result.critical_paths[index];
        os << "      {\n";
        os << "        \"event_name\": \"" << EscapeJsonString(path.event_name) << "\",\n";
        os << "        \"rule_type\": \"" << EscapeJsonString(path.rule_type) << "\",\n";
        os << "        \"cumulative_duration_seconds\": " << ToSeconds(path.cumulative_duration) << ",\n";
        os << "        \"path\": [";
        for (size_t path_index = 0; path_index < path.path.size(); ++path_index) {
            if (path_index > 0) {
                os << ", ";
            }
            os << "\"" << EscapeJsonString(path.path[path_index]) << "\"";
        }
        os << "]\n";
        os << "      }";
        if (index + 1 < result.critical_paths.size()) {
            os << ",";
        }
        os << "\n";
    }

    os << "    ],\n";
    os << "    \"suggestions\": [\n";
    for (size_t index = 0; index < result.suggestions.size(); ++index) {
        const auto& suggestion = result.suggestions[index];
        os << "      {\n";
        os << "        \"issue\": \"" << EscapeJsonString(suggestion.issue) << "\",\n";
        os << "        \"suggestion\": \"" << EscapeJsonString(suggestion.suggestion) << "\",\n";
        os << "        \"severity\": \"" << SeverityToString(suggestion.severity) << "\",\n";
        os << "        \"estimated_improvement\": " << suggestion.estimated_improvement << ",\n";
        os << "        \"affected_targets\": [";
        for (size_t target_index = 0; target_index < suggestion.affected_targets.size(); ++target_index) {
            if (target_index > 0) {
                os << ", ";
            }
            os << "\"" << EscapeJsonString(suggestion.affected_targets[target_index]) << "\"";
        }
        os << "]\n";
        os << "      }";
        if (index + 1 < result.suggestions.size()) {
            os << ",";
        }
        os << "\n";
    }
    os << "    ],\n";
    os << "    \"artifacts\": {";
    for (size_t index = 0; index < artifacts.size(); ++index) {
        const auto& [name, path] = artifacts[index];
        if (index > 0) {
            os << ", ";
        }
        os << "\"" << EscapeJsonString(name) << "\": \"" << EscapeJsonString(path) << "\"";
    }
    os << "}\n";
    os << "  }\n";
    os << "}\n";
}

void OutputReport::GenerateBuildTimeHtmlReport(
    const bazel_analyzer::AnalysisResult& result,
    std::ostream& os) const {
    const auto artifacts = CollectArtifacts(result);

    WriteHtmlDocumentStart(os, "构建耗时分析报告");
    WriteHtmlHeader(os,
                    "构建耗时分析报告",
                    {{"生成时间", GetCurrentTimestamp()},
                     {"状态", result.success ? "成功" : "失败"}});

    if (!result.success) {
        os << "  <section class=\"panel empty-state\">\n";
        os << "    <h2>分析失败</h2>\n";
        os << "    <p>" << EscapeHtmlString(result.error_message) << "</p>\n";
        os << "  </section>\n";
        WriteHtmlDocumentEnd(os);
        return;
    }

    os << "  <section class=\"panel\">\n";
    os << "    <div class=\"panel-header\">\n";
    os << "      <h2>摘要</h2>\n";
    os << "      <p>用卡片快速浏览构建总耗时与分析阶段表现。</p>\n";
    os << "    </div>\n";
    os << "    <div class=\"metric-grid\">\n";
    WriteHtmlMetricCard(os, "总构建耗时", FormatDuration(result.stats.total_duration), "danger");
    WriteHtmlMetricCard(os, "Profile 生成", FormatDuration(result.generation_time));
    WriteHtmlMetricCard(os, "分析耗时", FormatDuration(result.analysis_time));
    WriteHtmlMetricCard(os, "关键路径数", std::to_string(result.critical_paths.size()));
    os << "    </div>\n";
    os << "  </section>\n";

    os << "  <section class=\"panel\">\n";
    os << "    <div class=\"panel-header\">\n";
    os << "      <h2>阶段统计</h2>\n";
    os << "      <p>展示 loading / analysis / execution / vfs / other 五类阶段耗时。</p>\n";
    os << "    </div>\n";
    os << "    <div class=\"metric-grid\">\n";
    WriteHtmlMetricCard(os, "loading", FormatDuration(result.stats.loading_duration));
    WriteHtmlMetricCard(os, "analysis", FormatDuration(result.stats.analysis_duration));
    WriteHtmlMetricCard(os, "execution", FormatDuration(result.stats.execution_duration), "warning");
    WriteHtmlMetricCard(os, "vfs", FormatDuration(result.stats.vfs_duration));
    WriteHtmlMetricCard(os, "other", FormatDuration(result.stats.other_duration));
    os << "    </div>\n";
    os << "  </section>\n";

    os << "  <section class=\"panel two-column-layout\">\n";
    os << "    <div>\n";
    os << "      <div class=\"panel-header panel-header-inline\">\n";
    os << "        <div>\n";
    os << "          <h2>关键路径</h2>\n";
    os << "          <p>定位最慢链路，帮助识别拖慢构建的核心目标。</p>\n";
    os << "        </div>\n";
    os << "      </div>\n";
    if (result.critical_paths.empty()) {
        os << "      <div class=\"empty-mini\">无关键路径数据</div>\n";
    } else {
        os << "      <div class=\"stack-list\">\n";
        for (const auto& path : result.critical_paths) {
            os << "        <article class=\"item-card tone-warning\">\n";
            os << "          <div class=\"item-main\">\n";
            os << "            <h3>" << EscapeHtmlString(path.event_name) << "</h3>\n";
            os << "            <p>规则类型: "
               << EscapeHtmlString(path.rule_type.empty() ? "unknown" : path.rule_type)
               << " · 耗时: " << EscapeHtmlString(FormatDuration(path.cumulative_duration)) << "</p>\n";
            if (!path.path.empty()) {
                os << "            <p class=\"path-block\">";
                for (size_t path_index = 0; path_index < path.path.size(); ++path_index) {
                    os << EscapeHtmlString(path.path[path_index]);
                    if (path_index + 1 < path.path.size()) {
                        os << " → ";
                    }
                }
                os << "</p>\n";
            }
            os << "          </div>\n";
            os << "        </article>\n";
        }
        os << "      </div>\n";
    }
    os << "    </div>\n";

    os << "    <div>\n";
    os << "      <div class=\"panel-header panel-header-inline\">\n";
    os << "        <div>\n";
    os << "          <h2>优化建议</h2>\n";
    os << "          <p>按严重程度浏览建议，并查看受影响目标。</p>\n";
    os << "        </div>\n";
    os << "      </div>\n";
    if (result.suggestions.empty()) {
        os << "      <div class=\"empty-mini\">无优化建议</div>\n";
    } else {
        os << "      <div class=\"stack-list\">\n";
        for (const auto& suggestion : result.suggestions) {
            std::string tone = "default";
            if (suggestion.severity == bazel_analyzer::OptimizationSuggestion::Severity::HIGH) {
                tone = "danger";
            } else if (suggestion.severity == bazel_analyzer::OptimizationSuggestion::Severity::MEDIUM) {
                tone = "warning";
            }

            os << "        <article class=\"item-card tone-" << tone << "\">\n";
            os << "          <div class=\"item-main\">\n";
            os << "            <h3>" << EscapeHtmlString(suggestion.issue) << "</h3>\n";
            if (!suggestion.suggestion.empty()) {
                os << "            <p>" << EscapeHtmlString(suggestion.suggestion) << "</p>\n";
            }
            if (!suggestion.affected_targets.empty()) {
                os << "            <div class=\"pill-list\">\n";
                for (const auto& target : suggestion.affected_targets) {
                    os << "              <span class=\"pill\">" << EscapeHtmlString(target) << "</span>\n";
                }
                os << "            </div>\n";
            }
            os << "          </div>\n";
            os << "          <div class=\"item-side\">\n";
            os << "            <span class=\"chip chip-" << tone << "\">"
               << EscapeHtmlString(SeverityToString(suggestion.severity)) << "</span>\n";
            os << "          </div>\n";
            os << "        </article>\n";
        }
        os << "      </div>\n";
    }
    os << "    </div>\n";
    os << "  </section>\n";

    if (!artifacts.empty()) {
        os << "  <section class=\"panel\">\n";
        os << "    <div class=\"panel-header\">\n";
        os << "      <h2>导出文件</h2>\n";
        os << "      <p>展示分析过程额外生成的报告与中间产物。</p>\n";
        os << "    </div>\n";
        os << "    <div class=\"stack-list\">\n";
        for (const auto& [name, path] : artifacts) {
            os << "      <article class=\"item-card\">\n";
            os << "        <div class=\"item-main\">\n";
            os << "          <h3>" << EscapeHtmlString(name) << "</h3>\n";
            os << "          <p class=\"path-block\">" << EscapeHtmlString(path) << "</p>\n";
            os << "        </div>\n";
            os << "        <div class=\"item-side\"><span class=\"chip\">Artifact</span></div>\n";
            os << "      </article>\n";
        }
        os << "    </div>\n";
        os << "  </section>\n";
    }

    WriteHtmlDocumentEnd(os);
}

void OutputReport::WriteHtmlDocumentStart(std::ostream& os, const std::string& title) const {
    os << "<!DOCTYPE html>\n";
    os << "<html lang=\"zh-CN\">\n";
    os << "<head>\n";
    os << "  <meta charset=\"UTF-8\">\n";
    os << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    os << "  <title>" << EscapeHtmlString(title) << "</title>\n";
    os << "  <style>\n";
    os << "    :root { color-scheme: light dark; --bg:#0b1020; --panel:#121a2b; --panel-soft:#19233a; --text:#e7ecf7; --muted:#9fb0cf; --border:#2a3550; --accent:#6ea8fe; --success:#34d399; --warning:#fbbf24; --danger:#f87171; --shadow:0 18px 40px rgba(0,0,0,.24); }\n";
    os << "    * { box-sizing:border-box; }\n";
    os << "    body { margin:0; font-family:Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif; background:linear-gradient(180deg,#0b1020 0%,#101729 100%); color:var(--text); }\n";
    os << "    .page { max-width:1180px; margin:0 auto; padding:32px 20px 48px; }\n";
    os << "    .hero { padding:28px; border:1px solid rgba(255,255,255,.08); background:linear-gradient(135deg, rgba(110,168,254,.18), rgba(16,23,41,.88)); border-radius:24px; box-shadow:var(--shadow); margin-bottom:20px; }\n";
    os << "    .hero h1 { margin:0 0 8px; font-size:32px; line-height:1.2; }\n";
    os << "    .hero-meta { display:flex; flex-wrap:wrap; gap:12px; margin-top:16px; }\n";
    os << "    .hero-meta .chip { background:rgba(255,255,255,.06); }\n";
    os << "    .panel { background:rgba(18,26,43,.92); border:1px solid var(--border); border-radius:20px; padding:22px; box-shadow:var(--shadow); margin-bottom:18px; }\n";
    os << "    .panel-header { margin-bottom:16px; }\n";
    os << "    .panel-header h2 { margin:0 0 6px; font-size:22px; }\n";
    os << "    .panel-header p { margin:0; color:var(--muted); }\n";
    os << "    .panel-header-inline { display:flex; justify-content:space-between; align-items:flex-start; gap:16px; flex-wrap:wrap; }\n";
    os << "    .metric-grid { display:grid; grid-template-columns:repeat(auto-fit,minmax(180px,1fr)); gap:14px; }\n";
    os << "    .metric-card { background:var(--panel-soft); border:1px solid var(--border); border-radius:18px; padding:18px; min-height:120px; }\n";
    os << "    .metric-card .label { color:var(--muted); font-size:13px; letter-spacing:.04em; text-transform:uppercase; }\n";
    os << "    .metric-card .value { margin-top:10px; font-size:28px; font-weight:700; word-break:break-word; }\n";
    os << "    .metric-card.tone-success .value { color:var(--success); }\n";
    os << "    .metric-card.tone-warning .value { color:var(--warning); }\n";
    os << "    .metric-card.tone-danger .value { color:var(--danger); }\n";
    os << "    .group-card { border:1px solid var(--border); background:var(--panel-soft); border-radius:18px; margin-bottom:12px; overflow:hidden; }\n";
    os << "    .group-card summary { list-style:none; display:flex; justify-content:space-between; align-items:center; gap:12px; cursor:pointer; padding:18px 20px; }\n";
    os << "    .group-card summary::-webkit-details-marker { display:none; }\n";
    os << "    .stack-list { display:flex; flex-direction:column; gap:12px; padding:0 18px 18px; }\n";
    os << "    .item-card { display:flex; justify-content:space-between; gap:16px; align-items:flex-start; background:rgba(11,16,32,.55); border:1px solid rgba(255,255,255,.06); border-radius:16px; padding:16px; }\n";
    os << "    .item-card h3 { margin:0 0 8px; font-size:18px; }\n";
    os << "    .item-card p { margin:0; color:var(--muted); line-height:1.55; }\n";
    os << "    .item-card.tone-success { border-color:rgba(52,211,153,.28); }\n";
    os << "    .item-card.tone-warning { border-color:rgba(251,191,36,.28); }\n";
    os << "    .item-card.tone-danger { border-color:rgba(248,113,113,.28); }\n";
    os << "    .item-main { min-width:0; flex:1; }\n";
    os << "    .item-side { flex-shrink:0; }\n";
    os << "    .chip { display:inline-flex; align-items:center; gap:6px; border-radius:999px; padding:7px 12px; font-size:12px; font-weight:600; border:1px solid rgba(255,255,255,.08); color:var(--text); background:rgba(255,255,255,.04); }\n";
    os << "    .chip-success { color:var(--success); border-color:rgba(52,211,153,.3); }\n";
    os << "    .chip-warning { color:var(--warning); border-color:rgba(251,191,36,.3); }\n";
    os << "    .chip-danger { color:var(--danger); border-color:rgba(248,113,113,.3); }\n";
    os << "    .muted { color:var(--muted); display:block; margin-top:4px; }\n";
    os << "    .search-box { display:flex; flex-direction:column; gap:6px; color:var(--muted); min-width:260px; }\n";
    os << "    .search-box input { width:100%; background:#0b1020; color:var(--text); border:1px solid var(--border); border-radius:12px; padding:12px 14px; outline:none; }\n";
    os << "    .search-box input:focus { border-color:var(--accent); box-shadow:0 0 0 3px rgba(110,168,254,.18); }\n";
    os << "    .pill-list { display:flex; flex-wrap:wrap; gap:8px; margin-top:12px; }\n";
    os << "    .pill { display:inline-flex; padding:8px 12px; border-radius:999px; background:rgba(255,255,255,.06); color:var(--text); border:1px solid rgba(255,255,255,.08); font-size:13px; }\n";
    os << "    .bullet-list { margin:0; padding-left:18px; color:var(--muted); }\n";
    os << "    .bullet-list li { margin:6px 0; }\n";
    os << "    .path-block { margin-top:8px; padding:12px 14px; border-radius:12px; background:#0b1020; border:1px solid var(--border); color:#dbe7ff !important; font-family:ui-monospace,SFMono-Regular,Menlo,monospace; word-break:break-word; }\n";
    os << "    .two-column-layout { display:grid; grid-template-columns:repeat(auto-fit,minmax(320px,1fr)); gap:18px; }\n";
    os << "    .empty-state, .empty-mini { text-align:center; color:var(--muted); }\n";
    os << "    .empty-state h2 { color:var(--text); }\n";
    os << "    @media (max-width: 720px) { .page { padding:20px 14px 36px; } .hero h1 { font-size:26px; } .item-card { flex-direction:column; } }\n";
    os << "  </style>\n";
    os << "</head>\n";
    os << "<body>\n";
    os << "  <div class=\"page\">\n";
}

void OutputReport::WriteHtmlDocumentEnd(std::ostream& os) const {
    os << "  </div>\n";
    os << "</body>\n";
    os << "</html>\n";
}

void OutputReport::WriteHtmlHeader(
    std::ostream& os,
    const std::string& title,
    const std::vector<std::pair<std::string, std::string>>& meta_items) const {
    os << "  <section class=\"hero\">\n";
    os << "    <h1>" << EscapeHtmlString(title) << "</h1>\n";
    os << "    <p>静态 HTML 报告页，支持直接双击打开查看，不依赖额外前端运行环境。</p>\n";
    os << "    <div class=\"hero-meta\">\n";
    for (const auto& [label, value] : meta_items) {
        os << "      <span class=\"chip\"><strong>" << EscapeHtmlString(label) << ":</strong>&nbsp;"
           << EscapeHtmlString(value) << "</span>\n";
    }
    os << "    </div>\n";
    os << "  </section>\n";
}

void OutputReport::WriteHtmlMetricCard(
    std::ostream& os,
    const std::string& label,
    const std::string& value,
    const std::string& tone) const {
    os << "      <div class=\"metric-card";
    if (tone != "default") {
        os << " tone-" << tone;
    }
    os << "\">\n";
    os << "        <div class=\"label\">" << EscapeHtmlString(label) << "</div>\n";
    os << "        <div class=\"value\">" << EscapeHtmlString(value) << "</div>\n";
    os << "      </div>\n";
}

std::string OutputReport::FormatCyclePath(const std::vector<std::string>& cycle) const {
    std::ostringstream ss;
    for (size_t index = 0; index < cycle.size(); ++index) {
        ss << cycle[index];
        if (index + 1 < cycle.size()) {
            ss << " → ";
        }
    }
    return ss.str();
}

std::string OutputReport::FormatDuration(std::chrono::microseconds duration) const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3) << ToSeconds(duration) << "s";
    return ss.str();
}

std::string OutputReport::ConfidenceLevelToString(ConfidenceLevel level) const {
    switch (level) {
        case ConfidenceLevel::HIGH:
            return "高";
        case ConfidenceLevel::MEDIUM:
            return "中";
        case ConfidenceLevel::LOW:
            return "低";
    }

    return "未知";
}

std::string OutputReport::SeverityToString(
    bazel_analyzer::OptimizationSuggestion::Severity severity) const {
    switch (severity) {
        case bazel_analyzer::OptimizationSuggestion::Severity::HIGH:
            return "HIGH";
        case bazel_analyzer::OptimizationSuggestion::Severity::MEDIUM:
            return "MEDIUM";
        case bazel_analyzer::OptimizationSuggestion::Severity::LOW:
            return "LOW";
    }

    return "UNKNOWN";
}

std::string OutputReport::EscapeJsonString(const std::string& str) const {
    std::string result;
    result.reserve(str.size());

    for (char c : str) {
        switch (c) {
            case '"':
                result += "\\\"";
                break;
            case '\\':
                result += "\\\\";
                break;
            case '\b':
                result += "\\b";
                break;
            case '\f':
                result += "\\f";
                break;
            case '\n':
                result += "\\n";
                break;
            case '\r':
                result += "\\r";
                break;
            case '\t':
                result += "\\t";
                break;
            default:
                result += c;
                break;
        }
    }

    return result;
}

std::string OutputReport::EscapeHtmlString(const std::string& str) const {
    std::string result;
    result.reserve(str.size());

    for (char c : str) {
        switch (c) {
            case '&':
                result += "&amp;";
                break;
            case '<':
                result += "&lt;";
                break;
            case '>':
                result += "&gt;";
                break;
            case '"':
                result += "&quot;";
                break;
            case '\'':
                result += "&#39;";
                break;
            default:
                result += c;
                break;
        }
    }

    return result;
}

std::string OutputReport::GetCurrentTimestamp() const {
    std::time_t now = std::time(nullptr);
    std::tm local_time{};
#ifdef _WIN32
    localtime_s(&local_time, &now);
#else
    localtime_r(&now, &local_time);
#endif

    std::ostringstream ss;
    ss << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}
