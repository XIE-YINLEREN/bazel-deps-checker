#include "BazelAnalyzerSDK.h"

#include "analysis/BuildTimeAnalyzer.h"
#include "analysis/CycleDetector.h"
#include "graph/DependencyGraph.h"
#include "output/OutputReport.h"
#include "parser/AdvancedBazelQueryParser.h"

#include <filesystem>
#include <chrono>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <utility>

namespace {

struct DependencyAnalysisContext {
    std::unordered_map<std::string, BazelTarget> targets;
    std::shared_ptr<DependencyGraph> dependency_graph;
    std::shared_ptr<CycleDetector> cycle_detector;
};

struct CachedDependencyContext {
    std::shared_ptr<DependencyAnalysisContext> context;
    std::string fingerprint;
};

std::mutex& GetDependencyContextMutex() {
    static std::mutex mutex;
    return mutex;
}

std::unordered_map<std::string, CachedDependencyContext>& GetDependencyContextCache() {
    static std::unordered_map<std::string, CachedDependencyContext> cache;
    return cache;
}

std::string BuildDependencyContextKey(const CommandLineArgs& args) {
    return args.workspace_path + '\n' + args.bazel_binary;
}

std::string BuildWorkspaceFingerprint(const std::string& workspace_path) {
    namespace fs = std::filesystem;

    fs::path workspace(workspace_path);
    if (!fs::exists(workspace) || !fs::is_directory(workspace)) {
        return "missing";
    }

    uintmax_t file_count = 0;
    long long latest_write = 0;
    auto update_from_path = [&](const fs::path& path) {
        std::error_code ec;
        const auto time = fs::last_write_time(path, ec);
        if (ec) {
            return;
        }
        const auto raw = time.time_since_epoch().count();
        if (raw > latest_write) {
            latest_write = raw;
        }
        ++file_count;
    };

    for (const auto& name : {"WORKSPACE", "WORKSPACE.bazel", "MODULE.bazel"}) {
        fs::path path = workspace / name;
        if (fs::exists(path)) {
            update_from_path(path);
        }
    }

    std::error_code ec;
    for (fs::recursive_directory_iterator it(workspace, ec), end; it != end; it.increment(ec)) {
        if (ec) {
            break;
        }
        if (!it->is_regular_file()) {
            continue;
        }

        const std::string filename = it->path().filename().string();
        if (filename == "BUILD" || filename == "BUILD.bazel") {
            update_from_path(it->path());
        }
    }

    return std::to_string(file_count) + ":" + std::to_string(latest_write);
}

}  // namespace

class BazelAnalyzerSDK::Impl {
public:
    explicit Impl(const CommandLineArgs& args) {
        report_ = std::make_unique<OutputReport>();
        report_->SetOutputPath(args.output_path);
    }

    void analyzeUnusedDependencies(const CommandLineArgs& args) {
        EnsureDependencyAnalysisReady(args);
        auto unused_deps = cycle_detector_->AnalyzeUnusedDependencies();
        report_->GenerateUnusedDependenciesReport(unused_deps, args.output_format);
    }

    std::string renderUnusedDependencies(const CommandLineArgs& args, OutputFormat format) {
        ResetPerformance();
        const auto total_start = std::chrono::steady_clock::now();
        EnsureDependencyAnalysisReady(args);
        const auto analysis_start = std::chrono::steady_clock::now();
        auto unused_deps = cycle_detector_->AnalyzeUnusedDependencies();
        const auto render_start = std::chrono::steady_clock::now();
        const std::string rendered = report_->RenderUnusedDependenciesReport(unused_deps, format);
        FinalizePerformance(total_start, analysis_start, render_start);
        return rendered;
    }

