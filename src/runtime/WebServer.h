#pragma once

#include "runtime/BazelAnalyzerSDK.h"
#include "cli/CommandLine.h"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>

class WebServer {
public:
    explicit WebServer(CommandLineArgs base_args);

    int Run();

    struct HttpRequest {
        std::string method;
        std::string path;
        std::string route_path;
        std::string body;
        std::map<std::string, std::string> query_params;
    };

    struct HttpResponse {
        int status_code{200};
        std::string content_type{"text/plain; charset=utf-8"};
        std::string body;
    };

    struct CachedAnalyzeResponse {
        std::string response_body;
        std::string response_body_cache_hit;
    };

    struct AnalysisTask {
        std::string id;
        std::string status;
        std::string message;
        std::string response_body;
        std::string result_file_path;
        std::string mode;
        std::string workspace_path;
        std::string bazel_binary;
        bool include_tests{false};
        bool cache_hit{false};
        double total_ms{0.0};
        std::int64_t created_at_ms{0};
        std::int64_t updated_at_ms{0};
        std::string status_lower;
        std::string mode_lower;
        std::string searchable_text_lower;
    };

private:
    CommandLineArgs base_args_;
    mutable std::mutex cache_mutex_;
    mutable std::unordered_map<std::string, CachedAnalyzeResponse> analyze_cache_;
    mutable std::mutex tasks_mutex_;
    mutable std::unordered_map<std::string, AnalysisTask> tasks_;
    mutable std::atomic<unsigned long long> next_task_id_{1};

    int CreateListenSocket() const;
    void HandleClient(int client_fd) const;
    HttpRequest ParseRequest(const std::string& raw_request) const;
    HttpResponse RouteRequest(const HttpRequest& request) const;
    HttpResponse HandleAnalyzeRequest(const std::string& body) const;
    HttpResponse HandleTaskStatusRequest(const HttpRequest& request, const std::string& task_id) const;
    HttpResponse HandleTaskListRequest(const HttpRequest& request) const;
    HttpResponse HandleEnvironmentRequest() const;
    HttpResponse HandleCacheStatusRequest() const;
    HttpResponse HandleCacheClearRequest() const;
    void SendResponse(int client_fd, const HttpResponse& response) const;
    std::string BuildCacheKey(const CommandLineArgs& args) const;
    std::string BuildAnalyzeResponseBody(
        const CommandLineArgs& args,
        const std::pair<std::string, std::string>& reports,
        bool cache_hit,
        const BazelAnalyzerSDK::PerformanceInfo& performance) const;
    std::string CreateTask(const CommandLineArgs& request_args) const;
    void UpdateTaskStatus(
        const std::string& task_id,
        const std::string& status,
        const std::string& message,
        const std::string& response_body = "") const;
    void RunAnalyzeTask(
        const std::string& task_id,
        CommandLineArgs request_args,
        std::string cache_key) const;
    void LoadTaskHistory();
    void PersistTaskHistoryLocked() const;
    void TrimTasksLocked() const;
    std::filesystem::path GetTaskHistoryPath() const;
    std::filesystem::path GetTaskResultsDirectory() const;
    std::string LoadTaskResultBodyLocked(const AnalysisTask& task) const;
    void SaveTaskResultBodyLocked(AnalysisTask& task, const std::string& response_body) const;
    void DeleteTaskResultFileLocked(const AnalysisTask& task) const;

    static std::string BuildStatusText(int status_code);
    static std::string BuildUiPage();
};
