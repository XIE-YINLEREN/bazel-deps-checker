#pragma once

#include <string>
#include <vector>


struct BazelTarget {
    std::string name;
    std::string path;
    std::string full_label;
    std::string rule_type;
    std::vector<std::string> deps;
    std::vector<std::string> srcs;
    bool empty() const { return name.empty(); }
};
