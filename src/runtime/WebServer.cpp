#include "WebServer.h"

#include "log/logger.h"
#include "parser/AdvancedBazelQueryParser.h"
#include "runtime/BazelAnalyzerSDK.h"
#include <nlohmann/json.hpp>

#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <utility>
#include <vector>
#include <unistd.h>

using json = nlohmann::json;

namespace {

constexpr int kBacklog = 16;
constexpr size_t kMaxRequestSize = 1024 * 1024;
constexpr size_t kMaxPersistedTasks = 60;

bool IsExecutableFile(const std::filesystem::path& path) {
    if (path.empty()) {
        return false;
    }
    std::error_code error_code;
    if (!std::filesystem::exists(path, error_code) ||
        !std::filesystem::is_regular_file(path, error_code)) {
        return false;
    }
    return ::access(path.c_str(), X_OK) == 0;
}

std::vector<std::string> DetectBazelBinaries() {
    std::vector<std::string> detected;
    auto append_if_missing = [&detected](const std::string& value) {
        if (value.empty()) {
            return;
        }
        if (std::find(detected.begin(), detected.end(), value) == detected.end()) {
            detected.push_back(value);
        }
    };

    const std::vector<std::string> path_candidates = {"bazel", "bazelisk"};
    const char* path_env = std::getenv("PATH");
    if (path_env != nullptr) {
        std::stringstream path_stream(path_env);
        std::string segment;
        while (std::getline(path_stream, segment, ':')) {
            if (segment.empty()) {
                continue;
            }
            for (const std::string& candidate : path_candidates) {
                const std::filesystem::path executable =
                    std::filesystem::path(segment) / candidate;
                if (IsExecutableFile(executable)) {
                    append_if_missing(executable.string());
                }
            }
        }
    }

    const std::vector<std::string> absolute_candidates = {
        "/opt/homebrew/bin/bazel",
        "/opt/homebrew/bin/bazelisk",
        "/usr/local/bin/bazel",
        "/usr/local/bin/bazelisk",
        "/usr/bin/bazel",
        "/usr/bin/bazelisk",
    };
    for (const std::string& candidate : absolute_candidates) {
        if (IsExecutableFile(candidate)) {
            append_if_missing(candidate);
        }
    }

    return detected;
}

std::int64_t CurrentEpochMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string ModeToString(ExcuteFuction function) {
    switch (function) {
        case ExcuteFuction::UNUSED_DEPENDENCY_CHECK:
            return "unused";
        case ExcuteFuction::CYCLIC_DEPENDENCY_DETECTION:
            return "cycle";
        case ExcuteFuction::BUILD_TIME_ANALYZE:
            return "build-time";
    }

    return "cycle";
}

ExcuteFuction ParseMode(const std::string& mode) {
    if (mode == "unused") {
        return ExcuteFuction::UNUSED_DEPENDENCY_CHECK;
    }
    if (mode == "build-time") {
        return ExcuteFuction::BUILD_TIME_ANALYZE;
    }
    return ExcuteFuction::CYCLIC_DEPENDENCY_DETECTION;
}

std::string ExtractHeaderValue(const std::string& raw_request, const std::string& header_name) {
    const std::string marker = "\r\n" + header_name + ":";
    const size_t start = raw_request.find(marker);
    if (start == std::string::npos) {
        return "";
    }

    const size_t value_start = start + marker.size();
    const size_t line_end = raw_request.find("\r\n", value_start);
    if (line_end == std::string::npos) {
        return "";
    }

    size_t trimmed_start = value_start;
    while (trimmed_start < line_end &&
           (raw_request[trimmed_start] == ' ' || raw_request[trimmed_start] == '\t')) {
        ++trimmed_start;
    }

    return raw_request.substr(trimmed_start, line_end - trimmed_start);
}

std::string DecodeUrlComponent(const std::string& value) {
    std::string decoded;
    decoded.reserve(value.size());
    for (size_t index = 0; index < value.size(); ++index) {
        const char ch = value[index];
        if (ch == '+') {
            decoded.push_back(' ');
            continue;
        }
        if (ch == '%' && index + 2 < value.size()) {
            const std::string hex = value.substr(index + 1, 2);
            try {
                const char decoded_char = static_cast<char>(std::stoi(hex, nullptr, 16));
                decoded.push_back(decoded_char);
                index += 2;
                continue;
            } catch (const std::exception&) {
            }
        }
        decoded.push_back(ch);
    }
    return decoded;
}

std::map<std::string, std::string> ParseQueryParams(const std::string& query) {
    std::map<std::string, std::string> result;
    std::istringstream stream(query);
    std::string item;
    while (std::getline(stream, item, '&')) {
        if (item.empty()) {
            continue;
        }
        const size_t equal_pos = item.find('=');
        const std::string key = DecodeUrlComponent(item.substr(0, equal_pos));
        const std::string value = equal_pos == std::string::npos
                                      ? ""
                                      : DecodeUrlComponent(item.substr(equal_pos + 1));
        if (!key.empty()) {
            result[key] = value;
        }
    }
    return result;
}

std::string Lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string BuildTaskSearchableLower(const WebServer::AnalysisTask& task) {
    return Lowercase(
        task.id + "\n" + task.workspace_path + "\n" + task.message + "\n" + task.mode + "\n" +
        task.bazel_binary);
}

void RefreshTaskDerivedFields(WebServer::AnalysisTask& task) {
    task.status_lower = Lowercase(task.status);
    task.mode_lower = Lowercase(task.mode);
    task.searchable_text_lower = BuildTaskSearchableLower(task);
}

bool ParseBoolParam(const std::map<std::string, std::string>& query_params, const std::string& key) {
    const auto it = query_params.find(key);
    if (it == query_params.end()) {
        return false;
    }

    const std::string value = Lowercase(it->second);
    return value == "1" || value == "true" || value == "yes" || value == "on";
}

json BuildTaskSummaryJson(const WebServer::AnalysisTask& task) {
    return json{
        {"task_id", task.id},
        {"status", task.status},
        {"message", task.message},
        {"mode", task.mode},
        {"workspace_path", task.workspace_path},
        {"bazel_binary", task.bazel_binary},
        {"include_tests", task.include_tests},
        {"cache_hit", task.cache_hit},
        {"total_ms", task.total_ms},
        {"created_at_ms", task.created_at_ms},
        {"updated_at_ms", task.updated_at_ms},
    };
}

WebServer::CachedAnalyzeResponse BuildCachedAnalyzeResponse(const std::string& response_body) {
    WebServer::CachedAnalyzeResponse cache_entry{response_body, response_body};
    try {
        json cached_response = json::parse(response_body);
        cached_response["cache_hit"] = true;
        cache_entry.response_body_cache_hit = cached_response.dump(2);
    } catch (const std::exception&) {
    }
    return cache_entry;
}

json TaskToJson(const WebServer::AnalysisTask& task) {
    json item = {
        {"task_id", task.id},
        {"status", task.status},
        {"message", task.message},
        {"result_file_path", task.result_file_path},
        {"mode", task.mode},
        {"workspace_path", task.workspace_path},
        {"bazel_binary", task.bazel_binary},
        {"include_tests", task.include_tests},
        {"cache_hit", task.cache_hit},
        {"total_ms", task.total_ms},
        {"created_at_ms", task.created_at_ms},
        {"updated_at_ms", task.updated_at_ms},
    };

    return item;
}

WebServer::AnalysisTask TaskFromJson(const json& item) {
    WebServer::AnalysisTask task;
    task.id = item.value("task_id", "");
    task.status = item.value("status", "completed");
    task.message = item.value("message", "");
    task.result_file_path = item.value("result_file_path", "");
    task.mode = item.value("mode", "");
    task.workspace_path = item.value("workspace_path", "");
    task.bazel_binary = item.value("bazel_binary", "bazel");
    task.include_tests = item.value("include_tests", false);
    task.cache_hit = item.value("cache_hit", false);
    task.total_ms = item.value("total_ms", 0.0);
    task.created_at_ms = item.value("created_at_ms", CurrentEpochMillis());
    task.updated_at_ms = item.value("updated_at_ms", task.created_at_ms);
    task.response_body = item.value("response_body", "");
    RefreshTaskDerivedFields(task);
    return task;
}

void FillTaskMetadataFromResponse(WebServer::AnalysisTask& task, const std::string& response_body) {
    if (response_body.empty()) {
        return;
    }

    try {
        const json result = json::parse(response_body);
        task.mode = result.value("mode", task.mode);
        task.workspace_path = result.value("workspace_path", task.workspace_path);
        task.bazel_binary = result.value("bazel_binary", task.bazel_binary);
        task.include_tests = result.value("include_tests", task.include_tests);
        task.cache_hit = result.value("cache_hit", task.cache_hit);
        task.total_ms = result.value("performance", json::object()).value("total_ms", task.total_ms);
        RefreshTaskDerivedFields(task);
    } catch (const std::exception&) {
    }
}

}  // namespace

