# bazel-deps-checker / Bazel依赖检查器

**正在缓慢的开发中**
**It is currently in the process of being developed slowly.**

**一个用于分析Bazel依赖关系的工具，检查不必要的依赖和循环依赖。目前支持C++的target。在Ubuntu 22.04环境下开发与测试。**
**A tool to analyze Bazel dependencies, checking for unnecessary and circular dependencies. Currently supports C++ targets. Developed and tested on Ubuntu 22.04.**  

---

## 功能亮点 / Key Features

- **检查未使用的依赖** - 识别声明但未实际使用的依赖  
  **Check unused dependencies** - Identify declared but unused dependencies
- **检测循环依赖** - 发现可能导致构建失败的循环依赖链  
  **Detect circular dependencies** - Find dependency cycles that may break builds
- **多格式报告输出** - 支持控制台、Markdown、JSON和HTML格式  
  **Multi-format reports** - Console, Markdown, JSON and HTML outputs
- **本地 Web 控制台** - 提供可交互前端页面来配置并触发分析  
  **Local Web console** - Interactive frontend page for configuring and running analyses
- **测试目标分析** - 可选包含测试目标进行依赖检查  
  **Test target analysis** - Optionally include test targets in dependency checks
- **Ubuntu 22.04支持** - 在Ubuntu 22.04环境开发和测试  
  **Ubuntu 22.04 support** - Developed and tested on Ubuntu 22.04

---

## 使用示例 / Usage Examples

```bash
# 基础用法：分析工作区依赖
bazel-deps-analyzer -w /path/to/workspace

# 生成JSON格式报告
bazel-deps-analyzer -w . --unused -f json -o unused.json

# 输出Markdown报告并包含测试目标
bazel-deps-analyzer -w . -o report.md -f markdown -t

# 分析构建耗时并输出为 JSON
bazel-deps-analyzer -w . -T -f json -o build-time.json

# 生成可直接打开的前端 HTML 报告页
bazel-deps-analyzer -w . --unused -f html -o unused-report.html

# 启动本地 Web UI（可在页面里填写 workspace）
bazel-deps-analyzer --ui --port 8080

# 启动本地 Web UI，并预置 workspace 默认值
bazel-deps-analyzer -w . --ui
```

---

## Web UI 功能 / Web UI Features

- **异步分析任务**  
  点击“运行分析”后，前端会立即返回任务状态，后台异步执行分析，避免浏览器长时间卡住。  
  **Async analysis jobs**  
  The UI queues analysis in the background and polls task status instead of blocking the browser.

- **多视图结果展示**  
  支持摘要视图、HTML 报告预览、原始 JSON 三种结果展示方式。  
  **Multiple result views**  
  Supports summary view, HTML preview, and raw JSON output.

- **缓存管理面板**  
  页面可直接查看响应缓存、依赖上下文缓存、parser 缓存和任务数量，并支持一键清空。  
  **Cache management panel**  
  The UI shows response-cache, dependency-context, parser-cache, and task counts, and can clear all caches.

- **性能观测面板**  
  每次分析返回依赖准备、分析执行、报告渲染和总耗时，方便定位慢点。  
  **Performance telemetry**  
  Each run reports dependency-prepare, analysis, render, and total timings.

- **最近任务历史**  
  最近任务会显示模式、workspace、更新时间和总耗时，并支持查看详情、载入配置、重新运行、重新打开已完成结果。  
  **Recent task history**  
  Recent tasks show mode, workspace, update time, and total duration, and support opening details, reapplying config, rerunning, and reopening completed results.

- **任务筛选与分页**  
  最近任务支持按关键字、模式、状态筛选，并可逐页加载更多历史记录。  
  **Task filtering and paging**  
  Recent tasks support keyword, mode, and status filters, plus incremental history loading.

- **任务详情抽屉**  
  点击任务的“详情”后，会在右侧滑出抽屉，集中展示任务配置、状态、时间信息、结果摘要与快捷动作。  
  **Task detail drawer**  
  Clicking a task’s “详情” opens a right-side drawer with config, status, timestamps, result summary, and quick actions.

- **抽屉内趋势对比**  
  对于已完成任务，抽屉会自动寻找同 workspace、同模式的最近成功任务作为基线，直接显示“较上次”的变化。  
  **In-drawer trend comparison**  
  For completed tasks, the drawer automatically compares against the most recent successful task with the same workspace and mode.

- **分析预设 / Analysis presets**  
  可将当前 `workspace / bazel / mode / tests / force_refresh` 保存为预设，并支持一键运行或载入。  
  **Analysis presets**  
  Save the current `workspace / bazel / mode / tests / force_refresh` as presets, then run or apply them in one click.

- **收藏工作区与最近工作区**  
  Web UI 会记住最近工作区，并支持收藏常用 workspace，减少频繁切换项目时的输入成本。  
  **Favorite and recent workspaces**  
  The Web UI remembers recent workspaces and lets you pin favorite ones for faster switching.

