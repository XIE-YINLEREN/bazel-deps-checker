#include "CommandLine.h"

CommandLineArgs* CommandLineArgs::CommandLineArgsHandle = nullptr;

CommandLineArgs::CommandLineArgs(int argc, char* argv[]) {
    ParseCommandLine(argc, argv);
}


void CommandLineArgs::ParseCommandLine(int argc, char* argv[]) {
    if (argc < 2) {
        LOG_ERROR("Usage: bazel-deps-checker <workspace_root> [--output_format=text|json]");
        return;
    }

    workspace_root = argv[1];

    if (strcmp(argv[2], "-j") ==0 ) {
        output_format = JSON;
        return;
    }
    else if (strcmp(argv[2], "-t") == 0) {
        output_format = TEXT;
        return;
    }
}

CommandLineArgs* CommandLineArgs::GetInstance(int argc, char* argv[]) {
    if (CommandLineArgsHandle == nullptr) {
        CommandLineArgsHandle = new CommandLineArgs(argc, argv);
    }
    return CommandLineArgsHandle;
}


void CommandLineArgs::useage() const {
    LOG_INFO("Usage: bazel-deps-checker <workspace_root> -j |-t [text|json]");
}