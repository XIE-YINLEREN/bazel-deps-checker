#include "BazelAnalyzerSDK.h"
#include "analysis/CycleDetector.h"
#include "output/OutputReport.h"
#include "parser/AdvancedBazelQueryParser.h"
#include <memory>

CommandLineArgs* BazelAnalyzerSDK::args = nullptr;

class BazelAnalyzerSDK::Impl {
public:
    explicit Impl(int argc, char* argv[]) {
        parser = new AdvancedBazelQueryParser(args->workspace_path);
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

private:
    AdvancedBazelQueryParser* parser;
    DependencyGraph* dependencyer;
    CycleDetector* detector;
    OutputReport* report;
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
}