WebServer::WebServer(CommandLineArgs base_args) : base_args_(std::move(base_args)) {
    LoadTaskHistory();
}

int WebServer::Run() {
    const int server_fd = CreateListenSocket();

    std::cout << "Web UI started at http://127.0.0.1:" << base_args_.port << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl;

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        const int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (client_fd < 0) {
            LOG_WARN("Accept failed: " + std::string(std::strerror(errno)));
            continue;
        }

        std::thread([this, client_fd]() {
            HandleClient(client_fd);
        }).detach();
    }

    return 0;
}

int WebServer::CreateListenSocket() const {
    const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int enable = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
        close(server_fd);
        throw std::runtime_error("Failed to configure socket");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(static_cast<uint16_t>(base_args_.port));

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        const std::string message =
            "Failed to bind port " + std::to_string(base_args_.port) + ": " + std::strerror(errno);
        close(server_fd);
        throw std::runtime_error(message);
    }

    if (listen(server_fd, kBacklog) < 0) {
        close(server_fd);
        throw std::runtime_error("Failed to listen on socket");
    }

    return server_fd;
}

void WebServer::HandleClient(int client_fd) const {
    std::string request_text;
    std::array<char, 4096> buffer{};

    while (request_text.size() < kMaxRequestSize) {
        const ssize_t bytes_read = recv(client_fd, buffer.data(), buffer.size(), 0);
        if (bytes_read <= 0) {
            break;
        }

        request_text.append(buffer.data(), static_cast<size_t>(bytes_read));

        const size_t header_end = request_text.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            continue;
        }

        const std::string content_length_header = ExtractHeaderValue(request_text, "Content-Length");
        size_t content_length = 0;
        if (!content_length_header.empty()) {
            try {
                content_length = static_cast<size_t>(std::stoul(content_length_header));
            } catch (const std::exception&) {
                break;
            }
        }

        if (request_text.size() >= header_end + 4 + content_length) {
            break;
        }
    }

    HttpResponse response;
    try {
        const HttpRequest request = ParseRequest(request_text);
        response = RouteRequest(request);
    } catch (const std::invalid_argument& error) {
        response.status_code = 400;
        response.content_type = "application/json; charset=utf-8";
        response.body = json({{"ok", false}, {"error", error.what()}}).dump(2);
    } catch (const std::exception& error) {
        response.status_code = 500;
        response.content_type = "application/json; charset=utf-8";
        response.body = json({{"ok", false}, {"error", error.what()}}).dump(2);
    }

    SendResponse(client_fd, response);
    close(client_fd);
}

