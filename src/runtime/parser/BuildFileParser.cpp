#include "BuildFileParser.h"


BuildFileParser::BuildFileParser(const std::string& workspace_path)
    : workspace_path(workspace_path) {
    bazel_query_parser = std::make_unique<AdvancedBazelQueryParser>(workspace_path);
}


std::unordered_map<std::string, BazelTarget> BuildFileParser::ParseWorkspace() {
    return bazel_query_parser->ParseWorkspace();
}