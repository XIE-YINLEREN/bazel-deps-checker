#include "BazelAnalyzerSDK.h"
#include "analysis/CycleDetector.h"
#include "output/OutputReport.h"
#include "parser/AdvancedBazelQueryParser.h"
#include "analysis/BuildTimeAnalyzer.h"
#include "graph/DependencyGraph.h"
#include "analysis/BuildTimeAnalyzer.h"

#include <iterator>
#include <memory>

CommandLineArgs* BazelAnalyzerSDK::args = nullptr;

class BazelAnalyzerSDK::Impl {
public:
    explicit Impl(int argc, char* argv[]) {  
        parser = new AdvancedBazelQueryParser(args->workspace_path, args->bazel_binary);
        targets = parser->ParseWorkspace();
        dependencyer = new DependencyGraph(targets);
        detector = new CycleDetector(*dependencyer, targets);
        report = new OutputReport();
    }

    ~Impl() {
        delete parser;
        delete dependencyer;
        delete detector;
        delete report;
    }

    void analyzeUnusedDependencies() {
        auto unused_deps = detector->AnalyzeUnusedDependencies();
        report->GenerateUnusedDependenciesReport(unused_deps, args->output_format);
    }
    
    void analyzeCycles() {
        auto cycles = detector->AnalyzeCycles();
        report->GenerateCycleReport(cycles, args->output_format);
    }

    void analyzeBuildTime() {
        timer = new BuildTimeAnalyzer(args->bazel_binary, args->workspace_path);
        if (timer->createProfile("//...")) {
            // 生成报告
            std::string report = timer->generateBuildReport();
            std::cout << report << std::endl;
            
            // 或者获取JSON格式的分析结果
            auto analysis = timer->analyzeProfile();
            std::cout << analysis.dump(2) << std::endl;
        }
    }

private:
    AdvancedBazelQueryParser* parser;
    DependencyGraph* dependencyer;
    CycleDetector* detector;
    OutputReport* report;
    BuildTimeAnalyzer* timer;
    std::unordered_map<std::string, BazelTarget> targets;
};

BazelAnalyzerSDK::BazelAnalyzerSDK(int argc, char* argv[]) {
    args = CommandLineArgs::GetInstance(argc, argv);
    impl_ = std::make_unique<Impl>(argc, argv);
}

BazelAnalyzerSDK::~BazelAnalyzerSDK() {
    delete args;
    impl_.reset();
}

void BazelAnalyzerSDK::executeCommand() {
    if (args->execute_function == ExcuteFuction::UNUSED_DEPENDENCY_CHECK) {
        impl_->analyzeUnusedDependencies();
    }
    else if (args->execute_function == ExcuteFuction::CYCLIC_DEPENDENCY_DETECTION) {
        impl_->analyzeCycles();
    }
    else if (args->execute_function == ExcuteFuction::BUILD_TIME_ANALYZE) {
        impl_->analyzeBuildTime();
    }
}