#include "CommandLine.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace {

bool IsBazelWorkspace(const fs::path& workspace_path) {
    return fs::exists(workspace_path / "WORKSPACE") ||
           fs::exists(workspace_path / "WORKSPACE.bazel") ||
           fs::exists(workspace_path / "MODULE.bazel");
}

}  // namespace

CommandLineArgs CommandLineArgs::Parse(int argc, char* argv[]) {
    CommandLineArgs args;

    for (int index = 1; index < argc; ++index) {
        const std::string option = argv[index];

        if (option == "--workspace" || option == "-w") {
            args.workspace_path = RequireValue(argc, argv, index, option);
        } else if (option == "--unused" || option == "-u") {
            args.execute_function = ExcuteFuction::UNUSED_DEPENDENCY_CHECK;
        } else if (option == "--time" || option == "-T") {
            args.execute_function = ExcuteFuction::BUILD_TIME_ANALYZE;
        } else if (option == "--bazel_path" || option == "-b") {
            args.bazel_binary = RequireValue(argc, argv, index, option);
        } else if (option == "--output" || option == "-o") {
            args.output_path = RequireValue(argc, argv, index, option);
        } else if (option == "--format" || option == "-f") {
            args.output_format = ParseOutputFormat(RequireValue(argc, argv, index, option));
        } else if (option == "--port") {
            args.SetPort(RequireValue(argc, argv, index, option));
        } else if (option == "--verbose" || option == "-v") {
            args.verbose = true;
        } else if (option == "--ui") {
            args.ui_mode = true;
        } else if (option == "--tests" || option == "-t" || option == "-tests") {
            args.include_tests = true;
        } else if (option == "--help" || option == "-h") {
            throw HelpRequested{};
        } else {
            throw std::invalid_argument("Unknown option: " + option);
        }
    }

    args.Validate();
    return args;
}

void CommandLineArgs::PrintHelp(std::ostream& os) {
    os << "Bazel Dependency Analyzer\n";
    os << "Default to cyclic dependency detection when no analysis mode is selected.\n";
    os << "Usage: bazel-deps-analyzer [OPTIONS]\n\n";
    os << "Options:\n";
    os << "  -w, --workspace PATH    Bazel workspace path (required unless --ui)\n";
    os << "  -b, --bazel_path PATH   Bazel executable path\n";
    os << "  -u, --unused            Analyze unused dependencies\n";
    os << "  -t, --tests             Include test targets in analysis\n";
    os << "  -T, --time              Analyze build time\n";
    os << "  -o, --output FILE       Output file path\n";
    os << "  -f, --format FORMAT     Output format: console, markdown, json, html\n";
    os << "      --ui                Start local web UI server\n";
    os << "      --port PORT         Web UI port (default: 8080)\n";
    os << "  -v, --verbose           Enable verbose logging\n";
    os << "  -h, --help              Show this help message\n";
    os << "\nExamples:\n";
    os << "  bazel-deps-analyzer -w /path/to/workspace\n";
    os << "  bazel-deps-analyzer -w . --unused -f json -o unused.json\n";
    os << "  bazel-deps-analyzer -w . -t -f markdown -o report.md\n";
    os << "  bazel-deps-analyzer -w . -T -f json -o build-time.json\n";
    os << "  bazel-deps-analyzer --ui --port 8080\n";
    os << "  bazel-deps-analyzer -w . --ui\n";
}

OutputFormat CommandLineArgs::ParseOutputFormat(const std::string& format_str) {
    if (format_str == "console" || format_str == "text") {
        return OutputFormat::CONSOLE;
    }
    if (format_str == "markdown" || format_str == "md") {
        return OutputFormat::MARKDOWN;
    }
    if (format_str == "json") {
        return OutputFormat::JSON;
    }
    if (format_str == "html") {
        return OutputFormat::HTML;
    }

    throw std::invalid_argument("Unknown output format: " + format_str);
}

std::string CommandLineArgs::RequireValue(int argc, char* argv[], int& index, const std::string& option) {
    if (index + 1 >= argc) {
        throw std::invalid_argument("Missing value for option: " + option);
    }

    ++index;
    return argv[index];
}

void CommandLineArgs::SetPort(const std::string& port_str) {
    try {
        port = std::stoi(port_str);
    } catch (const std::exception&) {
        throw std::invalid_argument("Invalid port: " + port_str);
    }

    if (port <= 0 || port > 65535) {
        throw std::invalid_argument("Port must be between 1 and 65535");
    }
}

void CommandLineArgs::Validate() const {
    if (workspace_path.empty()) {
        if (ui_mode) {
            return;
        }
        throw std::invalid_argument("Workspace path is required");
    }

    const fs::path workspace(workspace_path);
    if (!fs::exists(workspace)) {
        throw std::invalid_argument("Workspace path does not exist: " + workspace_path);
    }

    if (!fs::is_directory(workspace)) {
        throw std::invalid_argument("Workspace path is not a directory: " + workspace_path);
    }

    if (!IsBazelWorkspace(workspace)) {
        throw std::invalid_argument(
            "The specified path is not a valid Bazel workspace: " + workspace_path);
    }
}