WebServer::HttpRequest WebServer::ParseRequest(const std::string& raw_request) const {
    if (raw_request.empty()) {
        throw std::invalid_argument("Empty HTTP request");
    }

    const size_t line_end = raw_request.find("\r\n");
    if (line_end == std::string::npos) {
        throw std::invalid_argument("Malformed HTTP request");
    }

    std::istringstream line_stream(raw_request.substr(0, line_end));
    HttpRequest request;
    std::string version;
    if (!(line_stream >> request.method >> request.path >> version)) {
        throw std::invalid_argument("Malformed request line");
    }

    const size_t query_pos = request.path.find('?');
    request.route_path = query_pos == std::string::npos ? request.path : request.path.substr(0, query_pos);
    if (query_pos != std::string::npos && query_pos + 1 < request.path.size()) {
        request.query_params = ParseQueryParams(request.path.substr(query_pos + 1));
    }

    const size_t body_start = raw_request.find("\r\n\r\n");
    if (body_start != std::string::npos) {
        request.body = raw_request.substr(body_start + 4);
    }

    return request;
}

WebServer::HttpResponse WebServer::RouteRequest(const HttpRequest& request) const {
    if (request.method == "GET" && request.route_path == "/") {
        return HttpResponse{200, "text/html; charset=utf-8", BuildUiPage()};
    }

    if (request.method == "GET" && request.route_path == "/api/health") {
        json payload = {{"ok", true}, {"port", base_args_.port}, {"ui", true}};
        return HttpResponse{200, "application/json; charset=utf-8", payload.dump(2)};
    }

    if (request.method == "GET" && request.route_path == "/api/cache") {
        return HandleCacheStatusRequest();
    }

    if (request.method == "GET" && request.route_path == "/api/environment") {
        return HandleEnvironmentRequest();
    }

    if (request.method == "POST" && request.route_path == "/api/cache/clear") {
        return HandleCacheClearRequest();
    }

    if (request.method == "GET" && request.route_path == "/api/tasks") {
        return HandleTaskListRequest(request);
    }

    if (request.method == "POST" && request.route_path == "/api/analyze") {
        return HandleAnalyzeRequest(request.body);
    }

    if (request.method == "GET" && request.route_path.rfind("/api/tasks/", 0) == 0) {
        return HandleTaskStatusRequest(
            request, request.route_path.substr(std::string("/api/tasks/").size()));
    }

    return HttpResponse{
        404,
        "application/json; charset=utf-8",
        json({{"ok", false}, {"error", "Not found"}}).dump(2),
    };
}