- **跨重启任务保留**  
  最近任务会持久化到系统临时目录，重启 Web UI 后仍可继续查看历史结果。  
  **Restart-safe task persistence**  
  Recent tasks are persisted in the system temp directory and remain available after UI restarts.

- **最近工作区与设置记忆**  
  Web UI 会记住最近使用过的 workspace，以及常用的 Bazel 路径、模式和开关状态。  
  **Recent workspaces and remembered settings**  
  The Web UI remembers recent workspaces plus commonly used Bazel path, mode, and toggle settings.

- **任务快照导出 / Task snapshot export**  
  在任务详情抽屉里，可以复制 Markdown 摘要，或导出当前任务快照为 JSON / Markdown，便于同步、汇报或留档。  
  **Task snapshot export**  
  From the task drawer, you can copy a Markdown summary or export a task snapshot as JSON / Markdown for sharing and reporting.

---

## Web UI 推荐工作流 / Recommended Web UI Workflow

1. **填写 workspace 并选择模式**  
   初次进入时，可先输入 workspace，选择 `cycle`、`unused` 或 `build-time`。

2. **保存常用分析为预设**  
   对固定项目或固定分析套路，点击“保存当前为预设”，后续可直接“一键运行”。

3. **查看最近任务并优先处理异常项**  
   “最近任务”会把运行中、排队中、失败任务放到“优先关注”，便于快速闭环。

4. **点击“详情”打开任务抽屉**  
   在抽屉里查看任务配置、时间信息、结果摘要和趋势变化。

5. **必要时切到“结果对比”或导出快照**  
   如果需要更完整对比，切到主结果区的“结果对比”；如果需要同步给他人，可直接在抽屉里复制摘要或导出快照。

---

## 任务详情抽屉 / Task Drawer

- **基础信息**  
  显示 `task_id`、模式、workspace、Bazel 路径、tests 开关、缓存命中、状态和时间信息。

- **模式摘要**  
  - `cycle`：循环数、最短环长度、结构风险提示  
  - `unused`：未使用依赖总数、高置信度数量、中低置信度汇总  
  - `build-time`：总耗时、最慢 phase、优化建议数量

- **趋势对比**  
  自动展示当前任务相较最近同 workspace / 同模式成功任务的变化。

- **快捷动作**  
  支持载入配置、重新运行、打开结果、查看对比、复制摘要、导出快照 JSON / Markdown。

---

## 典型使用场景 / Common Usage Scenarios

- **日常循环依赖治理**  
  先运行 `cycle` 模式，查看“摘要视图”里的循环数量，再打开最近任务的“详情”抽屉确认最短环和趋势变化。  
  **Routine cycle cleanup**  
  Run `cycle` first, inspect the summary count, then use the task drawer to verify shortest cycles and trend changes.

- **清理未使用依赖**  
  切换到 `unused` 模式，优先关注高置信度项；如果同一 workspace 之前跑过，抽屉里的趋势对比能快速判断清理是否见效。  
  **Unused dependency cleanup**  
  Switch to `unused`, prioritize high-confidence items, and use drawer trends to confirm whether cleanup has improved the workspace.

- **排查构建耗时回归**  
  使用 `build-time` 模式，查看最慢 phase、优化建议和趋势对比；如果耗时明显变慢，可继续跳到“结果对比”页看更完整的基线差异。  
  **Build-time regression triage**  
  Use `build-time` mode to inspect the hottest phase, optimization hints, and trend cards; jump to the compare view when a regression looks real.

- **固定项目的重复分析**  
  将常用 workspace + 模式保存为预设，日后直接“一键运行”，避免重复填写参数。  
  **Repeat analysis for stable projects**  
  Save frequently used workspace + mode combinations as presets and rerun them with one click.

- **同步结果给同事或写日报**  
  在任务详情抽屉中直接“复制摘要”或“导出快照 Markdown / JSON”，避免手动整理截图和原始输出。  
  **Sharing results with teammates**  
  Use “复制摘要” or snapshot export from the task drawer instead of manually reformatting screenshots and raw output.

---

## FAQ / 常见问题

- **为什么点击“运行分析”后没有立即出结果？**  
  Web UI 默认使用后台异步任务，页面会先返回任务状态，再轮询结果，避免长时间阻塞浏览器。

- **为什么有时会命中缓存？**  
  相同 `workspace + bazel + mode + include_tests` 请求会优先复用已有结果；如需强制重跑，请打开“强制重新分析”。

- **为什么“结果对比”或“趋势对比”里没有基线？**  
  只有当历史里存在“同 workspace + 同模式”的成功任务时，系统才会自动选出一个基线进行比较。

