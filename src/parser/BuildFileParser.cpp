#include "BuildFileParser.h"


BuildFileParser::BuildFileParser(const std::string& workspace_root)
    : workspace_root(workspace_root) {
    bazel_query_parser = std::make_unique<AdvancedBazelQueryParser>(workspace_root);
}


std::unordered_map<std::string, BazelTarget> BuildFileParser::ParseWorkspace() {
    return bazel_query_parser->ParseWorkspace();
}