WebServer::HttpResponse WebServer::HandleAnalyzeRequest(const std::string& body) const {
    const json request_json = json::parse(body.empty() ? "{}" : body);
    const bool force_refresh = request_json.value("force_refresh", false);

    CommandLineArgs request_args = base_args_;
    request_args.ui_mode = false;
    request_args.output_path.clear();
    request_args.output_format = OutputFormat::JSON;
    request_args.workspace_path =
        request_json.value("workspace_path", request_args.workspace_path);
    request_args.bazel_binary = request_json.value("bazel_binary", request_args.bazel_binary);
    request_args.include_tests = request_json.value("include_tests", request_args.include_tests);
    request_args.execute_function = ParseMode(request_json.value("mode", ModeToString(request_args.execute_function)));

    if (request_args.bazel_binary.empty() || request_args.bazel_binary == "bazel") {
        const std::vector<std::string> detected_binaries = DetectBazelBinaries();
        if (!detected_binaries.empty()) {
            request_args.bazel_binary = detected_binaries.front();
        }
    }

    request_args.Validate();

    const std::string cache_key = BuildCacheKey(request_args);
    if (!force_refresh) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        const auto cached = analyze_cache_.find(cache_key);
        if (cached != analyze_cache_.end()) {
            return HttpResponse{
                200,
                "application/json; charset=utf-8",
                cached->second.response_body_cache_hit.empty()
                    ? cached->second.response_body
                    : cached->second.response_body_cache_hit};
        }
    }

    const std::string task_id = CreateTask(request_args);
    UpdateTaskStatus(task_id, "queued", "任务已创建，等待执行。");
    std::thread([this, task_id, request_args, cache_key]() {
        RunAnalyzeTask(task_id, request_args, cache_key);
    }).detach();

    return HttpResponse{
        202,
        "application/json; charset=utf-8",
        json({
            {"ok", true},
            {"task_id", task_id},
            {"status", "queued"},
            {"cache_hit", false},
            {"message", "任务已加入后台队列"},
        }).dump(2),
    };
}

std::string WebServer::BuildCacheKey(const CommandLineArgs& args) const {
    std::ostringstream os;
    os << args.workspace_path << '\n'
       << args.bazel_binary << '\n'
       << static_cast<int>(args.execute_function) << '\n'
       << (args.include_tests ? "1" : "0");
    return os.str();
}

std::string WebServer::BuildAnalyzeResponseBody(
    const CommandLineArgs& args,
    const std::pair<std::string, std::string>& reports,
    bool cache_hit,
    const BazelAnalyzerSDK::PerformanceInfo& performance) const {
    json report_payload = json::parse(reports.first);
    json response = {
        {"ok", true},
        {"mode", ModeToString(args.execute_function)},
        {"workspace_path", args.workspace_path},
        {"bazel_binary", args.bazel_binary},
        {"include_tests", args.include_tests},
        {"cache_hit", cache_hit},
        {"performance", {
            {"dependency_prepare_ms", performance.dependency_prepare_ms},
            {"analysis_ms", performance.analysis_ms},
            {"report_render_ms", performance.report_render_ms},
            {"total_ms", performance.total_ms},
            {"reused_dependency_context", performance.reused_dependency_context},
        }},
        {"report", report_payload},
        {"html_report", reports.second},
    };
    return response.dump(2);
}

