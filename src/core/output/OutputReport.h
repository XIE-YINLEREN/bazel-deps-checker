#pragma once

#include <chrono>
#include <iosfwd>
#include <string>
#include <vector>

#include "analysis/BuildTimeAnalyzer.h"
#include "analysis/CycleDetector.h"
#include "struct.h"

class OutputReport {
public:
    void SetOutputPath(const std::string& path) { output_path_ = path; }
    void SetIncludeSuggestions(bool include) { include_suggestions_ = include; }

    std::string RenderCycleReport(
        const std::vector<CycleAnalysis>& cycles,
        const OutputFormat& format) const;
    std::string RenderUnusedDependenciesReport(
        const std::vector<RemovableDependency>& unused_dependencies,
        const OutputFormat& format) const;
    std::string RenderBuildTimeReport(
        const bazel_analyzer::AnalysisResult& result,
        const OutputFormat& format) const;

    void GenerateCycleReport(const std::vector<CycleAnalysis>& cycles, const OutputFormat& format) const;
    void GenerateUnusedDependenciesReport(
        const std::vector<RemovableDependency>& unused_dependencies,
        const OutputFormat& format) const;
    void GenerateBuildTimeReport(
        const bazel_analyzer::AnalysisResult& result,
        const OutputFormat& format) const;

private:
    void GenerateCycleReport(
        const std::vector<CycleAnalysis>& cycles,
        const OutputFormat& format,
        std::ostream& output_stream) const;
    void GenerateUnusedDependenciesReport(
        const std::vector<RemovableDependency>& unused_dependencies,
        const OutputFormat& format,
        std::ostream& output_stream) const;
    void GenerateBuildTimeReport(
        const bazel_analyzer::AnalysisResult& result,
        const OutputFormat& format,
        std::ostream& output_stream) const;

    void GenerateCycleConsoleReport(const std::vector<CycleAnalysis>& cycles, std::ostream& os) const;
    void GenerateCycleMarkdownReport(const std::vector<CycleAnalysis>& cycles, std::ostream& os) const;
    void GenerateCycleJsonReport(const std::vector<CycleAnalysis>& cycles, std::ostream& os) const;
    void GenerateCycleHtmlReport(const std::vector<CycleAnalysis>& cycles, std::ostream& os) const;

    void GenerateUnusedDependenciesConsoleReport(
        const std::vector<RemovableDependency>& unused_dependencies,
        std::ostream& os) const;
    void GenerateUnusedDependenciesMarkdownReport(
        const std::vector<RemovableDependency>& unused_dependencies,
        std::ostream& os) const;
    void GenerateUnusedDependenciesJsonReport(
        const std::vector<RemovableDependency>& unused_dependencies,
        std::ostream& os) const;
    void GenerateUnusedDependenciesHtmlReport(
        const std::vector<RemovableDependency>& unused_dependencies,
        std::ostream& os) const;

    void GenerateBuildTimeConsoleReport(
        const bazel_analyzer::AnalysisResult& result,
        std::ostream& os) const;
    void GenerateBuildTimeMarkdownReport(
        const bazel_analyzer::AnalysisResult& result,
        std::ostream& os) const;
    void GenerateBuildTimeJsonReport(
        const bazel_analyzer::AnalysisResult& result,
        std::ostream& os) const;
    void GenerateBuildTimeHtmlReport(
        const bazel_analyzer::AnalysisResult& result,
        std::ostream& os) const;

    std::string FormatCyclePath(const std::vector<std::string>& cycle) const;
    std::string FormatDuration(std::chrono::microseconds duration) const;
    std::string ConfidenceLevelToString(ConfidenceLevel level) const;
    std::string SeverityToString(bazel_analyzer::OptimizationSuggestion::Severity severity) const;
    std::string EscapeJsonString(const std::string& str) const;
    std::string EscapeHtmlString(const std::string& str) const;
    void WriteHtmlDocumentStart(std::ostream& os, const std::string& title) const;
    void WriteHtmlDocumentEnd(std::ostream& os) const;
    void WriteHtmlHeader(
        std::ostream& os,
        const std::string& title,
        const std::vector<std::pair<std::string, std::string>>& meta_items) const;
    void WriteHtmlMetricCard(
        std::ostream& os,
        const std::string& label,
        const std::string& value,
        const std::string& tone = "default") const;
    std::string GetCurrentTimestamp() const;

    std::string output_path_;
    bool include_suggestions_{true};
};
