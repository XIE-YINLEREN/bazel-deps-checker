#include "cli/CommandLine.h"
#include "log/logger.h"
#include "parser/BuildFileParser.h"
#include "dependency/BuildFileDependencyer.h"
// #include "analysis/CycleDetector.h"
// #include "output/OutputReport.h"

int main(int argc, char* argv[]) {

    CommandLineArgs* args = CommandLineArgs::GetInstance(argc, argv);
    
    if (args->workspace_root.empty()) {
        LOG_ERROR("Error: Please specify workspace root\n");
        return 1;
    }
    
    try {
        // 1. 解析BUILD文件
        BuildFileParser parser(args->workspace_root);
        auto targets = parser.ParseWorkspace();

        // 2. 构建依赖图
        BuildFileDependencyer graph(targets);
        
        // // 3. 分析问题
        // CycleDetector detector(graph);
        // auto cycles = detector.AnalyzeCycles();
        
        // // 4. 输出报告
        // OutputReport::GenerateReport(cycles, args->output_format);
        
    } catch (const std::exception& e) {
        std::string log_msg = e.what();
        LOG_ERROR(log_msg);
        return 1;
    }
    
    return 0;
}