WebServer::HttpResponse WebServer::HandleTaskStatusRequest(
    const HttpRequest& request,
    const std::string& task_id) const {
    AnalysisTask task_snapshot;
    {
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        const auto it = tasks_.find(task_id);
        if (it == tasks_.end()) {
            return HttpResponse{
                404,
                "application/json; charset=utf-8",
                json({{"ok", false}, {"error", "Task not found"}}).dump(2),
            };
        }
        task_snapshot = it->second;
    }

    json response = BuildTaskSummaryJson(task_snapshot);
    response["ok"] = true;

    const bool include_result = ParseBoolParam(request.query_params, "include_result");
    const std::string result_body =
        task_snapshot.status == "completed" ? LoadTaskResultBodyLocked(task_snapshot) : "";
    response["result_ready"] = !result_body.empty();

    if (include_result && !result_body.empty()) {
        response["result"] = json::parse(result_body);
    }

    return HttpResponse{200, "application/json; charset=utf-8", response.dump(2)};
}

WebServer::HttpResponse WebServer::HandleTaskListRequest(const HttpRequest& request) const {
    std::vector<AnalysisTask> ordered_tasks;
    {
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        ordered_tasks.reserve(tasks_.size());
        for (const auto& [task_id, task] : tasks_) {
            (void)task_id;
            if (task.id.empty()) {
                continue;
            }
            ordered_tasks.push_back(task);
        }
    }

    std::sort(
        ordered_tasks.begin(),
        ordered_tasks.end(),
        [](const AnalysisTask& left, const AnalysisTask& right) {
            if (left.updated_at_ms != right.updated_at_ms) {
                return left.updated_at_ms > right.updated_at_ms;
            }
            return left.id > right.id;
        });

    size_t limit = 8;
    size_t offset = 0;
    try {
        if (const auto it = request.query_params.find("limit"); it != request.query_params.end()) {
            limit = std::max<size_t>(1, std::min<size_t>(100, static_cast<size_t>(std::stoul(it->second))));
        }
        if (const auto it = request.query_params.find("offset"); it != request.query_params.end()) {
            offset = static_cast<size_t>(std::stoul(it->second));
        }
    } catch (const std::exception&) {
        throw std::invalid_argument("Invalid task list pagination parameters");
    }

    const std::string mode_filter = Lowercase(
        request.query_params.count("mode") ? request.query_params.at("mode") : "");
    const std::string status_filter = Lowercase(
        request.query_params.count("status") ? request.query_params.at("status") : "");
    const std::string text_filter = Lowercase(
        request.query_params.count("q") ? request.query_params.at("q") : "");

    json tasks_json = json::array();
    size_t total_filtered = 0;
    size_t emitted = 0;
    for (const AnalysisTask& task : ordered_tasks) {
        if (!mode_filter.empty() && task.mode_lower != mode_filter) {
            continue;
        }
        if (!status_filter.empty() && task.status_lower != status_filter) {
            continue;
        }
        if (!text_filter.empty() &&
            task.searchable_text_lower.find(text_filter) == std::string::npos) {
                continue;
        }

        ++total_filtered;
        if (total_filtered <= offset) {
            continue;
        }
        if (emitted >= limit) {
            continue;
        }

        tasks_json.push_back(BuildTaskSummaryJson(task));
        ++emitted;
    }

    return HttpResponse{
        200,
        "application/json; charset=utf-8",
        json({
            {"ok", true},
            {"tasks", tasks_json},
            {"total", total_filtered},
            {"limit", limit},
            {"offset", offset},
            {"has_more", offset + emitted < total_filtered},
        }).dump(2),
    };
}

WebServer::HttpResponse WebServer::HandleEnvironmentRequest() const {
    const std::vector<std::string> detected_binaries = DetectBazelBinaries();
    const std::string recommended_binary =
        detected_binaries.empty() ? std::string() : detected_binaries.front();

    json response = {
        {"ok", true},
        {"bazel_available", !detected_binaries.empty()},
        {"detected_bazel_binaries", detected_binaries},
        {"recommended_bazel_binary", recommended_binary},
    };

    return HttpResponse{200, "application/json; charset=utf-8", response.dump(2)};
}

