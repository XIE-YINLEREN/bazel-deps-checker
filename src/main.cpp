#include "runtime/BazelAnalyzerSDK.h"

int main(int argc, char* argv[]) {

    BazelAnalyzerSDK sdk(argc, argv);
    if (sdk.args->workspace_path.empty()) {
        LOG_ERROR("empty bazel workspace path");
    }
        
    try {
        sdk.executeCommand();
    } catch (const std::exception& e) {
        std::string log_msg = e.what();
        LOG_ERROR(log_msg);
        return 1;
    }
    
    return 0;
}