    std::pair<std::string, std::string> renderUnusedDependenciesJsonAndHtml(
        const CommandLineArgs& args) {
        ResetPerformance();
        const auto total_start = std::chrono::steady_clock::now();
        EnsureDependencyAnalysisReady(args);
        const auto analysis_start = std::chrono::steady_clock::now();
        auto unused_deps = cycle_detector_->AnalyzeUnusedDependencies();
        const auto render_start = std::chrono::steady_clock::now();
        auto reports = std::make_pair(
            report_->RenderUnusedDependenciesReport(unused_deps, OutputFormat::JSON),
            report_->RenderUnusedDependenciesReport(unused_deps, OutputFormat::HTML));
        FinalizePerformance(total_start, analysis_start, render_start);
        return reports;
    }

    void analyzeCycles(const CommandLineArgs& args) {
        EnsureDependencyAnalysisReady(args);
        auto cycles = cycle_detector_->AnalyzeCycles();
        report_->GenerateCycleReport(cycles, args.output_format);
    }

    std::string renderCycles(const CommandLineArgs& args, OutputFormat format) {
        ResetPerformance();
        const auto total_start = std::chrono::steady_clock::now();
        EnsureDependencyAnalysisReady(args);
        const auto analysis_start = std::chrono::steady_clock::now();
        auto cycles = cycle_detector_->AnalyzeCycles();
        const auto render_start = std::chrono::steady_clock::now();
        const std::string rendered = report_->RenderCycleReport(cycles, format);
        FinalizePerformance(total_start, analysis_start, render_start);
        return rendered;
    }

    std::pair<std::string, std::string> renderCyclesJsonAndHtml(const CommandLineArgs& args) {
        ResetPerformance();
        const auto total_start = std::chrono::steady_clock::now();
        EnsureDependencyAnalysisReady(args);
        const auto analysis_start = std::chrono::steady_clock::now();
        auto cycles = cycle_detector_->AnalyzeCycles();
        const auto render_start = std::chrono::steady_clock::now();
        auto reports = std::make_pair(
            report_->RenderCycleReport(cycles, OutputFormat::JSON),
            report_->RenderCycleReport(cycles, OutputFormat::HTML));
        FinalizePerformance(total_start, analysis_start, render_start);
        return reports;
    }

    void analyzeBuildTime(const CommandLineArgs& args) {
        if (!build_time_analyzer_) {
            build_time_analyzer_ = std::make_unique<bazel_analyzer::BuildTimeAnalyzer>(
                args.workspace_path, args.bazel_binary);
            build_time_analyzer_->SetBuildTargets({"//..."});
        }

        const bazel_analyzer::AnalysisResult result = build_time_analyzer_->RunFullAnalysis();
        report_->GenerateBuildTimeReport(result, args.output_format);

        if (!result.success) {
            throw std::runtime_error(result.error_message.empty()
                                         ? "Build time analysis failed"
                                         : result.error_message);
        }
    }

    std::string renderBuildTime(const CommandLineArgs& args, OutputFormat format) {
        ResetPerformance();
        const auto total_start = std::chrono::steady_clock::now();
        if (!build_time_analyzer_) {
            build_time_analyzer_ = std::make_unique<bazel_analyzer::BuildTimeAnalyzer>(
                args.workspace_path, args.bazel_binary);
            build_time_analyzer_->SetBuildTargets({"//..."});
        }

        const auto analysis_start = std::chrono::steady_clock::now();
        const bazel_analyzer::AnalysisResult result = build_time_analyzer_->RunFullAnalysis();
        const auto render_start = std::chrono::steady_clock::now();
        const std::string report = report_->RenderBuildTimeReport(result, format);
        FinalizePerformance(total_start, analysis_start, render_start);

        if (!result.success) {
            throw std::runtime_error(result.error_message.empty()
                                         ? "Build time analysis failed"
                                         : result.error_message);
        }

        return report;
    }

