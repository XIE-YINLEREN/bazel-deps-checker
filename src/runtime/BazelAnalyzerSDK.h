#pragma once

#include "cli/CommandLine.h"

#include <memory>
#include <utility>
#include <string>

class BazelAnalyzerSDK {
public:
    struct PerformanceInfo {
        double dependency_prepare_ms{0.0};
        double analysis_ms{0.0};
        double report_render_ms{0.0};
        double total_ms{0.0};
        bool reused_dependency_context{false};
    };

    struct DualDependencyReports {
        std::pair<std::string, std::string> cycle;
        std::pair<std::string, std::string> unused;
    };

    explicit BazelAnalyzerSDK(CommandLineArgs args);
    ~BazelAnalyzerSDK();

    static void ClearDependencyContextCache();
    static size_t GetDependencyContextCacheSize();

    void executeCommand();
    std::string renderReport(OutputFormat format);
    std::pair<std::string, std::string> renderJsonAndHtmlReports();
    DualDependencyReports renderDependencyJsonAndHtmlReports();
    PerformanceInfo getLastPerformanceInfo() const;

private:
    class Impl;

    CommandLineArgs args_;
    std::unique_ptr<Impl> impl_;
};