WebServer::HttpResponse WebServer::HandleCacheStatusRequest() const {
    size_t response_cache_size = 0;
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        response_cache_size = analyze_cache_.size();
    }

    size_t task_count = 0;
    {
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        task_count = tasks_.size();
    }

    json response = {
        {"ok", true},
        {"response_cache_size", response_cache_size},
        {"dependency_context_cache_size", BazelAnalyzerSDK::GetDependencyContextCacheSize()},
        {"workspace_parser_cache_size", AdvancedBazelQueryParser::GetWorkspaceCacheSize()},
        {"task_count", task_count},
    };

    return HttpResponse{200, "application/json; charset=utf-8", response.dump(2)};
}

WebServer::HttpResponse WebServer::HandleCacheClearRequest() const {
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        analyze_cache_.clear();
    }
    BazelAnalyzerSDK::ClearDependencyContextCache();
    AdvancedBazelQueryParser::ClearWorkspaceCache();

    json response = {
        {"ok", true},
        {"message", "All caches cleared"},
    };
    return HttpResponse{200, "application/json; charset=utf-8", response.dump(2)};
}

std::string WebServer::CreateTask(const CommandLineArgs& request_args) const {
    const std::string task_id = "task-" + std::to_string(next_task_id_.fetch_add(1));
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    const std::int64_t now_ms = CurrentEpochMillis();
    tasks_[task_id] = AnalysisTask{
        task_id,
        "queued",
        "任务已创建。",
        "",
        "",
        ModeToString(request_args.execute_function),
        request_args.workspace_path,
        request_args.bazel_binary,
        request_args.include_tests,
        false,
        0.0,
        now_ms,
        now_ms,
        "",
        "",
        ""};
    RefreshTaskDerivedFields(tasks_[task_id]);
    TrimTasksLocked();
    PersistTaskHistoryLocked();
    return task_id;
}

void WebServer::UpdateTaskStatus(
    const std::string& task_id,
    const std::string& status,
    const std::string& message,
    const std::string& response_body) const {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    auto it = tasks_.find(task_id);
    if (it == tasks_.end()) {
        const std::int64_t now_ms = CurrentEpochMillis();
        tasks_[task_id] = AnalysisTask{
            task_id,
            status,
            message,
            "",
            "",
            "",
            "",
            "bazel",
            false,
            false,
            0.0,
            now_ms,
            now_ms,
            "",
            "",
            ""};
        if (!response_body.empty()) {
            SaveTaskResultBodyLocked(tasks_[task_id], response_body);
            FillTaskMetadataFromResponse(tasks_[task_id], response_body);
        }
        RefreshTaskDerivedFields(tasks_[task_id]);
        TrimTasksLocked();
        if (status != "running") {
            PersistTaskHistoryLocked();
        }
        return;
    }

    const bool status_changed = it->second.status != status;
    it->second.status = status;
    it->second.message = message;
    it->second.updated_at_ms = CurrentEpochMillis();
    if (!response_body.empty()) {
        SaveTaskResultBodyLocked(it->second, response_body);
        FillTaskMetadataFromResponse(it->second, response_body);
    }
    RefreshTaskDerivedFields(it->second);
    TrimTasksLocked();
    if (status != "running" || status_changed) {
        PersistTaskHistoryLocked();
    }
}

