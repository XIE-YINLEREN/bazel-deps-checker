# Performance Notes

## Overview

This project now has several optimization layers across CLI execution, Web UI task orchestration,
dependency graph analysis, and source-level include analysis.

The goal of this note is to make future performance work easier to reason about and easier to
measure consistently.

## Current Fast Paths

- **Web response cache**
  - Keyed by `workspace + bazel + mode + include_tests`
  - Used by `/api/analyze`
  - Cache hits can return immediately without background task creation

- **Cross-mode warm-up**
  - Running `cycle` or `unused` computes both reports together
  - Warm `unused` after `cycle` should usually be much faster than a cold run

- **Workspace dependency-context cache**
  - Reuses parsed Bazel targets, `DependencyGraph`, and `CycleDetector`
  - Invalidated by workspace fingerprint changes

- **Parser workspace cache**
  - Reuses parsed Bazel query results
  - Invalidated by `WORKSPACE` / `WORKSPACE.bazel` / `MODULE.bazel` / `BUILD*`

- **DependencyGraph optimizations**
  - Reverse dependency cache
  - Transitive dependency cache
  - Direct edge adjacency set
  - Internal node-id indexing for transitive traversal
  - SCC prefiltering before cycle DFS
  - Small-SCC fast path for self-cycle and 2-node cycle cases

- **CycleDetector optimizations**
  - Cached cycle analysis results
  - Cached unused dependency results
  - Cached edge-level code analysis
  - Cached edge-level target analysis
  - Cached critical dependency checks

- **SourceAnalyzer optimizations**
  - Parsed include cache per file
  - Recursive header include closure cache
  - Dependency-needed cache per `target -> dependency`
  - Removable dependencies cache per target
  - Reverse index: `provided_header -> targets`
  - Reduced retained `TargetAnalysis` payload to only query-relevant sets

- **Task persistence optimizations**
  - Task index and full task results are stored separately
  - `/api/tasks/<task_id>` returns lightweight metadata by default
  - Full result is only loaded on demand with `?include_result=1`

## Measurement Guidance

Use the benchmark script instead of single manual runs:

```bash
SAMPLES=3 bash scripts/benchmark_cache.sh
```

The script now:

- compiles the binary
- starts the Web UI locally
- clears caches before each sample
- measures `cold cycle`
- measures `warm unused` after a `cycle` warm-up
- reports per-sample timings
- reports median and mean

Median is the preferred comparison number for local development.

## Interpreting Results

- **Cold cycle**
  - Includes Bazel-side setup, parsing, graph prep, and report generation
  - Usually dominated by dependency preparation and external process cost

- **Warm unused**
  - Mostly reflects cache effectiveness and residual analysis overhead
  - Useful for validating graph/source-analysis optimizations

If `warm unused` regresses a little but `cold cycle` improves, the change may still be valuable.
If both regress, inspect the latest cache-related or graph-analysis changes first.

## Known Bottlenecks

- Bazel invocation / dependency preparation still dominates cold path latency
- First-time `SourceAnalyzer::AnalyzeTarget()` remains more expensive than later cached lookups
- End-to-end timings on small workspaces can be noisy; differences below ~10–20 ms should be treated cautiously

## Recommended Next Steps

Highest-value next investigations:

1. Further reduce first-time `SourceAnalyzer` target analysis cost
2. Move more cycle enumeration internals onto node-id structures
3. Add optional targeted micro-benchmarks for parser / graph / source-analysis phases
