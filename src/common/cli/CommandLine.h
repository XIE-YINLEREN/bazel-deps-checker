#pragma once

#include <cstring>

#include "log/logger.h"

enum OutputFormat {
    TEXT,
    JSON
};


class CommandLineArgs {
private:
    CommandLineArgs() = default;

    CommandLineArgs(int argc, char* argv[]);

    void ParseCommandLine(int argc, char* argv[]);

    void useage() const;

public:
    static CommandLineArgs* GetInstance(int argc, char* argv[]);

    // ï¿½ï¿½ï¿½ï¿½ï¿½Ê?
    std::string workspace_root{};

    // ï¿½ï¿½ï¿½ï¿½ï¿½Ê?
    OutputFormat output_format{TEXT};

    // ï¿½ï¿½ï¿½ï¿½Ö¸ï¿½ï¿½
    static CommandLineArgs* CommandLineArgsHandle;
};