    std::pair<std::string, std::string> renderBuildTimeJsonAndHtml(const CommandLineArgs& args) {
        ResetPerformance();
        const auto total_start = std::chrono::steady_clock::now();
        if (!build_time_analyzer_) {
            build_time_analyzer_ = std::make_unique<bazel_analyzer::BuildTimeAnalyzer>(
                args.workspace_path, args.bazel_binary);
            build_time_analyzer_->SetBuildTargets({"//..."});
        }

        const auto analysis_start = std::chrono::steady_clock::now();
        const bazel_analyzer::AnalysisResult result = build_time_analyzer_->RunFullAnalysis();
        const auto render_start = std::chrono::steady_clock::now();
        const auto reports = std::make_pair(
            report_->RenderBuildTimeReport(result, OutputFormat::JSON),
            report_->RenderBuildTimeReport(result, OutputFormat::HTML));
        FinalizePerformance(total_start, analysis_start, render_start);

        if (!result.success) {
            throw std::runtime_error(result.error_message.empty()
                                         ? "Build time analysis failed"
                                         : result.error_message);
        }

        return reports;
    }

    BazelAnalyzerSDK::DualDependencyReports renderDependencyJsonAndHtml(
        const CommandLineArgs& args) {
        ResetPerformance();
        const auto total_start = std::chrono::steady_clock::now();
        EnsureDependencyAnalysisReady(args);
        const auto analysis_start = std::chrono::steady_clock::now();

        auto cycles = cycle_detector_->AnalyzeCycles();
        auto unused_deps = cycle_detector_->AnalyzeUnusedDependencies();

        const auto render_start = std::chrono::steady_clock::now();
        BazelAnalyzerSDK::DualDependencyReports reports;
        reports.cycle = {report_->RenderCycleReport(cycles, OutputFormat::JSON),
                         report_->RenderCycleReport(cycles, OutputFormat::HTML)};
        reports.unused = {report_->RenderUnusedDependenciesReport(unused_deps, OutputFormat::JSON),
                          report_->RenderUnusedDependenciesReport(unused_deps, OutputFormat::HTML)};
        FinalizePerformance(total_start, analysis_start, render_start);
        return reports;
    }

    BazelAnalyzerSDK::PerformanceInfo getLastPerformanceInfo() const {
        return last_performance_;
    }

private:
    static double ToMillis(std::chrono::steady_clock::duration duration) {
        return std::chrono::duration<double, std::milli>(duration).count();
    }

    void ResetPerformance() {
        last_performance_ = BazelAnalyzerSDK::PerformanceInfo{};
    }

    void FinalizePerformance(
        std::chrono::steady_clock::time_point total_start,
        std::chrono::steady_clock::time_point analysis_start,
        std::chrono::steady_clock::time_point render_start) {
        const auto end = std::chrono::steady_clock::now();
        last_performance_.analysis_ms = ToMillis(render_start - analysis_start);
        last_performance_.report_render_ms = ToMillis(end - render_start);
        last_performance_.total_ms = ToMillis(end - total_start);
    }

    void EnsureDependencyAnalysisReady(const CommandLineArgs& args) {
        if (dependency_context_) {
            last_performance_.reused_dependency_context = true;
            return;
        }

        const auto start = std::chrono::steady_clock::now();
        const std::string cache_key = BuildDependencyContextKey(args);
        const std::string fingerprint = BuildWorkspaceFingerprint(args.workspace_path);
        {
            std::lock_guard<std::mutex> lock(GetDependencyContextMutex());
            auto& cache = GetDependencyContextCache();
            auto it = cache.find(cache_key);
            if (it != cache.end()) {
                if (it->second.fingerprint == fingerprint && it->second.context) {
                    dependency_context_ = it->second.context;
                    last_performance_.reused_dependency_context = true;
                } else {
                    cache.erase(it);
                }
            }
        }

        if (!dependency_context_) {
            parser_ = std::make_unique<AdvancedBazelQueryParser>(args.workspace_path, args.bazel_binary);
            auto context = std::make_shared<DependencyAnalysisContext>();
            context->targets = parser_->ParseWorkspace();
            context->dependency_graph = std::make_shared<DependencyGraph>(context->targets);
            context->cycle_detector = std::make_shared<CycleDetector>(
                *context->dependency_graph, context->targets, args.workspace_path);
            dependency_context_ = std::move(context);

            std::lock_guard<std::mutex> lock(GetDependencyContextMutex());
            GetDependencyContextCache()[cache_key] =
                CachedDependencyContext{dependency_context_, fingerprint};
        }

        targets_ = &dependency_context_->targets;
        dependency_graph_ = dependency_context_->dependency_graph;
        cycle_detector_ = dependency_context_->cycle_detector;
        last_performance_.dependency_prepare_ms = ToMillis(std::chrono::steady_clock::now() - start);
    }

