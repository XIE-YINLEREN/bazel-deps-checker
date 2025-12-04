#include "CommandLine.h"


CommandLineArgs* CommandLineArgs::CommandLineArgsHandle = nullptr;

CommandLineArgs::CommandLineArgs(int argc, char* argv[]) {
    ParseCommandLine(argc, argv);
}

CommandLineArgs* CommandLineArgs::GetInstance(int argc, char* argv[]) {
    if (CommandLineArgsHandle == nullptr) {
        CommandLineArgsHandle = new CommandLineArgs(argc, argv);
    }
    return CommandLineArgsHandle;
}

void CommandLineArgs::ParseCommandLine(int argc, char* argv[]) {

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--workspace") == 0 || strcmp(argv[i], "-w") == 0) {
            if (i + 1 < argc) {
                workspace_path = argv[++i];
            }
        } else if (strcmp(argv[i], "--unused") == 0 || strcmp(argv[i], "-u") == 0) {
            execute_function = ExcuteFuction::UNUSED_DEPENDENCY_CHECK;
        } else if (strcmp(argv[i], "--output") == 0 || strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_path = argv[++i];
            }
        } else if (strcmp(argv[i], "--format") == 0 || strcmp(argv[i], "-f") == 0) {
            if (i + 1 < argc) {
                output_format = ParseOutputFormat(argv[++i]);
            }
        } else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "--include-tests") == 0 || strcmp(argv[i], "-t") == 0) {
            include_tests = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            PrintHelp();
            exit(0);
        } else {
            std::cerr << "Unknown option: " << argv[i] << std::endl;
            PrintHelp();
            exit(1);
        }
    }
    
    // 验证必需参数
    if (workspace_path.empty()) {
        std::cerr << "Error: Workspace path is required" << std::endl;
        PrintHelp();
        exit(1);
    }

    // 验证工作区路径是否存在
    if (!std::filesystem::exists(workspace_path) || !std::filesystem::is_directory(workspace_path)) {
        LOG_ERROR("Invalid workspace path: " + workspace_path);
        exit(1);
    }

    // 验证bazel工作区
    if ((!std::filesystem::exists(std::filesystem::path(workspace_path) / "WORKSPACE")) && 
          (!std::filesystem::exists(std::filesystem::path(workspace_path) / "WORKSPACE.bazel")) &&
          (!std::filesystem::exists(std::filesystem::path(workspace_path) / "MODULE.bazel"))) {
        LOG_ERROR("The specified path is not a valid Bazel workspace (missing WORKSPACE file): " + workspace_path);
        exit(1);
    }
}

OutputFormat CommandLineArgs::ParseOutputFormat(const std::string& format_str) const {
    if (format_str == "console" || format_str == "text") {
        return OutputFormat::CONSOLE;
    } else if (format_str == "markdown" || format_str == "md") {
        return OutputFormat::MARKDOWN;
    } else if (format_str == "json") {
        return OutputFormat::JSON;
    } else if (format_str == "html") {
        return OutputFormat::HTML;
    } else {
        LOG_WARN("Unknown output format '" + format_str + "', using console format");
        return OutputFormat::CONSOLE;
    }
}

void CommandLineArgs::PrintHelp() const {
    std::cout << "Bazel Dependency Analyzer\n";
    std::cout << "Default to using cyclic dependency detection\n";
    std::cout << "Usage: bazel-deps-analyzer [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -w, --workspace PATH    Bazel workspace path (required)\n";
    std::cout << "  -u, --unused            Analyzing unwanted dependencies\n";
    std::cout << "  -o, --output FILE       Output file path\n";
    std::cout << "  -f, --format FORMAT     Output format: console, markdown, json, html\n";
    std::cout << "  -v, --verbose           Enable verbose output\n";
    std::cout << "  -t, --include-tests     Include test targets in analysis\n";
    std::cout << "  -h, --help              Show this help message\n";
    std::cout << "\nExamples:\n";
    std::cout << "  bazel-deps-analyzer -w /path/to/workspace -f json\n";
    std::cout << "  bazel-deps-analyzer -w . -o report.md -f markdown -v\n";
}