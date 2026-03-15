# Changelog

## Unreleased

### CLI and output unification

- Reworked command-line parsing to use explicit value-based parsing instead of global singleton access
- Unified help / error handling at the entry layer
- Standardized output behavior for `cycle`, `unused`, and `build-time`
- Added shared output-file / output-format semantics for build-time analysis

### Web UI productization

- Added a local interactive Web UI with async background task execution
- Added recent task history with persisted task metadata
- Added task filtering, paging, reopen, rerun, and config reapply flows
- Added recent workspaces, favorite workspaces, and remembered UI preferences
- Added analysis presets with one-click run and apply actions
- Added task detail drawer with mode-specific result summaries
- Added compare view for current result vs baseline task
- Added in-drawer trend comparison against the latest comparable successful task
- Added task snapshot export as JSON / Markdown
- Added clipboard copy for task summaries

### Performance

- Added response-cache reuse for repeated UI requests
- Added cross-mode warm-up for `cycle` and `unused`
- Added workspace dependency-context reuse
- Added parser workspace cache invalidation based on key Bazel files
- Optimized dependency graph traversal, cycle detection, and source-analysis caches
- Updated benchmark workflow to use repeated samples with median / mean reporting

### Documentation

- Expanded README with productized Web UI behavior and workflow guidance
- Added task drawer, presets, compare, snapshot, and FAQ documentation
- Added `PERF_NOTES.md` for performance-layer explanation
- Added `ROADMAP.md` for product priorities and non-goals

### Validation

- Repeatedly validated with full `clang++ -std=c++17 ...` builds
- Verified local Web UI startup and smoke-checked key user-facing flows