    std::unique_ptr<AdvancedBazelQueryParser> parser_;
    std::shared_ptr<DependencyAnalysisContext> dependency_context_;
    std::shared_ptr<DependencyGraph> dependency_graph_;
    std::shared_ptr<CycleDetector> cycle_detector_;
    std::unique_ptr<OutputReport> report_;
    std::unique_ptr<bazel_analyzer::BuildTimeAnalyzer> build_time_analyzer_;
    const std::unordered_map<std::string, BazelTarget>* targets_{nullptr};
    BazelAnalyzerSDK::PerformanceInfo last_performance_{};
};

BazelAnalyzerSDK::BazelAnalyzerSDK(CommandLineArgs args)
    : args_(std::move(args)), impl_(std::make_unique<Impl>(args_)) {
}

BazelAnalyzerSDK::~BazelAnalyzerSDK() = default;

void BazelAnalyzerSDK::ClearDependencyContextCache() {
    std::lock_guard<std::mutex> lock(GetDependencyContextMutex());
    GetDependencyContextCache().clear();
}

size_t BazelAnalyzerSDK::GetDependencyContextCacheSize() {
    std::lock_guard<std::mutex> lock(GetDependencyContextMutex());
    return GetDependencyContextCache().size();
}

void BazelAnalyzerSDK::executeCommand() {
    switch (args_.execute_function) {
        case ExcuteFuction::UNUSED_DEPENDENCY_CHECK:
            impl_->analyzeUnusedDependencies(args_);
            break;
        case ExcuteFuction::CYCLIC_DEPENDENCY_DETECTION:
            impl_->analyzeCycles(args_);
            break;
        case ExcuteFuction::BUILD_TIME_ANALYZE:
            impl_->analyzeBuildTime(args_);
            break;
    }
}

std::string BazelAnalyzerSDK::renderReport(OutputFormat format) {
    switch (args_.execute_function) {
        case ExcuteFuction::UNUSED_DEPENDENCY_CHECK:
            return impl_->renderUnusedDependencies(args_, format);
        case ExcuteFuction::CYCLIC_DEPENDENCY_DETECTION:
            return impl_->renderCycles(args_, format);
        case ExcuteFuction::BUILD_TIME_ANALYZE:
            return impl_->renderBuildTime(args_, format);
    }

    throw std::runtime_error("Unsupported execute function");
}

std::pair<std::string, std::string> BazelAnalyzerSDK::renderJsonAndHtmlReports() {
    switch (args_.execute_function) {
        case ExcuteFuction::UNUSED_DEPENDENCY_CHECK:
            return impl_->renderUnusedDependenciesJsonAndHtml(args_);
        case ExcuteFuction::CYCLIC_DEPENDENCY_DETECTION:
            return impl_->renderCyclesJsonAndHtml(args_);
        case ExcuteFuction::BUILD_TIME_ANALYZE:
            return impl_->renderBuildTimeJsonAndHtml(args_);
    }

    throw std::runtime_error("Unsupported execute function");
}

BazelAnalyzerSDK::DualDependencyReports BazelAnalyzerSDK::renderDependencyJsonAndHtmlReports() {
    return impl_->renderDependencyJsonAndHtml(args_);
}

BazelAnalyzerSDK::PerformanceInfo BazelAnalyzerSDK::getLastPerformanceInfo() const {
    return impl_->getLastPerformanceInfo();
}
