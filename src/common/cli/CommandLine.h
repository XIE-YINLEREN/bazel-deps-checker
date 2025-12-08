#pragma once

#include <cstring>
#include <filesystem>
#include <iostream>
#include <cstring>

#include "log/logger.h"
#include "pipe.h"
#include "struct.h"


class CommandLineArgs {
private:
    CommandLineArgs() = default;

    CommandLineArgs(int argc, char* argv[]);

    OutputFormat ParseOutputFormat(const std::string& format_str) const;

    // 解析命令行参数
    void ParseCommandLine(int argc, char* argv[]);

    // 显示使用说明
    void PrintHelp() const;

public:
    static CommandLineArgs* GetInstance(int argc, char* argv[]);

    // 工作区根路径
    std::string workspace_path{};

    // 输出路径
    std::string output_path{};

    // 设置bazel命令位置
    std::string bazel_binary;

    // 输出格式
    OutputFormat output_format{OutputFormat::CONSOLE};

    // 是否启用详细日志
    bool verbose{false};

    // 是否包含测试目标
    bool include_tests{false};

    // 单例实例
    static CommandLineArgs* CommandLineArgsHandle;

    // 执行功能
    ExcuteFuction execute_function{ExcuteFuction::CYCLIC_DEPENDENCY_DETECTION};
};