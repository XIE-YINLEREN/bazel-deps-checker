#pragma once

#include <exception>
#include <iosfwd>
#include <string>

#include "struct.h"

class HelpRequested : public std::exception {
public:
    const char* what() const noexcept override {
        return "Help requested";
    }
};

class CommandLineArgs {
public:
    static CommandLineArgs Parse(int argc, char* argv[]);
    static void PrintHelp(std::ostream& os);

    std::string workspace_path{};
    std::string output_path{};
    std::string bazel_binary{"bazel"};
    OutputFormat output_format{OutputFormat::CONSOLE};
    int port{8080};
    bool verbose{false};
    bool ui_mode{false};
    bool include_tests{false};
    ExcuteFuction execute_function{ExcuteFuction::CYCLIC_DEPENDENCY_DETECTION};

    static OutputFormat ParseOutputFormat(const std::string& format_str);
    static std::string RequireValue(int argc, char* argv[], int& index, const std::string& option);

    void SetPort(const std::string& port_str);
    void Validate() const;
};