void WebServer::RunAnalyzeTask(
    const std::string& task_id,
    CommandLineArgs request_args,
    std::string cache_key) const {
    try {
        UpdateTaskStatus(task_id, "running", "准备分析环境…");
        BazelAnalyzerSDK sdk(request_args);

        if (request_args.execute_function == ExcuteFuction::CYCLIC_DEPENDENCY_DETECTION ||
            request_args.execute_function == ExcuteFuction::UNUSED_DEPENDENCY_CHECK) {
            UpdateTaskStatus(task_id, "running", "解析 Bazel 目标与依赖图…");
            const auto reports = sdk.renderDependencyJsonAndHtmlReports();
            UpdateTaskStatus(task_id, "running", "生成 cycle / unused 报告…");

            CommandLineArgs cycle_args = request_args;
            cycle_args.execute_function = ExcuteFuction::CYCLIC_DEPENDENCY_DETECTION;
            const std::string cycle_cache_key = BuildCacheKey(cycle_args);
            const std::string cycle_response_body =
                BuildAnalyzeResponseBody(cycle_args, reports.cycle, false, sdk.getLastPerformanceInfo());

            CommandLineArgs unused_args = request_args;
            unused_args.execute_function = ExcuteFuction::UNUSED_DEPENDENCY_CHECK;
            const std::string unused_cache_key = BuildCacheKey(unused_args);
            const std::string unused_response_body =
                BuildAnalyzeResponseBody(unused_args, reports.unused, false, sdk.getLastPerformanceInfo());

            {
                std::lock_guard<std::mutex> lock(cache_mutex_);
                analyze_cache_[cycle_cache_key] = BuildCachedAnalyzeResponse(cycle_response_body);
                analyze_cache_[unused_cache_key] = BuildCachedAnalyzeResponse(unused_response_body);
            }

            UpdateTaskStatus(
                task_id,
                "completed",
                "分析完成。",
                request_args.execute_function == ExcuteFuction::CYCLIC_DEPENDENCY_DETECTION
                    ? cycle_response_body
                    : unused_response_body);
            return;
        }

        UpdateTaskStatus(task_id, "running", "执行构建耗时分析…");
        const auto reports = sdk.renderJsonAndHtmlReports();
        UpdateTaskStatus(task_id, "running", "整理 build-time 报告输出…");
        const std::string response_body =
            BuildAnalyzeResponseBody(request_args, reports, false, sdk.getLastPerformanceInfo());
        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            analyze_cache_[cache_key] = BuildCachedAnalyzeResponse(response_body);
        }
        UpdateTaskStatus(task_id, "completed", "分析完成。", response_body);
    } catch (const std::exception& error) {
        UpdateTaskStatus(task_id, "failed", error.what());
    }
}

void WebServer::SendResponse(int client_fd, const HttpResponse& response) const {
    std::ostringstream os;
    os << "HTTP/1.1 " << response.status_code << " " << BuildStatusText(response.status_code) << "\r\n";
    os << "Content-Type: " << response.content_type << "\r\n";
    os << "Content-Length: " << response.body.size() << "\r\n";
    os << "Connection: close\r\n\r\n";
    os << response.body;

    const std::string raw_response = os.str();
    const char* data = raw_response.data();
    size_t remaining = raw_response.size();

    while (remaining > 0) {
        const ssize_t sent = send(client_fd, data, remaining, 0);
        if (sent <= 0) {
            return;
        }

        data += sent;
        remaining -= static_cast<size_t>(sent);
    }
}

std::string WebServer::BuildStatusText(int status_code) {
    switch (status_code) {
        case 200:
            return "OK";
        case 400:
            return "Bad Request";
        case 404:
            return "Not Found";
        case 500:
            return "Internal Server Error";
        default:
            return "OK";
    }
}

std::filesystem::path WebServer::GetTaskHistoryPath() const {
    std::error_code ec;
    const std::filesystem::path temp_dir = std::filesystem::temp_directory_path(ec);
    if (!ec) {
        return temp_dir / "bazel-deps-checker-ui-tasks.json";
    }
    return std::filesystem::path("bazel-deps-checker-ui-tasks.json");
}

std::filesystem::path WebServer::GetTaskResultsDirectory() const {
    std::error_code ec;
    const std::filesystem::path temp_dir = std::filesystem::temp_directory_path(ec);
    if (!ec) {
        return temp_dir / "bazel-deps-checker-ui-task-results";
    }
    return std::filesystem::path("bazel-deps-checker-ui-task-results");
}

std::string WebServer::LoadTaskResultBodyLocked(const AnalysisTask& task) const {
    if (!task.response_body.empty()) {
        return task.response_body;
    }
    if (task.result_file_path.empty()) {
        return "";
    }

    try {
        std::ifstream input(task.result_file_path);
        if (!input) {
            return "";
        }
        std::ostringstream buffer;
        buffer << input.rdbuf();
        return buffer.str();
    } catch (const std::exception& error) {
        LOG_WARN("Failed to load task result body: " + std::string(error.what()));
        return "";
    }
}

