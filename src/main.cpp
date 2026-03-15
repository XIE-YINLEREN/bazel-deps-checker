#include "cli/CommandLine.h"
#include "log/logger.h"
#include "runtime/BazelAnalyzerSDK.h"
#include "runtime/WebServer.h"

#include <iostream>
#include <utility>

int main(int argc, char* argv[]) {
    try {
        CommandLineArgs args = CommandLineArgs::Parse(argc, argv);
        Logger::getInstance().setLogLevel(args.verbose ? LogLevel::DEBUG : LogLevel::WARN);

        if (args.ui_mode) {
            WebServer server(std::move(args));
            return server.Run();
        }

        BazelAnalyzerSDK sdk(std::move(args));
        sdk.executeCommand();
        return 0;
    } catch (const HelpRequested&) {
        CommandLineArgs::PrintHelp(std::cout);
        return 0;
    } catch (const std::invalid_argument& e) {
        std::cerr << "Error: " << e.what() << "\n\n";
        CommandLineArgs::PrintHelp(std::cerr);
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
