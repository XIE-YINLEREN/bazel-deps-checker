#include "BuildFileDependencyer.h"

BuildFileDependencyer::BuildFileDependencyer(std::unordered_map<std::string, BazelTarget> targets) {
    bazel_dependency = std::make_unique<DependencyGraph>(targets);
}

const DependencyGraph& BuildFileDependencyer::GetDependencyGraph() const {
    return *(bazel_dependency.get());
}