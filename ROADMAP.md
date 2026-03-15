# Product Roadmap

## Current State

The project now has three relatively complete layers:

1. **CLI layer**
   - Unified argument parsing and predictable exit behavior
   - Shared output semantics for `cycle`, `unused`, and `build-time`
   - Console / Markdown / JSON / HTML output support

2. **Web UI layer**
   - Local interactive control panel
   - Background task execution and polling
   - Recent tasks, task drawer, compare view, presets, and snapshot export

3. **Performance layer**
   - Response cache
   - Workspace dependency-context cache
   - Parser / graph / source-analysis caches
   - Repeated-sample benchmark workflow

This means the project is already beyond “demo” stage and is closer to a usable local engineering tool.

---

## Near-Term Priorities

### 1. Product polish

Goal: make the tool easier to use repeatedly in day-to-day engineering work.

- Refine the task drawer information hierarchy
- Improve comparison readability for large reports
- Make preset management more ergonomic
- Reduce UI friction for common workflows

### 2. Performance follow-up

Goal: continue improving first-run latency and large-workspace responsiveness.

- Reduce first-time source analysis cost
- Lower build-time mode cold-start overhead
- Add more phase-level measurements where useful
- Keep warm-path performance stable while evolving features

### 3. Documentation and onboarding

Goal: make the tool understandable without reading the code.

- Keep README aligned with shipped UI behavior
- Add more operator-facing examples
- Expand validation / benchmark guidance
- Document limitations and expected tradeoffs clearly

---

## Mid-Term Directions

### A. Better result workflows

- More detailed result diffing inside the task drawer
- Sharable task snapshots with stronger formatting
- Better report handoff for team communication

### B. Better task management

- Stronger grouping for related tasks
- More explicit pinned / favorite workflows
- Faster navigation across repeated analyses

### C. Better observability

- Clearer breakdown of cold vs warm paths
- Better attribution for expensive phases
- More visible cache-hit / cache-miss explanations

---

## Non-Goals For Now

To keep the project moving without too much churn, these are intentionally not prioritized right now:

- A separate frontend build system or external web framework
- A large backend rewrite for task orchestration
- A full persistent database for task history
- Broad language-support expansion beyond the current focus
- Heavy packaging / deployment work for remote multi-user hosting

---

## Release Readiness Checklist

Before calling a milestone “stable enough” for regular use, aim to keep these true:

- Main CLI flows compile cleanly
- `--ui` mode starts reliably
- Task creation / polling / reopen flows still work
- Compare view still matches task history behavior
- Snapshot export remains usable
- README stays aligned with shipped behavior
- Benchmark script still works and reports median / mean

---

## Suggested Next Milestones

### Milestone P1 — Daily usability

- Make the task drawer even more actionable
- Improve compare readability
- Tighten copy and affordances in the Web UI

### Milestone P2 — Performance confidence

- Add more targeted benchmark coverage
- Continue reducing cold-start analysis cost
- Track regressions more explicitly

### Milestone P3 — Product maturity

- Improve long-term maintainability of the embedded UI
- Strengthen user-facing docs and examples
- Decide what should remain local-only vs become more shareable
