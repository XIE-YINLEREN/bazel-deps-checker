#pragma once

#include "cli/CommandLine.h"

#include <memory>

class BazelAnalyzerSDK {
public:
    BazelAnalyzerSDK(int argc, char* argv[]);

    ~BazelAnalyzerSDK();
    
    void executeCommand();

    static CommandLineArgs* args;

private:
    class Impl;

    std::unique_ptr<Impl> impl_;
};