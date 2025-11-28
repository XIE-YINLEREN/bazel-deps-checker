#include "DependencyGraph.h"

DependencyGraph::DependencyGraph(std::unordered_map<std::string, BazelTarget> targets) {
    grap_targets = targets;
    BuildGraph();
}


void DependencyGraph::BuildGraph() {

    // 过滤外部依赖
    for (auto& target : grap_targets) {
        target.second.deps.erase(
            std::remove_if(target.second.deps.begin(), target.second.deps.end(),
                [](const std::string& dep) {
                    return dep.find("@") == 0;
                }),
            target.second.deps.end()
        );
    }

}
