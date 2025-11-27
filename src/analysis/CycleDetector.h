#pragma once

#include "graph/DependencyGraph.h"

struct CycleAnalysis {
    std::vector<std::string> cycle;
    std::string cycle_type;
    std::vector<std::string> suggested_fixes;
};

class CycleDetector {
public:
    explicit CycleDetector(const DependencyGraph& graph);
    
    std::vector<CycleAnalysis> AnalyzeCycles();
    
    // 分类循环类型并提供修复建议
    CycleAnalysis ClassifyCycle(const std::vector<std::string>& cycle);
    
    // 自动修复建议
    std::string GenerateFixSuggestion(const CycleAnalysis& analysis);
    
private:
    const DependencyGraph& graph_;
    
    bool IsDirectCycle(const std::vector<std::string>& cycle) const;
    bool IsDiamondDependency(const std::vector<std::string>& cycle) const;
    std::string ExtractCommonInterface(const std::vector<std::string>& targets);
};