- **任务历史会不会在重启后丢失？**  
  不会。最近任务会持久化到系统临时目录，重启 Web UI 后仍然可见，但它不是长期归档系统。

- **什么时候应该用预设，什么时候应该直接重跑历史任务？**  
  如果你关注的是固定分析套路，用预设更合适；如果你关注的是某次具体任务配置和结果上下文，用历史任务详情或“重新运行”更合适。

- **快照导出适合什么场景？**  
  Markdown 更适合发给同事、贴到文档或日报；JSON 更适合后续脚本处理或保留完整结果结构。

---

## 缓存与性能策略 / Cache and Performance Strategy

- **结果缓存 / Response cache**  
  相同 `workspace + bazel + mode + include_tests` 请求会优先直接返回已有结果。  
  Reuses prior results for identical request parameters.

- **跨模式联动缓存 / Cross-mode warm-up**  
  `cycle` 与 `unused` 任一模式首次执行时，会一起生成两类报告，后续切换模式更快。  
  Running either `cycle` or `unused` seeds both reports for faster mode switching.

- **workspace 依赖上下文缓存 / Workspace dependency-context cache**  
  会复用已解析的 Bazel targets、`DependencyGraph` 和 `CycleDetector`。  
  Reuses parsed targets, dependency graph, and cycle detector for the same workspace.

- **parser 层缓存 / Parser-level cache**  
  会缓存 Bazel query 解析结果，并基于 `WORKSPACE` / `MODULE.bazel` / `BUILD*` 文件变更自动失效。  
  Caches parsed Bazel query results and invalidates automatically when key workspace files change.

- **源码分析缓存 / Source-analysis cache**  
  会缓存头文件与源文件的 include 解析结果，减少重复文件扫描。  
  Caches include parsing results to reduce repeated file scans.

---

## Web UI API（本地） / Local Web UI API

在 `--ui` 模式下，程序会启动本地 HTTP 服务，主要接口如下：  
When running with `--ui`, the app starts a local HTTP service with these main endpoints:

- `GET /`  
  返回 Web UI 页面。  
  Returns the interactive Web UI page.

- `GET /api/health`  
  返回服务健康状态。  
  Returns service health information.

- `POST /api/analyze`  
  提交分析任务；若未命中缓存，会返回后台任务 `task_id`。  
  Submits an analysis job and returns a background `task_id` when needed.

- `GET /api/tasks/<task_id>`  
  查询后台任务状态；默认返回轻量摘要，追加 `?include_result=1` 时返回完整结果。  
  Polls background task status; returns lightweight metadata by default, and full results with `?include_result=1`.

- `GET /api/tasks`  
  返回最近任务列表（按最近更新时间倒序），支持 `limit`、`offset`、`mode`、`status`、`q` 查询参数。  
  Returns recent tasks in reverse updated-time order, with `limit`, `offset`, `mode`, `status`, and `q` query parameters.

- `GET /api/cache`  
  查看缓存状态。  
  Returns cache statistics.

- `POST /api/cache/clear`  
  清空全部缓存。  
  Clears all caches.

---

## 验证脚本 / Validation Scripts

- `scripts/smoke_ui.sh`  
  编译程序、启动本地 Web UI、检查 `/api/health`，并提交一次分析任务后轮询直到完成。  
  Compiles the app, starts the local Web UI, checks `/api/health`, and polls one analysis task until completion.

- `scripts/benchmark_cache.sh`  
  编译程序并启动本地 Web UI；对冷启动 `cycle` 和 warm `unused` 做多次采样，输出 sample、median 和 mean。  
  Compiles the app and starts the local Web UI; runs repeated samples for cold `cycle` and warm `unused`, then reports sample timings, median, and mean.

示例：  
Examples:

```bash
bash scripts/smoke_ui.sh
bash scripts/benchmark_cache.sh
SAMPLES=5 bash scripts/benchmark_cache.sh
```

---

## 性能备注 / Performance Notes

- 更完整的性能分层、缓存策略和测量说明见 `/Users/xie/Desktop/bazel-deps-checker/PERF_NOTES.md`。  
  See `/Users/xie/Desktop/bazel-deps-checker/PERF_NOTES.md` for a fuller explanation of the current cache layers, performance strategy, and measurement guidance.

## 路线图 / Roadmap

- 产品化方向、近期优先级和非目标说明见 `/Users/xie/Desktop/bazel-deps-checker/ROADMAP.md`。  
  See `/Users/xie/Desktop/bazel-deps-checker/ROADMAP.md` for productization priorities, near-term milestones, and explicit non-goals.

## 变更记录 / Changelog

- 最近一轮 CLI、Web UI、性能和文档收口记录见 `/Users/xie/Desktop/bazel-deps-checker/CHANGELOG.md`。  
  See `/Users/xie/Desktop/bazel-deps-checker/CHANGELOG.md` for the latest consolidated CLI, Web UI, performance, and documentation changes.