void WebServer::SaveTaskResultBodyLocked(AnalysisTask& task, const std::string& response_body) const {
    if (response_body.empty()) {
        return;
    }

    task.response_body.clear();
    const std::filesystem::path results_dir = GetTaskResultsDirectory();
    std::error_code ec;
    std::filesystem::create_directories(results_dir, ec);
    if (ec) {
        LOG_WARN("Failed to create task results directory: " + results_dir.string());
        task.response_body = response_body;
        task.result_file_path.clear();
        return;
    }

    const std::filesystem::path result_path = results_dir / (task.id + ".json");
    try {
        std::ofstream output(result_path, std::ios::trunc);
        if (!output) {
            LOG_WARN("Failed to persist task result body to " + result_path.string());
            task.response_body = response_body;
            task.result_file_path.clear();
            return;
        }
        output << response_body;
        task.result_file_path = result_path.string();
    } catch (const std::exception& error) {
        LOG_WARN("Failed to persist task result body: " + std::string(error.what()));
        task.response_body = response_body;
        task.result_file_path.clear();
    }
}

void WebServer::DeleteTaskResultFileLocked(const AnalysisTask& task) const {
    if (task.result_file_path.empty()) {
        return;
    }

    std::error_code ec;
    std::filesystem::remove(task.result_file_path, ec);
}

void WebServer::LoadTaskHistory() {
    const std::filesystem::path history_path = GetTaskHistoryPath();
    std::error_code ec;
    if (!std::filesystem::exists(history_path, ec) || ec) {
        return;
    }

    try {
        std::ifstream input(history_path);
        if (!input) {
            return;
        }

        json payload;
        input >> payload;
        if (!payload.is_array()) {
            return;
        }

        std::lock_guard<std::mutex> lock(tasks_mutex_);
        unsigned long long next_id = 1;
        for (const auto& item : payload) {
            AnalysisTask task = TaskFromJson(item);
            if (task.id.empty()) {
                continue;
            }
            if (task.result_file_path.empty() && !task.response_body.empty()) {
                SaveTaskResultBodyLocked(task, task.response_body);
            }
            task.response_body.clear();
            tasks_[task.id] = std::move(task);

            const std::string prefix = "task-";
            if (tasks_[task.id].id.rfind(prefix, 0) == 0) {
                try {
                    const unsigned long long value =
                        std::stoull(tasks_[task.id].id.substr(prefix.size()));
                    next_id = std::max(next_id, value + 1);
                } catch (const std::exception&) {
                }
            }
        }
        next_task_id_.store(next_id);
        TrimTasksLocked();
        PersistTaskHistoryLocked();
    } catch (const std::exception& error) {
        LOG_WARN("Failed to load task history: " + std::string(error.what()));
    }
}

void WebServer::PersistTaskHistoryLocked() const {
    const std::filesystem::path history_path = GetTaskHistoryPath();
    json payload = json::array();

    std::vector<const AnalysisTask*> ordered_tasks;
    ordered_tasks.reserve(tasks_.size());
    for (const auto& [task_id, task] : tasks_) {
        (void)task_id;
        if (task.id.empty()) {
            continue;
        }
        ordered_tasks.push_back(&task);
    }

    std::sort(
        ordered_tasks.begin(),
        ordered_tasks.end(),
        [](const AnalysisTask* left, const AnalysisTask* right) {
            if (left->updated_at_ms != right->updated_at_ms) {
                return left->updated_at_ms > right->updated_at_ms;
            }
            return left->id > right->id;
        });

    for (const AnalysisTask* task : ordered_tasks) {
        payload.push_back(TaskToJson(*task));
    }

    try {
        std::ofstream output(history_path, std::ios::trunc);
        if (!output) {
            LOG_WARN("Failed to persist task history to " + history_path.string());
            return;
        }
        output << payload.dump(2);
    } catch (const std::exception& error) {
        LOG_WARN("Failed to persist task history: " + std::string(error.what()));
    }
}

void WebServer::TrimTasksLocked() const {
    if (tasks_.size() <= kMaxPersistedTasks) {
        return;
    }

    std::vector<std::pair<std::string, std::int64_t>> removable_tasks;
    removable_tasks.reserve(tasks_.size());
    for (const auto& [task_id, task] : tasks_) {
        if (task.status == "queued" || task.status == "running") {
            continue;
        }
        removable_tasks.emplace_back(task_id, task.updated_at_ms);
    }

    std::sort(
        removable_tasks.begin(),
        removable_tasks.end(),
        [](const auto& left, const auto& right) { return left.second < right.second; });

    size_t index = 0;
    while (tasks_.size() > kMaxPersistedTasks && index < removable_tasks.size()) {
        const auto it = tasks_.find(removable_tasks[index].first);
        if (it != tasks_.end()) {
            DeleteTaskResultFileLocked(it->second);
            tasks_.erase(it);
        }
        ++index;
    }
}
