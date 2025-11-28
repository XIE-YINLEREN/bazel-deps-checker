#pragma once

#include <memory>

#include "graph/DependencyGraph.h"

class BuildFileDependencyer {

private:
    std::unique_ptr<DependencyGraph> bazel_dependency;

public:
    explicit BuildFileDependencyer(std::unordered_map<std::string, BazelTarget> targets);

    
};