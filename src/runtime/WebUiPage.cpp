#include "WebServer.h"

std::string WebServer::BuildUiPage() {
    return std::string(R"HTML(<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Bazel Deps Checker UI</title>
  <style>
    :root {
      color-scheme: dark;
      --bg: #07101f;
      --bg-soft: #0d1729;
      --panel: rgba(18, 26, 43, 0.82);
      --panel-solid: #121a2b;
      --panel-soft: #18233a;
      --panel-strong: #10192c;
      --text: #edf3ff;
      --text-soft: #c7d5f2;
      --muted: #92a5c9;
      --border: rgba(152, 180, 235, 0.16);
      --border-strong: rgba(152, 180, 235, 0.28);
      --accent: #78a8ff;
      --accent-strong: #4f8fff;
      --success: #34d399;
      --warning: #fbbf24;
      --danger: #f87171;
      --shadow: 0 20px 60px rgba(0, 0, 0, 0.28);
      --shadow-soft: 0 10px 24px rgba(0, 0, 0, 0.18);
      --radius-xl: 28px;
      --radius-lg: 22px;
      --radius-md: 18px;
      --radius-sm: 14px;
    }
    * { box-sizing: border-box; }
    html { scroll-behavior: smooth; }
    body {
      margin: 0;
      min-height: 100vh;
      font-family: Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      color: var(--text);
      background:
        radial-gradient(circle at top left, rgba(120, 168, 255, 0.18), transparent 30%),
        radial-gradient(circle at top right, rgba(52, 211, 153, 0.08), transparent 24%),
        linear-gradient(180deg, var(--bg) 0%, #0b1324 52%, #111a2c 100%);
    }
    body::before {
      content: "";
      position: fixed;
      inset: 0;
      pointer-events: none;
      background-image:
        linear-gradient(rgba(255,255,255,.018) 1px, transparent 1px),
        linear-gradient(90deg, rgba(255,255,255,.018) 1px, transparent 1px);
      background-size: 28px 28px;
      mask-image: linear-gradient(180deg, rgba(0,0,0,.38), transparent 88%);
    }
    .page {
      position: relative;
      max-width: 1380px;
      margin: 0 auto;
      padding: 28px 20px 56px;
    }
    .hero {
      position: relative;
      overflow: hidden;
      padding: 32px;
      border-radius: var(--radius-xl);
      border: 1px solid var(--border);
      background:
        linear-gradient(135deg, rgba(120, 168, 255, 0.18), rgba(18, 26, 43, 0.96) 42%),
        rgba(18, 26, 43, 0.92);
      box-shadow: var(--shadow);
      margin-bottom: 20px;
    }
    .hero::after {
      content: "";
      position: absolute;
      right: -80px;
      top: -80px;
      width: 240px;
      height: 240px;
      border-radius: 50%;
      background: radial-gradient(circle, rgba(120, 168, 255, 0.28), transparent 70%);
      pointer-events: none;
    }
    .hero-top {
      display: flex;
      justify-content: space-between;
      gap: 18px;
      align-items: flex-start;
      flex-wrap: wrap;
    }
    .hero h1 {
      margin: 0 0 10px;
      font-size: clamp(30px, 4vw, 40px);
      line-height: 1.12;
      letter-spacing: -.03em;
    }
    .hero p {
      margin: 0;
      max-width: 760px;
      color: var(--text-soft);
      line-height: 1.7;
      font-size: 15px;
    }
    .hero-badges, .hero-stats {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
    }
    .hero-badges { margin-top: 18px; }
    .badge, .mini-chip {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      padding: 8px 12px;
      border-radius: 999px;
      border: 1px solid rgba(255,255,255,.08);
      background: rgba(255,255,255,.05);
      color: var(--text);
      font-size: 13px;
      white-space: nowrap;
    }
    .mini-chip {
      padding: 6px 10px;
      color: var(--muted);
      background: rgba(255,255,255,.03);
    }
    .hero-stats {
      justify-content: flex-end;
      max-width: 320px;
    }
    .hero-stat {
      min-width: 140px;
      padding: 14px 16px;
      border-radius: 18px;
      border: 1px solid rgba(255,255,255,.08);
      background: rgba(255,255,255,.04);
      box-shadow: var(--shadow-soft);
    }
    .hero-stat .label {
      font-size: 12px;
      color: var(--muted);
      text-transform: uppercase;
      letter-spacing: .08em;
    }
    .hero-stat .value {
      margin-top: 8px;
      font-size: 22px;
      font-weight: 800;
    }
    .layout {
      display: grid;
      grid-template-columns: minmax(320px, 420px) minmax(0, 1fr);
      gap: 20px;
      align-items: start;
    }
    .panel {
      border-radius: var(--radius-lg);
      border: 1px solid var(--border);
      background: var(--panel);
      backdrop-filter: blur(10px);
      box-shadow: var(--shadow-soft);
    }
    .control-panel {
      position: sticky;
      top: 18px;
      padding: 22px;
    }
    .results-panel {
      position: relative;
      padding: 22px;
      min-height: 760px;
    }
    .section-title {
      display: flex;
      justify-content: space-between;
      align-items: flex-start;
      gap: 16px;
      flex-wrap: wrap;
      margin-bottom: 8px;
    }
    .section-title h2 {
      margin: 0;
      font-size: 22px;
    }
    .section-title p {
      margin: 6px 0 0;
      color: var(--muted);
      line-height: 1.6;
      max-width: 640px;
    }
    .form-grid {
      display: grid;
      gap: 16px;
      margin-top: 20px;
    }
    .input-group {
      display: grid;
      gap: 8px;
    }
    .field-label {
      display: flex;
      justify-content: space-between;
      gap: 12px;
      align-items: center;
      font-size: 14px;
      color: var(--text-soft);
    }
    .field-tip {
      color: var(--muted);
      font-size: 12px;
    }
    input, select, button {
      font: inherit;
      border-radius: var(--radius-sm);
      border: 1px solid var(--border);
      background: var(--panel-strong);
      color: var(--text);
      padding: 13px 14px;
      transition: border-color .18s ease, box-shadow .18s ease, transform .18s ease, background .18s ease;
    }
    input::placeholder { color: #6f84aa; }
    input:focus, select:focus {
      outline: none;
      border-color: var(--accent);
      box-shadow: 0 0 0 4px rgba(120, 168, 255, 0.15);
    }
    .mode-grid {
      display: grid;
      grid-template-columns: repeat(3, minmax(0, 1fr));
      gap: 10px;
    }
    .mode-card {
      position: relative;
      padding: 14px 12px;
      border-radius: 16px;
      border: 1px solid var(--border);
      background: rgba(255,255,255,.035);
      color: var(--text-soft);
      cursor: pointer;
      text-align: left;
    }
    .mode-card strong {
      display: block;
      color: var(--text);
      margin-bottom: 4px;
      font-size: 14px;
    }
    .mode-card span {
      display: block;
      font-size: 12px;
      line-height: 1.5;
      color: var(--muted);
    }
    .mode-card.active {
      border-color: rgba(120, 168, 255, 0.46);
      background: linear-gradient(180deg, rgba(120, 168, 255, 0.18), rgba(255,255,255,.05));
      box-shadow: inset 0 0 0 1px rgba(120, 168, 255, 0.18);
    }
    .toggle-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 14px;
      padding: 14px 16px;
      border: 1px solid var(--border);
      border-radius: 16px;
      background: rgba(255,255,255,.03);
    }
    .toggle-copy strong {
      display: block;
      font-size: 14px;
    }
    .toggle-copy span {
      display: block;
      margin-top: 4px;
      color: var(--muted);
      font-size: 12px;
      line-height: 1.5;
    }
    .switch {
      position: relative;
      width: 52px;
      height: 30px;
      flex-shrink: 0;
    }
    .switch input {
      position: absolute;
      inset: 0;
      opacity: 0;
      cursor: pointer;
      margin: 0;
      padding: 0;
    }
    .switch-track {
      position: absolute;
      inset: 0;
      border-radius: 999px;
      background: rgba(255,255,255,.12);
      border: 1px solid rgba(255,255,255,.1);
      transition: background .18s ease;
    }
    .switch-track::after {
      content: "";
      position: absolute;
      top: 3px;
      left: 3px;
      width: 22px;
      height: 22px;
      border-radius: 50%;
      background: white;
      transition: transform .18s ease;
      box-shadow: 0 4px 10px rgba(0,0,0,.24);
    }
    .switch input:checked + .switch-track {
      background: linear-gradient(135deg, var(--accent), var(--accent-strong));
    }
    .switch input:checked + .switch-track::after {
      transform: translateX(22px);
    }
    .action-row {
      display: flex;
      gap: 10px;
      flex-wrap: wrap;
      margin-top: 6px;
    }
    button {
      cursor: pointer;
      font-weight: 700;
      border: 0;
    }
    .primary-button {
      flex: 1;
      min-width: 180px;
      background: linear-gradient(135deg, rgba(120,168,255,.98), rgba(79,143,255,.92));
      color: white;
      box-shadow: 0 12px 24px rgba(79, 143, 255, 0.28);
    }
    .secondary-button {
      background: rgba(255,255,255,.06);
      border: 1px solid var(--border);
      color: var(--text);
    }
    button:hover:not([disabled]) {
      transform: translateY(-1px);
    }
    button[disabled] {
      opacity: .72;
      cursor: wait;
      transform: none;
    }
    .status {
      display: flex;
      align-items: center;
      gap: 12px;
      margin-top: 18px;
      padding: 14px 16px;
      min-height: 58px;
      border-radius: 16px;
      border: 1px solid rgba(255,255,255,.06);
      background: rgba(255,255,255,.04);
      color: var(--text-soft);
    }
    .status-dot {
      width: 11px;
      height: 11px;
      border-radius: 50%;
      background: var(--muted);
      flex-shrink: 0;
      box-shadow: 0 0 0 4px rgba(255,255,255,.05);
    }
    .status.success {
      color: #d8ffed;
      border-color: rgba(52,211,153,.24);
      background: rgba(52,211,153,.09);
    }
    .status.success .status-dot {
      background: var(--success);
      box-shadow: 0 0 0 4px rgba(52,211,153,.14);
    }
    .status.error {
      color: #ffd8d8;
      border-color: rgba(248,113,113,.24);
      background: rgba(248,113,113,.09);
    }
    .status.error .status-dot {
      background: var(--danger);
      box-shadow: 0 0 0 4px rgba(248,113,113,.14);
    }
    .status.loading .status-dot {
      background: var(--accent);
      box-shadow: 0 0 0 4px rgba(120,168,255,.14);
      animation: pulse 1.3s infinite ease-in-out;
    }
    )HTML") + std::string(R"HTML(    .helper-grid {
      display: grid;
      grid-template-columns: repeat(2, minmax(0, 1fr));
      gap: 12px;
      margin-top: 16px;
    }
    .helper-card {
      padding: 14px 16px;
      border-radius: 16px;
      border: 1px solid var(--border);
      background: rgba(255,255,255,.03);
    }
    .helper-card strong {
      display: block;
      font-size: 13px;
      color: var(--text);
    }
    .helper-card span {
      display: block;
      margin-top: 6px;
      color: var(--muted);
      font-size: 12px;
      line-height: 1.5;
    }
    .results-head {
      display: flex;
      justify-content: space-between;
      align-items: flex-start;
      gap: 12px;
      flex-wrap: wrap;
      margin-bottom: 16px;
    }
    .results-meta {
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
      margin-top: 10px;
    }
    .toolbar {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
    }
    .toolbar button {
      padding: 10px 14px;
      border-radius: 999px;
      background: rgba(255,255,255,.05);
      border: 1px solid var(--border);
      color: var(--text-soft);
      font-weight: 700;
    }
    .toolbar button.active {
      color: white;
      background: linear-gradient(135deg, rgba(120,168,255,.95), rgba(79,143,255,.85));
      border-color: transparent;
      box-shadow: 0 10px 20px rgba(79, 143, 255, 0.22);
    }
    .results {
      display: grid;
      gap: 18px;
    }
    .empty-state {
      display: grid;
      place-items: center;
      min-height: 420px;
      border-radius: 20px;
      border: 1px dashed var(--border-strong);
      background: linear-gradient(180deg, rgba(255,255,255,.025), rgba(255,255,255,.015));
      padding: 24px;
      text-align: center;
    }
    .empty-state h3 {
      margin: 0 0 8px;
      font-size: 22px;
    }
    .empty-state p {
      margin: 0;
      max-width: 420px;
      color: var(--muted);
      line-height: 1.7;
    }
    .metric-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
      gap: 14px;
    }
    .metric-card {
      padding: 16px 18px;
      border-radius: 18px;
      border: 1px solid var(--border);
      background: linear-gradient(180deg, rgba(255,255,255,.05), rgba(255,255,255,.025));
    }
    .metric-card .label {
      color: var(--muted);
      font-size: 12px;
      letter-spacing: .08em;
      text-transform: uppercase;
    }
    .metric-card .value {
      margin-top: 10px;
      font-size: 28px;
      font-weight: 800;
      color: var(--text);
      word-break: break-word;
    }
    .summary-grid {
      display: grid;
      grid-template-columns: minmax(0, 1.2fr) minmax(320px, .8fr);
      gap: 16px;
      margin-top: 18px;
    }
    .summary-main, .summary-side {
      display: grid;
      gap: 14px;
    }
    .card {
      border-radius: 20px;
      border: 1px solid var(--border);
      background: rgba(255,255,255,.035);
      overflow: hidden;
    }
    .card-header {
      padding: 18px 18px 0;
    }
    .card-header h3 {
      margin: 0;
      font-size: 18px;
    }
    .card-header p {
      margin: 6px 0 0;
      color: var(--muted);
      line-height: 1.6;
      font-size: 13px;
    }
    .stack {
      display: grid;
      gap: 12px;
      padding: 18px;
    }
    .item {
      padding: 16px;
      border-radius: 18px;
      border: 1px solid rgba(255,255,255,.06);
      background: rgba(8,14,25,.34);
    }
    .item h3 {
      margin: 0 0 8px;
      font-size: 17px;
      line-height: 1.45;
    }
    .item p, .item li {
      color: var(--muted);
      line-height: 1.65;
    }
    .item p { margin: 0; }
    .item ul {
      margin: 10px 0 0;
      padding-left: 18px;
    }
    .item + .item { margin-top: 0; }
    .path-chip-list, .tag-list {
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
      margin-top: 12px;
    }
    .tag {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      padding: 7px 10px;
      border-radius: 999px;
      border: 1px solid rgba(255,255,255,.08);
      background: rgba(255,255,255,.04);
      color: var(--text-soft);
      font-size: 12px;
    }
    .tag.success { border-color: rgba(52,211,153,.24); color: #d8ffed; }
    .tag.warning { border-color: rgba(251,191,36,.24); color: #ffe7a9; }
    .tag.danger { border-color: rgba(248,113,113,.24); color: #ffd8d8; }
    .list-table {
      display: grid;
      gap: 10px;
      padding: 18px;
    }
    .list-row {
      display: grid;
      grid-template-columns: minmax(0, 1fr) auto;
      gap: 12px;
      align-items: center;
      padding: 14px 16px;
      border-radius: 16px;
      border: 1px solid rgba(255,255,255,.06);
      background: rgba(8,14,25,.3);
    }
    .list-row strong {
      display: block;
      font-size: 14px;
      color: var(--text);
      line-height: 1.5;
      word-break: break-word;
    }
    .list-row span {
      display: block;
      margin-top: 5px;
      color: var(--muted);
      font-size: 12px;
      line-height: 1.55;
    }
    .preview-shell {
      overflow: hidden;
      border-radius: 18px;
      border: 1px solid var(--border);
      background: rgba(255,255,255,.02);
    }
    iframe {
      width: 100%;
      min-height: 720px;
      border: 0;
      background: white;
    }
    pre {
      margin: 0;
      padding: 20px;
      white-space: pre-wrap;
      word-break: break-word;
      overflow: auto;
      border-radius: 18px;
      border: 1px solid var(--border);
      background: #0b1020;
      color: #dce9ff;
      line-height: 1.65;
      font-size: 13px;
    }
    .caption {
      color: var(--muted);
      font-size: 13px;
      line-height: 1.6;
    }
    .drawer-backdrop {
      position: absolute;
      inset: 0;
      background: rgba(2, 6, 14, 0.52);
      border-radius: inherit;
      opacity: 0;
      pointer-events: none;
      transition: opacity .18s ease;
    }
    .drawer-backdrop.visible {
      opacity: 1;
      pointer-events: auto;
    }
    .task-drawer {
      position: absolute;
      top: 14px;
      right: 14px;
      bottom: 14px;
      width: min(420px, calc(100% - 28px));
      border-radius: 22px;
      border: 1px solid var(--border-strong);
      background: rgba(10, 16, 28, 0.96);
      box-shadow: var(--shadow);
      transform: translateX(calc(100% + 24px));
      opacity: 0;
      pointer-events: none;
      transition: transform .22s ease, opacity .22s ease;
      display: grid;
      grid-template-rows: auto 1fr;
      overflow: hidden;
      z-index: 3;
    }
    .task-drawer.visible {
      transform: translateX(0);
      opacity: 1;
      pointer-events: auto;
    }
    .task-drawer-head {
      display: flex;
      justify-content: space-between;
      gap: 12px;
      align-items: flex-start;
      padding: 18px 18px 12px;
      border-bottom: 1px solid rgba(255,255,255,.06);
    }
    .task-drawer-head h3 {
      margin: 0;
      font-size: 18px;
    }
    .task-drawer-head p {
      margin: 6px 0 0;
      color: var(--muted);
      font-size: 13px;
      line-height: 1.6;
    }
    .drawer-close-button {
      min-width: auto;
      padding: 10px 12px;
    }
    .task-drawer-body {
      overflow: auto;
      padding: 18px;
      display: grid;
      gap: 14px;
      align-content: start;
    }
    @keyframes pulse {
      0%, 100% { transform: scale(1); opacity: 1; }
      50% { transform: scale(1.18); opacity: .85; }
    }
    @media (max-width: 1180px) {
      .summary-grid {
        grid-template-columns: 1fr;
      }
    }
    @media (max-width: 980px) {
      .layout {
        grid-template-columns: 1fr;
      }
      .control-panel {
        position: static;
      }
      .hero-top {
        flex-direction: column;
      }
      .hero-stats {
        justify-content: flex-start;
        max-width: none;
      }
    }
    @media (max-width: 720px) {
      .page {
        padding: 16px 14px 36px;
      }
      .hero, .control-panel, .results-panel {
        padding: 18px;
      }
      .mode-grid, .helper-grid {
        grid-template-columns: 1fr;
      }
      .toolbar, .action-row {
        width: 100%;
      }
      .toolbar button, .secondary-button, .primary-button {
        flex: 1 1 auto;
      }
      .list-row {
        grid-template-columns: 1fr;
      }
    }
  )HTML") + std::string(R"HTML(</style>
</head>
<body>
  <div class="page">
    <section class="hero">
      <div class="hero-top">
        <div>
          <h1>Bazel Deps Checker 控制台</h1>
          <p>这是一个真正可交互的本地前端页面：在浏览器里配置 workspace、切换分析模式、触发分析，并直接查看结构化结果、HTML 预览和原始 JSON。适合日常排查循环依赖、清理未使用依赖，以及定位 build-time 热点。</p>
          <div class="hero-badges">
            <span class="badge">Cycle / Unused / Build-time</span>
            <span class="badge">本地运行 · 无额外前端依赖</span>
            <span class="badge">统一 JSON / HTML 报告</span>
          </div>
        </div>
        <div class="hero-stats">
          <div class="hero-stat">
            <div class="label">UI Mode</div>
            <div class="value">Ready</div>
          </div>
          <div class="hero-stat">
            <div class="label">Output</div>
            <div class="value">Live Preview</div>
          </div>
        </div>
      </div>
    </section>

    <div class="layout">
      <section class="panel control-panel">
        <div class="section-title">
          <div>
            <h2>分析控制</h2>
            <p>配置工作区、Bazel 路径和分析模式。执行后结果会在右侧即时更新。</p>
          </div>
        </div>

        <form id="analysis-form" class="form-grid">
          <div class="input-group">
            <div class="field-label">
              <span>Workspace 路径</span>
              <span class="field-tip">必填</span>
            </div>
            <input id="workspace_path" name="workspace_path" placeholder="/path/to/workspace" autocomplete="off">
            <div id="recent-workspaces-panel" class="field-tip"></div>
          </div>

          <div class="input-group">
            <div class="field-label">
              <span>Bazel 路径</span>
              <span class="field-tip">默认 bazel</span>
            </div>
            <input id="bazel_binary" name="bazel_binary" value="bazel" placeholder="bazel" autocomplete="off">
          </div>

          <div class="input-group">
            <div class="field-label">
              <span>分析模式</span>
              <span class="field-tip">切换后右侧摘要自动适配</span>
            </div>
            <div class="mode-grid" id="mode-grid">
              <button class="mode-card active" type="button" data-mode="cycle">
                <strong>循环依赖</strong>
                <span>分析循环链路与修复建议</span>
              </button>
              <button class="mode-card" type="button" data-mode="unused">
                <strong>未使用依赖</strong>
                <span>发现可移除依赖与置信度</span>
              </button>
              <button class="mode-card" type="button" data-mode="build-time">
                <strong>构建耗时</strong>
                <span>查看阶段耗时与关键路径</span>
              </button>
            </div>
            <select id="mode" name="mode" hidden>
              <option value="cycle" selected>循环依赖</option>
              <option value="unused">未使用依赖</option>
              <option value="build-time">构建耗时</option>
            </select>
          </div>

          <div class="toggle-row">
            <div class="toggle-copy">
              <strong>包含测试目标</strong>
              <span>build-time 模式下仅作为附加配置保留，不改变主执行分支。</span>
            </div>
            <label class="switch" aria-label="包含测试目标">
              <input id="include_tests" name="include_tests" type="checkbox">
              <span class="switch-track"></span>
            </label>
          </div>

          <div class="toggle-row">
            <div class="toggle-copy">
              <strong>强制重新分析</strong>
              <span>关闭时，相同 workspace / mode / bazel / tests 参数会优先复用缓存结果，响应更快。</span>
            </div>
            <label class="switch" aria-label="强制重新分析">
              <input id="force_refresh" name="force_refresh" type="checkbox">
              <span class="switch-track"></span>
            </label>
          </div>

          <div class="action-row">
            <button id="run-button" class="primary-button" type="submit">运行分析</button>
            <button id="reset-button" class="secondary-button" type="button">重置表单</button>
          </div>
        </form>

        <div class="helper-card" style="margin-top:16px;">
          <strong>分析预设</strong>
          <span>把常用的 workspace / mode / bazel / tests 组合保存下来，减少重复输入。</span>
          <div class="action-row" style="margin-top:12px;">
            <button id="save-preset-button" class="secondary-button" type="button">保存当前为预设</button>
          </div>
          <div id="preset-panel" class="stack" style="padding:12px 0 0;"></div>
        </div>

        <div id="status" class="status">
          <span class="status-dot"></span>
          <span id="status-text">等待执行。</span>
        </div>

        <div class="helper-grid">
          <div class="helper-card">
            <strong>推荐顺序</strong>
            <span>先跑循环依赖，确认结构问题；再跑未使用依赖，最后看 build-time 热点。</span>
          </div>
          <div class="helper-card">
            <strong>结果视图</strong>
            <span>摘要适合日常浏览；HTML 预览适合汇报；JSON 适合继续处理或导出。</span>
          </div>
          <div id="cache-panel"></div>
          <div id="task-history-panel"></div>
        </div>
      </section>

      <section class="panel results-panel">
        <div class="results-head">
          <div>
            <div class="section-title">
              <div>
                <h2>结果展示</h2>
                <p>摘要视图会做结构化重排，HTML 保留完整报告样式，JSON 保留原始响应结构。</p>
              </div>
            </div>
            <div id="results-meta" class="results-meta">
              <span class="mini-chip">尚未执行分析</span>
            </div>
          </div>
          <div class="toolbar">
            <button id="show-summary" type="button" class="active">摘要视图</button>
            <button id="show-compare" type="button">结果对比</button>
            <button id="show-html" type="button">HTML 预览</button>
            <button id="show-json" type="button">原始 JSON</button>
          </div>
        </div>
        <div id="results" class="results"></div>
        <div id="task-drawer-backdrop" class="drawer-backdrop"></div>
        <aside id="task-drawer" class="task-drawer" aria-hidden="true">
          <div class="task-drawer-head">
            <div>
              <h3>任务详情</h3>
              <p>查看任务配置、状态、性能与快捷动作。</p>
            </div>
            <button id="close-task-drawer-button" class="secondary-button drawer-close-button" type="button">关闭</button>
          </div>
          <div id="task-drawer-body" class="task-drawer-body">
            <div class="item">
              <strong>尚未选择任务</strong>
              <span>点击历史任务中的“详情”查看完整信息。</span>
            </div>
          </div>
        </aside>
      </section>
    </div>
  </div>

  )HTML") + std::string(R"HTML(  <script>
    const statusEl = document.getElementById('status');
    const statusTextEl = document.getElementById('status-text');
    const resultsEl = document.getElementById('results');
    const resultsMetaEl = document.getElementById('results-meta');
    const formEl = document.getElementById('analysis-form');
    const runButton = document.getElementById('run-button');
    const resetButton = document.getElementById('reset-button');
    const modeSelectEl = document.getElementById('mode');
    const modeButtons = Array.from(document.querySelectorAll('.mode-card'));
    let activePollTimer = null;
    const toolbarButtons = {
      summary: document.getElementById('show-summary'),
      compare: document.getElementById('show-compare'),
      html: document.getElementById('show-html'),
      json: document.getElementById('show-json')
    };

    let latestPayload = null;
    let latestPayloadTaskId = '';
    let comparePayload = null;
    let latestCacheStatus = null;
    let latestTaskHistory = [];
    let latestTaskHistoryTotal = 0;
    let currentView = 'summary';
    let recentWorkspaces = [];
    let favoriteWorkspaces = [];
    let analysisPresets = [];
    let taskFilters = {
      q: '',
      mode: '',
      status: '',
      limit: 8,
      offset: 0
    };
    let taskSearchDebounceTimer = null;
    let activeTaskPollAttempts = 0;
    let preferencesReady = false;
    const workspaceInputEl = document.getElementById('workspace_path');
    const bazelInputEl = document.getElementById('bazel_binary');
    const includeTestsEl = document.getElementById('include_tests');
    const forceRefreshEl = document.getElementById('force_refresh');
    const recentWorkspacesPanelEl = document.getElementById('recent-workspaces-panel');
    const presetPanelEl = document.getElementById('preset-panel');
    const savePresetButtonEl = document.getElementById('save-preset-button');
    const taskDrawerEl = document.getElementById('task-drawer');
    const taskDrawerBodyEl = document.getElementById('task-drawer-body');
    const taskDrawerBackdropEl = document.getElementById('task-drawer-backdrop');
    const closeTaskDrawerButtonEl = document.getElementById('close-task-drawer-button');
    const preferencesStorageKey = 'bazel-deps-checker-ui-preferences-v1';
    let selectedTask = null;

    function escapeHtml(value) {
      return String(value ?? '')
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;')
        .replace(/'/g, '&#39;');
    }

    function normalizeModeLabel(mode) {
      if (mode === 'unused') return '未使用依赖';
      if (mode === 'build-time') return '构建耗时';
      return '循环依赖';
    }

    function formatDateTime(epochMs) {
      if (!epochMs) {
        return '未知时间';
      }
      const date = new Date(Number(epochMs));
      if (Number.isNaN(date.getTime())) {
        return '未知时间';
      }
      return date.toLocaleString('zh-CN', {
        year: 'numeric',
        month: '2-digit',
        day: '2-digit',
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit'
      });
    }

    function normalizeWorkspaceName(workspacePath) {
      const text = String(workspacePath || '').trim();
      if (!text) {
        return '未记录 workspace';
      }
      return text.split('/').filter(Boolean).pop() || text;
    }

    function taskStatusTone(status) {
      if (status === 'completed') return 'success';
      if (status === 'failed') return 'danger';
      return 'warning';
    }

    function summarizeTaskHistory(tasks) {
      const summary = {
        total: tasks.length,
        running: 0,
        failed: 0,
        completed: 0,
        cacheHit: 0,
        slowest: null
      };
      tasks.forEach((task) => {
        if (task.status === 'running' || task.status === 'queued') summary.running += 1;
        if (task.status === 'failed') summary.failed += 1;
        if (task.status === 'completed') summary.completed += 1;
        if (task.cache_hit) summary.cacheHit += 1;
        if (!summary.slowest || Number(task.total_ms || 0) > Number(summary.slowest.total_ms || 0)) {
          summary.slowest = task;
        }
      });
      return summary;
    }

    function sortTasksForDisplay(tasks) {
      const priority = { running: 0, queued: 1, failed: 2, completed: 3 };
      return tasks.slice().sort((left, right) => {
        const leftPriority = priority[left.status] ?? 9;
        const rightPriority = priority[right.status] ?? 9;
        if (leftPriority !== rightPriority) {
          return leftPriority - rightPriority;
        }
        return Number(right.updated_at_ms || 0) - Number(left.updated_at_ms || 0);
      });
    }

    function buildTaskItem(task) {
      return `
        <div class="item" style="padding:12px;">
          <strong>${escapeHtml(normalizeModeLabel(task.mode || 'cycle'))}</strong>
          <span>${escapeHtml(task.workspace_path || task.message || '未记录 workspace')}</span>
          <span class="field-tip">更新时间：${escapeHtml(formatDateTime(task.updated_at_ms))}${task.total_ms ? ` · 总耗时 ${escapeHtml(Number(task.total_ms).toFixed(2))}ms` : ''}</span>
          <div class="tag-list">
            <span class="tag">${escapeHtml(task.task_id)}</span>
            <span class="tag ${taskStatusTone(task.status)}">${escapeHtml(task.status)}</span>
            ${task.include_tests ? '<span class="tag">tests</span>' : ''}
            ${task.cache_hit ? '<span class="tag success">cache</span>' : ''}
            ${task.bazel_binary ? `<span class="tag">${escapeHtml(task.bazel_binary)}</span>` : ''}
          </div>
          <div class="action-row" style="margin-top:10px;">
            <button class="secondary-button task-detail-button" type="button" data-task-id="${escapeHtml(task.task_id)}">详情</button>
            <button class="secondary-button task-rerun-button" type="button" data-task-id="${escapeHtml(task.task_id)}">重新运行</button>
            <button class="secondary-button task-apply-button" type="button" data-task-id="${escapeHtml(task.task_id)}">载入配置</button>
            <button class="secondary-button task-open-button" type="button" data-task-id="${escapeHtml(task.task_id)}" ${task.status === 'completed' ? '' : 'disabled'}>打开结果</button>
          </div>
        </div>`;
    }

    function closeTaskDrawer() {
      selectedTask = null;
      taskDrawerEl.classList.remove('visible');
      taskDrawerBackdropEl.classList.remove('visible');
      taskDrawerEl.setAttribute('aria-hidden', 'true');
    }

    async function openTaskDrawer(taskId) {
      const task = latestTaskHistory.find((item) => item.task_id === taskId);
      if (!task) {
        setStatus('未找到任务详情。', 'error');
        return;
      }
      selectedTask = task;
      taskDrawerEl.classList.add('visible');
      taskDrawerBackdropEl.classList.add('visible');
      taskDrawerEl.setAttribute('aria-hidden', 'false');
      taskDrawerBodyEl.innerHTML = `
        <div class="item">
          <strong>正在加载任务详情…</strong>
          <span>${escapeHtml(task.task_id)}</span>
        </div>`;

      let resultPayload = null;
      let baselineTask = null;
      let baselinePayload = null;
      if (task.status === 'completed') {
        try {
          resultPayload = await fetchTaskResultPayload(task.task_id);
          baselineTask = findComparableTaskForTask(task, task.task_id);
          if (baselineTask) {
            baselinePayload = await fetchTaskResultPayload(baselineTask.task_id);
          }
        } catch (error) {
        }
      }

      const summaryHtml = renderTaskDrawerSummary(resultPayload);
      const trendHtml = renderTaskDrawerTrend(resultPayload, baselineTask, baselinePayload);

      taskDrawerBodyEl.innerHTML = `
        <div class="item">
          <strong>${escapeHtml(normalizeModeLabel(task.mode || 'cycle'))}</strong>
          <span>${escapeHtml(task.workspace_path || '未记录 workspace')}</span>
          <div class="tag-list">
            <span class="tag">${escapeHtml(task.task_id)}</span>
            <span class="tag ${taskStatusTone(task.status)}">${escapeHtml(task.status)}</span>
            ${task.include_tests ? '<span class="tag">tests</span>' : ''}
            ${task.cache_hit ? '<span class="tag success">cache</span>' : ''}
            ${task.bazel_binary ? `<span class="tag">${escapeHtml(task.bazel_binary)}</span>` : ''}
          </div>
        </div>
        <div class="item">
          <strong>时间信息</strong>
          <span>创建：${escapeHtml(formatDateTime(task.created_at_ms))}</span>
          <span>更新：${escapeHtml(formatDateTime(task.updated_at_ms))}</span>
          <span>${task.total_ms ? `总耗时：${escapeHtml(Number(task.total_ms).toFixed(2))}ms` : '总耗时：未记录'}</span>
        </div>
        ${summaryHtml}
        ${trendHtml}
        <div class="item">
          <strong>快捷动作</strong>
          <span>从这里可以直接复用配置、重新执行，或跳到主结果区查看详情与对比。</span>
          <div class="action-row" style="margin-top:12px;">
            <button class="secondary-button drawer-apply-button" type="button">载入配置</button>
            <button class="secondary-button drawer-rerun-button" type="button">重新运行</button>
            <button class="secondary-button drawer-open-button" type="button" ${task.status === 'completed' ? '' : 'disabled'}>打开结果</button>
            <button class="secondary-button drawer-compare-button" type="button" ${task.status === 'completed' ? '' : 'disabled'}>查看对比</button>
            <button class="secondary-button drawer-copy-button" type="button">复制摘要</button>
            <button class="secondary-button drawer-export-json-button" type="button">导出快照 JSON</button>
            <button class="secondary-button drawer-export-md-button" type="button">导出快照 Markdown</button>
          </div>
        </div>`;

      taskDrawerBodyEl.querySelector('.drawer-apply-button').addEventListener('click', () => {
        applyTaskToForm(task);
        setStatus(`已从任务详情载入配置：${task.task_id}`, 'success');
      });
      taskDrawerBodyEl.querySelector('.drawer-rerun-button').addEventListener('click', () => {
        rerunTask(task);
      });
      taskDrawerBodyEl.querySelector('.drawer-open-button')?.addEventListener('click', () => {
        openTaskResult(task.task_id);
      });
      taskDrawerBodyEl.querySelector('.drawer-compare-button')?.addEventListener('click', async () => {
        await openTaskResult(task.task_id);
        currentView = 'compare';
        refreshView();
      });
      taskDrawerBodyEl.querySelector('.drawer-copy-button').addEventListener('click', async () => {
        try {
          const markdown = buildTaskSnapshotMarkdown(task, resultPayload, baselineTask);
          await copyTextToClipboard(markdown);
          setStatus(`已复制任务摘要：${task.task_id}`, 'success');
        } catch (error) {
          setStatus(error && error.message ? error.message : '复制任务摘要失败', 'error');
        }
      });
      taskDrawerBodyEl.querySelector('.drawer-export-json-button').addEventListener('click', () => {
        const snapshot = buildTaskSnapshot(task, resultPayload, baselineTask);
        downloadTextFile(
          `${normalizeWorkspaceName(task.workspace_path)}-${task.task_id}-snapshot.json`,
          JSON.stringify(snapshot, null, 2),
          'application/json;charset=utf-8');
        setStatus(`已导出任务快照 JSON：${task.task_id}`, 'success');
      });
      taskDrawerBodyEl.querySelector('.drawer-export-md-button').addEventListener('click', () => {
        const markdown = buildTaskSnapshotMarkdown(task, resultPayload, baselineTask);
        downloadTextFile(
          `${normalizeWorkspaceName(task.workspace_path)}-${task.task_id}-snapshot.md`,
          markdown,
          'text/markdown;charset=utf-8');
        setStatus(`已导出任务快照 Markdown：${task.task_id}`, 'success');
      });
    }

    function renderTaskDrawerTrend(currentPayload, baselineTask, baselinePayload) {
      if (!currentPayload || !currentPayload.ok) {
        return '';
      }
      if (!baselineTask || !baselinePayload || !baselinePayload.ok) {
        return `
          <div class="item">
            <strong>趋势对比</strong>
            <span>当前没有找到同 workspace、同模式的历史成功任务作为基线。</span>
          </div>`;
      }

      let metricsHtml = '';
      let summaryText = '';
      if (currentPayload.mode === 'cycle') {
        const currentReport = currentPayload.report.report || {};
        const baselineReport = baselinePayload.report.report || {};
        metricsHtml = `
          ${buildTrendMetric('循环数', currentReport.total_cycles ?? 0, baselineReport.total_cycles ?? 0)}
          ${buildTrendMetric('分析总耗时(ms)', Number((currentPayload.performance || {}).total_ms || 0).toFixed(2), Number((baselinePayload.performance || {}).total_ms || 0).toFixed(2), 'ms')}`;
        const delta = Number(currentReport.total_cycles || 0) - Number(baselineReport.total_cycles || 0);
        summaryText = delta > 0 ? '循环风险较上次上升，建议优先查看新增环。'
          : delta < 0 ? '循环风险较上次下降，说明治理方向有效。'
          : '循环数与上次持平，可结合具体路径继续判断。';
      } else if (currentPayload.mode === 'unused') {
        const currentReport = currentPayload.report.unused_dependencies_report || {};
        const baselineReport = baselinePayload.report.unused_dependencies_report || {};
        const currentStats = currentReport.statistics || {};
        const baselineStats = baselineReport.statistics || {};
        metricsHtml = `
          ${buildTrendMetric('未使用依赖', currentReport.total_unused_dependencies ?? 0, baselineReport.total_unused_dependencies ?? 0)}
          ${buildTrendMetric('高置信度', currentStats.high_confidence ?? 0, baselineStats.high_confidence ?? 0)}
          ${buildTrendMetric('分析总耗时(ms)', Number((currentPayload.performance || {}).total_ms || 0).toFixed(2), Number((baselinePayload.performance || {}).total_ms || 0).toFixed(2), 'ms')}`;
        const delta = Number(currentReport.total_unused_dependencies || 0) - Number(baselineReport.total_unused_dependencies || 0);
        summaryText = delta > 0 ? '潜在可清理项比上次更多，适合安排一轮依赖治理。'
          : delta < 0 ? '未使用依赖较上次减少，清理已见效。'
          : '未使用依赖数量与上次接近，可重点看高置信度变化。';
      } else {
        const currentReport = currentPayload.report.build_time_report || {};
        const baselineReport = baselinePayload.report.build_time_report || {};
        const currentSummary = currentReport.summary || {};
        const baselineSummary = baselineReport.summary || {};
        metricsHtml = `
          ${buildTrendMetric('总耗时(s)', currentSummary.total_duration_seconds ?? 0, baselineSummary.total_duration_seconds ?? 0, 's')}
          ${buildTrendMetric('优化建议', (currentReport.suggestions || []).length, (baselineReport.suggestions || []).length)}
          ${buildTrendMetric('分析总耗时(ms)', Number((currentPayload.performance || {}).total_ms || 0).toFixed(2), Number((baselinePayload.performance || {}).total_ms || 0).toFixed(2), 'ms')}`;
        const delta = Number(currentSummary.total_duration_seconds || 0) - Number(baselineSummary.total_duration_seconds || 0);
        summaryText = delta > 0 ? '构建耗时较上次变慢，建议优先看最慢 phase 和关键路径。'
          : delta < 0 ? '构建耗时较上次改善，优化动作可能已生效。'
          : '构建耗时与上次接近，可结合建议数量判断优化空间。';
      }

      return `
        <div class="item">
          <strong>趋势对比</strong>
          <span>基线任务：${escapeHtml(baselineTask.task_id)} · ${escapeHtml(formatDateTime(baselineTask.updated_at_ms))}</span>
          <div class="metric-grid" style="margin-top:12px;">
            ${metricsHtml}
          </div>
          <div class="tag-list">
            <span class="tag">${escapeHtml(normalizeWorkspaceName(baselineTask.workspace_path))}</span>
            <span class="tag">${escapeHtml(normalizeModeLabel(baselineTask.mode || 'cycle'))}</span>
          </div>
          <p style="margin-top:12px;color:var(--muted);line-height:1.7;">${escapeHtml(summaryText)}</p>
        </div>`;
    }

    function renderTaskDrawerSummary(resultPayload) {
      if (!resultPayload || !resultPayload.ok) {
        return '<div class="item"><strong>暂无结果摘要</strong><span>该任务尚未产出完整结果，或结果文件暂不可用。</span></div>';
      }

      if (resultPayload.mode === 'cycle') {
        const report = resultPayload.report.report || {};
        const cycles = report.cycles || [];
        const shortestCycle = cycles.length
          ? cycles.reduce((best, current) => Number(current.length || 0) < Number(best.length || 0) ? current : best, cycles[0])
          : null;
        return `
          <div class="item">
            <strong>循环依赖摘要</strong>
            <div class="metric-grid" style="margin-top:12px;">
              ${renderMetric('循环数', report.total_cycles ?? 0)}
              ${renderMetric('最短环长度', shortestCycle ? shortestCycle.length ?? 0 : 0)}
              ${renderMetric('分析总耗时(ms)', Number((resultPayload.performance || {}).total_ms || 0).toFixed(2))}
            </div>
            <div class="tag-list">
              <span class="tag ${Number(report.total_cycles || 0) > 0 ? 'danger' : 'success'}">${Number(report.total_cycles || 0) > 0 ? '存在结构风险' : '未发现循环'}</span>
              ${shortestCycle ? `<span class="tag">最短路径：${escapeHtml((shortestCycle.path || []).slice(0, 3).join(' → '))}${(shortestCycle.path || []).length > 3 ? '…' : ''}</span>` : ''}
            </div>
          </div>`;
      }

      if (resultPayload.mode === 'unused') {
        const report = resultPayload.report.unused_dependencies_report || {};
        const stats = report.statistics || {};
        return `
          <div class="item">
            <strong>未使用依赖摘要</strong>
            <div class="metric-grid" style="margin-top:12px;">
              ${renderMetric('未使用依赖', report.total_unused_dependencies ?? 0)}
              ${renderMetric('高置信度', stats.high_confidence ?? 0)}
              ${renderMetric('中/低置信度', Number(stats.medium_confidence ?? 0) + Number(stats.low_confidence ?? 0))}
            </div>
            <div class="tag-list">
              <span class="tag ${Number(stats.high_confidence || 0) > 0 ? 'warning' : 'success'}">${Number(stats.high_confidence || 0) > 0 ? '建议优先清理高置信度' : '暂无高置信度项'}</span>
              <span class="tag">分析总耗时：${escapeHtml(Number((resultPayload.performance || {}).total_ms || 0).toFixed(2))}ms</span>
            </div>
          </div>`;
      }

      const report = resultPayload.report.build_time_report || {};
      const summary = report.summary || {};
      const phase = report.phase_stats || {};
      const suggestions = report.suggestions || [];
      const hottestPhase = [
        ['loading', Number(phase.loading_seconds || 0)],
        ['analysis', Number(phase.analysis_seconds || 0)],
        ['execution', Number(phase.execution_seconds || 0)],
        ['vfs', Number(phase.vfs_seconds || 0)],
        ['other', Number(phase.other_seconds || 0)]
      ].sort((left, right) => right[1] - left[1])[0];
      return `
        <div class="item">
          <strong>构建耗时摘要</strong>
          <div class="metric-grid" style="margin-top:12px;">
            ${renderMetric('总耗时(s)', summary.total_duration_seconds ?? 0)}
            ${renderMetric('最慢阶段', hottestPhase ? hottestPhase[0] : 'n/a')}
            ${renderMetric('优化建议', suggestions.length)}
          </div>
          <div class="tag-list">
            ${hottestPhase ? `<span class="tag warning">${escapeHtml(hottestPhase[0])}：${escapeHtml(hottestPhase[1])}s</span>` : ''}
            <span class="tag">分析总耗时：${escapeHtml(Number((resultPayload.performance || {}).total_ms || 0).toFixed(2))}ms</span>
            <span class="tag ${suggestions.length ? 'warning' : 'success'}">${suggestions.length ? '存在可优化项' : '暂无建议'}</span>
          </div>
        </div>`;
    }

    function formatDelta(current, previous, suffix = '') {
      const currentValue = Number(current || 0);
      const previousValue = Number(previous || 0);
      const delta = currentValue - previousValue;
      const rounded = Math.abs(delta) >= 100 ? delta.toFixed(0) : delta.toFixed(2);
      const prefix = delta > 0 ? '+' : '';
      return `${prefix}${rounded}${suffix}`;
    }

    function compareTone(delta) {
      const numeric = Number(delta || 0);
      if (numeric > 0) return 'danger';
      if (numeric < 0) return 'success';
      return '';
    }

    function buildTrendMetric(label, current, previous, suffix = '') {
      const delta = Number(current || 0) - Number(previous || 0);
      return `
        <div class="metric-card">
          <div class="label">${escapeHtml(label)}</div>
          <div class="value">${escapeHtml(String(current ?? 0))}</div>
          <div class="caption" style="margin-top:8px;color:${delta > 0 ? 'var(--danger)' : delta < 0 ? 'var(--success)' : 'var(--muted)'};">
            较上次 ${escapeHtml(formatDelta(current, previous, suffix))}
          </div>
        </div>`;
    }

    async function fetchTaskResultPayload(taskId) {
      const response = await fetch(`/api/tasks/${encodeURIComponent(taskId)}?include_result=1`);
      const data = await response.json();
      if (!response.ok || !data.ok || !data.result) {
        throw new Error(data.error || data.message || '读取任务结果失败');
      }
      return data.result;
    }

    function findComparableTaskForTask(task, excludeTaskId = '') {
      if (!task) {
        return null;
      }
      return latestTaskHistory.find((candidate) =>
        candidate.status === 'completed' &&
        candidate.task_id !== excludeTaskId &&
        candidate.mode === task.mode &&
        candidate.workspace_path === task.workspace_path);
    }

    function findComparableTask(payload) {
      if (!payload || !payload.ok) {
        return null;
      }
      return findComparableTaskForTask({
        mode: payload.mode,
        workspace_path: payload.workspace_path
      }, latestPayloadTaskId);
    }

    async function ensureComparePayload(payload) {
      const comparableTask = findComparableTask(payload);
      if (!comparableTask) {
        comparePayload = null;
        return null;
      }
      if (comparePayload && comparePayload.task_id === comparableTask.task_id && comparePayload.result) {
        return comparePayload;
      }
      comparePayload = {
        task_id: comparableTask.task_id,
        result: await fetchTaskResultPayload(comparableTask.task_id)
      };
      return comparePayload;
    }

    function buildCurrentFormPayload() {
      return {
        workspace_path: workspaceInputEl.value.trim(),
        bazel_binary: bazelInputEl.value.trim() || 'bazel',
        mode: modeSelectEl.value,
        include_tests: includeTestsEl.checked,
        force_refresh: forceRefreshEl.checked
      };
    }

    function createPresetName(payload) {
      const workspaceName = normalizeWorkspaceName(payload.workspace_path);
      const modeName = normalizeModeLabel(payload.mode || 'cycle');
      return `${workspaceName} · ${modeName}${payload.include_tests ? ' · tests' : ''}`;
    }

    function sanitizePresets(presets) {
      return (Array.isArray(presets) ? presets : [])
        .map((preset) => ({
          id: String(preset.id || `preset-${Date.now()}`),
          name: String(preset.name || '').trim(),
          workspace_path: String(preset.workspace_path || '').trim(),
          bazel_binary: String(preset.bazel_binary || 'bazel').trim() || 'bazel',
          mode: String(preset.mode || 'cycle'),
          include_tests: Boolean(preset.include_tests),
          force_refresh: Boolean(preset.force_refresh),
          created_at_ms: Number(preset.created_at_ms || Date.now()),
          last_used_ms: Number(preset.last_used_ms || 0)
        }))
        .filter((preset) => preset.name && preset.workspace_path)
        .slice(0, 10);
    }

    function buildDownloadFilename(payload, extension) {
      const mode = (payload && payload.mode ? payload.mode : 'report').replace(/[^a-z0-9-]+/gi, '-');
      const workspace = (payload && payload.workspace_path ? payload.workspace_path.split('/').filter(Boolean).pop() : 'workspace')
        .replace(/[^a-z0-9-_.]+/gi, '-');
      const stamp = new Date().toISOString().replace(/[:.]/g, '-');
      return `${workspace}-${mode}-${stamp}.${extension}`;
    }

    function severityTone(value) {
      const text = String(value || '').toLowerCase();
      if (text.includes('high') || text.includes('高')) return 'danger';
      if (text.includes('medium') || text.includes('中')) return 'warning';
      if (text.includes('low') || text.includes('低')) return 'success';
      return '';
    }

    function loadPreferences() {
      try {
        const raw = window.localStorage.getItem(preferencesStorageKey);
        if (!raw) {
          return;
        }
        const data = JSON.parse(raw);
        recentWorkspaces = Array.isArray(data.recentWorkspaces) ? data.recentWorkspaces.slice(0, 6) : [];
        favoriteWorkspaces = Array.isArray(data.favoriteWorkspaces) ? data.favoriteWorkspaces.slice(0, 8) : [];
        analysisPresets = sanitizePresets(data.analysisPresets);
        if (data.workspace_path) workspaceInputEl.value = data.workspace_path;
        if (data.bazel_binary) bazelInputEl.value = data.bazel_binary;
        if (typeof data.include_tests === 'boolean') includeTestsEl.checked = data.include_tests;
        if (typeof data.force_refresh === 'boolean') forceRefreshEl.checked = data.force_refresh;
        if (data.mode) setMode(data.mode);
      } catch (error) {
      }
    }

    function savePreferences() {
      try {
        window.localStorage.setItem(preferencesStorageKey, JSON.stringify({
          workspace_path: workspaceInputEl.value.trim(),
          bazel_binary: bazelInputEl.value.trim() || 'bazel',
          include_tests: includeTestsEl.checked,
          force_refresh: forceRefreshEl.checked,
          mode: modeSelectEl.value,
          recentWorkspaces,
          favoriteWorkspaces,
          analysisPresets
        }));
      } catch (error) {
      }
    }

    function renderPresetPanel() {
      if (!presetPanelEl) {
        return;
      }
      if (!analysisPresets.length) {
        presetPanelEl.innerHTML = `
          <div class="item" style="padding:12px;">
            <strong>暂无预设</strong>
            <span>先配置好参数，再点击“保存当前为预设”。</span>
          </div>`;
        return;
      }

      const orderedPresets = analysisPresets.slice().sort((left, right) => {
        const leftUsed = Number(left.last_used_ms || 0);
        const rightUsed = Number(right.last_used_ms || 0);
        if (leftUsed !== rightUsed) {
          return rightUsed - leftUsed;
        }
        return Number(right.created_at_ms || 0) - Number(left.created_at_ms || 0);
      });

      presetPanelEl.innerHTML = orderedPresets.map((preset) => `
        <div class="item" style="padding:12px;">
          <strong>${escapeHtml(preset.name)}</strong>
          <span>${escapeHtml(preset.workspace_path)}</span>
          <div class="tag-list">
            <span class="tag">${escapeHtml(normalizeModeLabel(preset.mode))}</span>
            <span class="tag">${escapeHtml(preset.bazel_binary || 'bazel')}</span>
            ${preset.include_tests ? '<span class="tag">tests</span>' : ''}
            ${preset.force_refresh ? '<span class="tag warning">force</span>' : ''}
            ${preset.last_used_ms ? `<span class="tag">最近使用：${escapeHtml(formatDateTime(preset.last_used_ms))}</span>` : '<span class="tag">未使用</span>'}
          </div>
          <div class="action-row" style="margin-top:10px;">
            <button class="secondary-button preset-run-button" type="button" data-preset-id="${escapeHtml(preset.id)}">一键运行</button>
            <button class="secondary-button preset-apply-button" type="button" data-preset-id="${escapeHtml(preset.id)}">载入预设</button>
            <button class="secondary-button preset-delete-button" type="button" data-preset-id="${escapeHtml(preset.id)}">删除</button>
          </div>
        </div>`).join('');

      presetPanelEl.querySelectorAll('.preset-apply-button').forEach((button) => {
        button.addEventListener('click', () => {
          const preset = analysisPresets.find((item) => item.id === button.dataset.presetId);
          if (!preset) {
            return;
          }
          applyTaskToForm(preset);
          forceRefreshEl.checked = Boolean(preset.force_refresh);
          savePreferences();
          renderPresetPanel();
          setStatus(`已载入预设：${preset.name}`, 'success');
        });
      });
      presetPanelEl.querySelectorAll('.preset-run-button').forEach((button) => {
        button.addEventListener('click', async () => {
          const preset = analysisPresets.find((item) => item.id === button.dataset.presetId);
          if (!preset) {
            return;
          }
          await runPreset(preset);
        });
      });
      presetPanelEl.querySelectorAll('.preset-delete-button').forEach((button) => {
        button.addEventListener('click', () => {
          const preset = analysisPresets.find((item) => item.id === button.dataset.presetId);
          analysisPresets = analysisPresets.filter((item) => item.id !== button.dataset.presetId);
          savePreferences();
          renderPresetPanel();
          setStatus(`已删除预设：${preset ? preset.name : button.dataset.presetId}`, 'success');
        });
      });
    }

    function saveCurrentAsPreset() {
      const payload = buildCurrentFormPayload();
      if (!payload.workspace_path) {
        setStatus('保存预设前请先填写 workspace 路径。', 'error');
        return;
      }
      const suggestedName = createPresetName(payload);
      const customName = window.prompt('输入预设名称：', suggestedName);
      if (customName == null) {
        return;
      }
      const trimmedName = String(customName).trim();
      if (!trimmedName) {
        setStatus('预设名称不能为空。', 'error');
        return;
      }
      const duplicateIndex = analysisPresets.findIndex((item) => item.name === trimmedName);
      const preset = {
        id: duplicateIndex >= 0 ? analysisPresets[duplicateIndex].id : `preset-${Date.now()}`,
        name: trimmedName,
        workspace_path: payload.workspace_path,
        bazel_binary: payload.bazel_binary,
        mode: payload.mode,
        include_tests: payload.include_tests,
        force_refresh: payload.force_refresh,
        created_at_ms: duplicateIndex >= 0 ? analysisPresets[duplicateIndex].created_at_ms : Date.now(),
        last_used_ms: duplicateIndex >= 0 ? analysisPresets[duplicateIndex].last_used_ms : 0
      };
      if (duplicateIndex >= 0) {
        analysisPresets.splice(duplicateIndex, 1, preset);
      } else {
        analysisPresets = [preset].concat(analysisPresets).slice(0, 10);
      }
      savePreferences();
      renderPresetPanel();
      setStatus(`已保存预设：${preset.name}`, 'success');
    }

    function pushRecentWorkspace(workspacePath) {
      const trimmed = String(workspacePath || '').trim();
      if (!trimmed) {
        return;
      }
      recentWorkspaces = [trimmed].concat(recentWorkspaces.filter((item) => item !== trimmed)).slice(0, 6);
      renderRecentWorkspaces();
      savePreferences();
    }

    function isFavoriteWorkspace(workspacePath) {
      return favoriteWorkspaces.includes(String(workspacePath || '').trim());
    }

    function toggleFavoriteWorkspace(workspacePath) {
      const trimmed = String(workspacePath || '').trim();
      if (!trimmed) {
        setStatus('请先填写 workspace 路径。', 'error');
        return;
      }
      if (isFavoriteWorkspace(trimmed)) {
        favoriteWorkspaces = favoriteWorkspaces.filter((item) => item !== trimmed);
        setStatus(`已取消收藏工作区：${trimmed}`, 'success');
      } else {
        favoriteWorkspaces = [trimmed].concat(favoriteWorkspaces.filter((item) => item !== trimmed)).slice(0, 8);
        setStatus(`已收藏工作区：${trimmed}`, 'success');
      }
      renderRecentWorkspaces();
      savePreferences();
    }

    function renderRecentWorkspaces() {
      if (!recentWorkspacesPanelEl) {
        return;
      }
      if (!recentWorkspaces.length && !favoriteWorkspaces.length) {
        recentWorkspacesPanelEl.innerHTML = `
          <div class="action-row" style="margin-top:8px;">
            <button class="secondary-button favorite-current-workspace-button" type="button">收藏当前工作区</button>
          </div>`;
        recentWorkspacesPanelEl.querySelector('.favorite-current-workspace-button').addEventListener('click', () => {
          toggleFavoriteWorkspace(workspaceInputEl.value);
        });
        return;
      }
      recentWorkspacesPanelEl.innerHTML = `
        <div class="stack" style="padding:8px 0 0;">
          <div class="action-row" style="margin-top:0;">
            <button class="secondary-button favorite-current-workspace-button" type="button">${isFavoriteWorkspace(workspaceInputEl.value) ? '取消收藏当前工作区' : '收藏当前工作区'}</button>
          </div>
          ${favoriteWorkspaces.length ? `
            <div class="tag-list" style="margin-top:8px;">
              <span class="field-tip" style="width:100%;">收藏工作区：</span>
              ${favoriteWorkspaces.map((workspace) => `
                <button class="secondary-button favorite-workspace-button" type="button" data-workspace="${escapeHtml(workspace)}" title="${escapeHtml(workspace)}">★ ${escapeHtml(workspace.split('/').filter(Boolean).pop() || workspace)}</button>
              `).join('')}
            </div>` : ''}
          ${recentWorkspaces.length ? `
            <div class="tag-list" style="margin-top:8px;">
              <span class="field-tip" style="width:100%;">最近工作区：</span>
              ${recentWorkspaces.map((workspace) => `
                <button class="secondary-button recent-workspace-button" type="button" data-workspace="${escapeHtml(workspace)}" title="${escapeHtml(workspace)}">${escapeHtml(workspace.split('/').filter(Boolean).pop() || workspace)}</button>
              `).join('')}
            </div>` : ''}
        </div>`;
      recentWorkspacesPanelEl.querySelector('.favorite-current-workspace-button').addEventListener('click', () => {
        toggleFavoriteWorkspace(workspaceInputEl.value);
      });
      recentWorkspacesPanelEl.querySelectorAll('.favorite-workspace-button').forEach((button) => {
        button.addEventListener('click', () => {
          workspaceInputEl.value = button.dataset.workspace || '';
          savePreferences();
          renderRecentWorkspaces();
          setStatus(`已选中收藏工作区：${workspaceInputEl.value}`, 'success');
        });
      });
      recentWorkspacesPanelEl.querySelectorAll('.recent-workspace-button').forEach((button) => {
        button.addEventListener('click', () => {
          workspaceInputEl.value = button.dataset.workspace || '';
          savePreferences();
          renderRecentWorkspaces();
          setStatus(`已选中最近工作区：${workspaceInputEl.value}`, 'success');
        });
      });
    }

    function applyTaskToForm(task) {
      if (!task) {
        return;
      }
      workspaceInputEl.value = task.workspace_path || '';
      bazelInputEl.value = task.bazel_binary || 'bazel';
      includeTestsEl.checked = Boolean(task.include_tests);
      setMode(task.mode || 'cycle');
      savePreferences();
      renderRecentWorkspaces();
    }

    async function rerunTask(task) {
      applyTaskToForm(task);
      forceRefreshEl.checked = false;
      savePreferences();
      setStatus(`已载入任务配置：${task.task_id}，准备重新运行。`, 'success');
      await submitAnalysis(buildCurrentFormPayload());
    }

    async function runPreset(preset) {
      if (!preset) {
        return;
      }
      applyTaskToForm(preset);
      forceRefreshEl.checked = Boolean(preset.force_refresh);
      analysisPresets = analysisPresets.map((item) => item.id === preset.id
        ? { ...item, last_used_ms: Date.now() }
        : item);
      savePreferences();
      renderPresetPanel();
      setStatus(`正在运行预设：${preset.name}`, 'loading');
      await submitAnalysis(buildCurrentFormPayload());
    }

    function setStatus(message, kind = '') {
      statusEl.className = 'status' + (kind ? ' ' + kind : '');
      statusTextEl.textContent = message;
    }

    function stopPolling() {
      if (activePollTimer) {
        clearTimeout(activePollTimer);
        activePollTimer = null;
      }
      activeTaskPollAttempts = 0;
    }

    function setToolbarState(view) {
      Object.entries(toolbarButtons).forEach(([name, button]) => {
        button.classList.toggle('active', name === view);
      });
    }

    async function refreshCacheStatus() {
      try {
        const response = await fetch('/api/cache');
        const data = await response.json();
        if (!response.ok || !data.ok) {
          return;
        }
        latestCacheStatus = data;
        renderCachePanel();
      } catch (error) {
      }
    }

    async function refreshTaskHistory(resetOffset = false) {
      if (resetOffset) {
        taskFilters.offset = 0;
      }
      try {
        const params = new URLSearchParams();
        params.set('limit', String(taskFilters.limit));
        params.set('offset', String(taskFilters.offset));
        if (taskFilters.q) params.set('q', taskFilters.q);
        if (taskFilters.mode) params.set('mode', taskFilters.mode);
        if (taskFilters.status) params.set('status', taskFilters.status);

        const response = await fetch(`/api/tasks?${params.toString()}`);
        const data = await response.json();
        if (!response.ok || !data.ok) {
          return;
        }
        const tasks = Array.isArray(data.tasks) ? data.tasks : [];
        latestTaskHistory = resetOffset || taskFilters.offset === 0
          ? tasks
          : latestTaskHistory.concat(tasks);
        latestTaskHistoryTotal = Number(data.total || 0);
        renderTaskHistoryPanel();
      } catch (error) {
      }
    }

    async function openTaskResult(taskId) {
      try {
        const response = await fetch(`/api/tasks/${encodeURIComponent(taskId)}?include_result=1`);
        const data = await response.json();
        if (!response.ok || !data.ok) {
          throw new Error(data.error || '读取任务详情失败');
        }
        if (!data.result) {
          throw new Error(data.message || '该任务还没有可展示结果');
        }
        latestPayload = data.result;
        latestPayloadTaskId = data.task_id || taskId;
        currentView = 'summary';
        setStatus(`已打开任务 ${taskId} · ${normalizeModeLabel(data.mode || data.result.mode)}`, 'success');
        refreshView();
      } catch (error) {
        const message = error && error.message ? error.message : String(error);
        setStatus(message, 'error');
      }
    }

    function nextPollDelay() {
      const attempt = activeTaskPollAttempts;
      if (attempt < 3) return 600;
      if (attempt < 8) return 1000;
      if (attempt < 15) return 1600;
      return 2400;
    }

    function renderCachePanel() {
      const panel = document.getElementById('cache-panel');
      if (!panel) {
        return;
      }

      if (!latestCacheStatus) {
        panel.innerHTML = `
          <div class="helper-card">
            <strong>缓存管理</strong>
            <span>正在加载缓存状态…</span>
          </div>`;
        return;
      }

      panel.innerHTML = `
        <div class="helper-card">
          <strong>缓存管理</strong>
          <span>响应缓存：${escapeHtml(latestCacheStatus.response_cache_size)} · 依赖上下文：${escapeHtml(latestCacheStatus.dependency_context_cache_size)} · Parser：${escapeHtml(latestCacheStatus.workspace_parser_cache_size)} · 任务数：${escapeHtml(latestCacheStatus.task_count)}</span>
          <div class="action-row" style="margin-top:12px;">
            <button id="refresh-cache-button" class="secondary-button" type="button">刷新缓存状态</button>
            <button id="clear-cache-button" class="secondary-button" type="button">清空全部缓存</button>
          </div>
        </div>`;

      document.getElementById('refresh-cache-button').addEventListener('click', () => {
        refreshCacheStatus();
      });

      document.getElementById('clear-cache-button').addEventListener('click', async () => {
        try {
          const response = await fetch('/api/cache/clear', { method: 'POST' });
          const data = await response.json();
          if (!response.ok || !data.ok) {
            throw new Error(data.error || '清空缓存失败');
          }
          setStatus('缓存已清空。', 'success');
          latestPayload = null;
          refreshView();
          await refreshCacheStatus();
          await refreshTaskHistory();
        } catch (error) {
          const message = error && error.message ? error.message : String(error);
          setStatus(message, 'error');
        }
      });
    }

    )HTML") + std::string(R"HTML(    function renderTaskHistoryPanel() {
      const panel = document.getElementById('task-history-panel');
      if (!panel) {
        return;
      }

      if (!latestTaskHistory.length) {
        panel.innerHTML = `
          <div class="helper-card">
            <strong>最近任务</strong>
            <span>暂无任务记录。</span>
          </div>`;
        return;
      }

      const prioritizedTasks = sortTasksForDisplay(latestTaskHistory);
      const summary = summarizeTaskHistory(prioritizedTasks);
      const activeTasks = prioritizedTasks.filter((task) => task.status === 'running' || task.status === 'queued' || task.status === 'failed');
      const recentCompletedTasks = prioritizedTasks.filter((task) => task.status === 'completed');

      panel.innerHTML = `
        <div class="helper-card">
          <strong>最近任务</strong>
          <span>支持按关键字、模式、状态筛选，并可跨重启保留最近任务结果。</span>
          <div class="metric-grid" style="margin-top:12px;">
            ${renderMetric('当前列表', summary.total)}
            ${renderMetric('运行/排队', summary.running)}
            ${renderMetric('失败任务', summary.failed)}
            ${renderMetric('缓存命中', `${summary.total ? Math.round((summary.cacheHit / summary.total) * 100) : 0}%`)}
          </div>
          <div class="tag-list" style="margin-top:12px;">
            <span class="tag ${taskFilters.status === '' ? 'success' : ''} task-filter-shortcut" data-status="">全部</span>
            <span class="tag ${taskFilters.status === 'running' ? 'warning' : ''} task-filter-shortcut" data-status="running">仅运行中</span>
            <span class="tag ${taskFilters.status === 'failed' ? 'danger' : ''} task-filter-shortcut" data-status="failed">仅失败</span>
            <span class="tag ${taskFilters.status === 'completed' ? 'success' : ''} task-filter-shortcut" data-status="completed">仅成功</span>
          </div>
          <div class="form-grid" style="margin-top:12px;">
            <div class="input-group">
              <label class="field-label" for="task-search-input">
                <span>搜索任务</span>
                <span class="field-tip">按 task id / workspace / 状态消息过滤</span>
              </label>
              <input id="task-search-input" type="text" placeholder="输入 task id、workspace 或错误信息">
            </div>
            <div class="mode-grid" style="grid-template-columns:repeat(2,minmax(0,1fr));">
              <div class="input-group">
                <label class="field-label" for="task-mode-filter">
                  <span>模式</span>
                </label>
                <select id="task-mode-filter">
                  <option value="">全部模式</option>
                  <option value="cycle">循环依赖</option>
                  <option value="unused">未使用依赖</option>
                  <option value="build-time">构建耗时</option>
                </select>
              </div>
              <div class="input-group">
                <label class="field-label" for="task-status-filter">
                  <span>状态</span>
                </label>
                <select id="task-status-filter">
                  <option value="">全部状态</option>
                  <option value="queued">queued</option>
                  <option value="running">running</option>
                  <option value="completed">completed</option>
                  <option value="failed">failed</option>
                </select>
              </div>
            </div>
          </div>
          <div class="stack" style="padding:12px 0 0;">
            ${activeTasks.length ? `
              <div class="item">
                <h3>优先关注</h3>
                <p>先处理运行中、排队中和失败任务，缩短反馈回路。</p>
                <div class="stack" style="padding:12px 0 0;">
                  ${activeTasks.map((task) => buildTaskItem(task)).join('')}
                </div>
              </div>` : ''}
            ${recentCompletedTasks.length ? `
              <div class="item">
                <h3>最近完成</h3>
                <p>${summary.slowest ? `当前列表最慢任务：${escapeHtml(normalizeWorkspaceName(summary.slowest.workspace_path))} · ${escapeHtml(Number(summary.slowest.total_ms || 0).toFixed(2))}ms` : '可直接复用或重跑成功任务配置。'}</p>
                <div class="stack" style="padding:12px 0 0;">
                  ${recentCompletedTasks.map((task) => buildTaskItem(task)).join('')}
                </div>
              </div>` : ''}
          </div>
          <div class="action-row" style="margin-top:12px;">
            <span class="field-tip">已显示 ${escapeHtml(latestTaskHistory.length)} / ${escapeHtml(latestTaskHistoryTotal)} 条</span>
            <button id="task-load-more-button" class="secondary-button" type="button" ${latestTaskHistory.length < latestTaskHistoryTotal ? '' : 'disabled'}>加载更多</button>
          </div>
        </div>`;

      const searchInput = document.getElementById('task-search-input');
      const modeFilter = document.getElementById('task-mode-filter');
      const statusFilter = document.getElementById('task-status-filter');
      if (searchInput) {
        searchInput.value = taskFilters.q;
        searchInput.addEventListener('input', (event) => {
          taskFilters.q = event.target.value.trim();
          if (taskSearchDebounceTimer) {
            clearTimeout(taskSearchDebounceTimer);
          }
          taskSearchDebounceTimer = setTimeout(() => {
            refreshTaskHistory(true);
          }, 220);
        });
      }
      if (modeFilter) {
        modeFilter.value = taskFilters.mode;
        modeFilter.addEventListener('change', (event) => {
          taskFilters.mode = event.target.value;
          refreshTaskHistory(true);
        });
      }
      if (statusFilter) {
        statusFilter.value = taskFilters.status;
        statusFilter.addEventListener('change', (event) => {
          taskFilters.status = event.target.value;
          refreshTaskHistory(true);
        });
      }
      panel.querySelectorAll('.task-filter-shortcut').forEach((button) => {
        button.addEventListener('click', () => {
          taskFilters.status = button.dataset.status || '';
          refreshTaskHistory(true);
        });
      });

      panel.querySelectorAll('.task-open-button').forEach((button) => {
        button.addEventListener('click', () => {
          openTaskResult(button.dataset.taskId);
        });
      });
      panel.querySelectorAll('.task-detail-button').forEach((button) => {
        button.addEventListener('click', () => {
          openTaskDrawer(button.dataset.taskId);
        });
      });
      panel.querySelectorAll('.task-apply-button').forEach((button) => {
        button.addEventListener('click', () => {
          const task = latestTaskHistory.find((item) => item.task_id === button.dataset.taskId);
          applyTaskToForm(task);
          setStatus(`已载入任务 ${button.dataset.taskId} 的配置。`, 'success');
        });
      });
      panel.querySelectorAll('.task-rerun-button').forEach((button) => {
        button.addEventListener('click', () => {
          const task = latestTaskHistory.find((item) => item.task_id === button.dataset.taskId);
          if (!task) {
            setStatus('未找到任务配置，无法重新运行。', 'error');
            return;
          }
          rerunTask(task);
        });
      });

      const loadMoreButton = document.getElementById('task-load-more-button');
      if (loadMoreButton) {
        loadMoreButton.addEventListener('click', () => {
          taskFilters.offset += taskFilters.limit;
          refreshTaskHistory();
        });
      }
    }

    function updateResultsMeta(payload) {
      if (!payload || !payload.ok) {
        resultsMetaEl.innerHTML = '<span class="mini-chip">尚未执行成功分析</span>';
        return;
      }

      const chips = [
        `模式：${normalizeModeLabel(payload.mode)}`,
        `Workspace：${payload.workspace_path || '-'}`,
        payload.include_tests ? '包含测试目标' : '不含测试目标',
        payload.cache_hit ? '结果来自缓存' : '结果为实时分析'
      ];
      resultsMetaEl.innerHTML = chips.map((item) => `<span class="mini-chip">${escapeHtml(item)}</span>`).join('');
    }

    function renderMetric(label, value) {
      return `<div class="metric-card"><div class="label">${escapeHtml(label)}</div><div class="value">${escapeHtml(value)}</div></div>`;
    }

    function downloadTextFile(filename, content, mimeType) {
      const blob = new Blob([content], { type: mimeType });
      const url = URL.createObjectURL(blob);
      const link = document.createElement('a');
      link.href = url;
      link.download = filename;
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
      URL.revokeObjectURL(url);
    }

    async function copyTextToClipboard(text) {
      if (navigator.clipboard && navigator.clipboard.writeText) {
        await navigator.clipboard.writeText(text);
        return;
      }
      const textarea = document.createElement('textarea');
      textarea.value = text;
      document.body.appendChild(textarea);
      textarea.select();
      document.execCommand('copy');
      document.body.removeChild(textarea);
    }

    function buildTaskSnapshot(task, resultPayload, baselineTask) {
      return {
        task_id: task.task_id,
        mode: task.mode,
        workspace_path: task.workspace_path,
        bazel_binary: task.bazel_binary,
        include_tests: Boolean(task.include_tests),
        status: task.status,
        cache_hit: Boolean(task.cache_hit),
        total_ms: Number(task.total_ms || 0),
        created_at_ms: Number(task.created_at_ms || 0),
        updated_at_ms: Number(task.updated_at_ms || 0),
        baseline_task_id: baselineTask ? baselineTask.task_id : '',
        result: resultPayload || null
      };
    }

    function buildTaskSnapshotMarkdown(task, resultPayload, baselineTask) {
      const lines = [
        `# 任务快照`,
        ``,
        `- 任务 ID：${task.task_id}`,
        `- 模式：${normalizeModeLabel(task.mode || 'cycle')}`,
        `- Workspace：${task.workspace_path || '未记录'}`,
        `- Bazel：${task.bazel_binary || 'bazel'}`,
        `- 包含测试：${task.include_tests ? '是' : '否'}`,
        `- 状态：${task.status}`,
        `- 缓存：${task.cache_hit ? '命中' : '未命中'}`,
        `- 总耗时：${task.total_ms ? `${Number(task.total_ms).toFixed(2)}ms` : '未记录'}`,
        `- 更新时间：${formatDateTime(task.updated_at_ms)}`,
        baselineTask ? `- 基线任务：${baselineTask.task_id}` : `- 基线任务：无`,
        ``
      ];

      if (resultPayload && resultPayload.ok) {
        if (resultPayload.mode === 'cycle') {
          const report = resultPayload.report.report || {};
          lines.push(`## 循环依赖摘要`, ``);
          lines.push(`- 循环数：${report.total_cycles ?? 0}`);
        } else if (resultPayload.mode === 'unused') {
          const report = resultPayload.report.unused_dependencies_report || {};
          const stats = report.statistics || {};
          lines.push(`## 未使用依赖摘要`, ``);
          lines.push(`- 未使用依赖：${report.total_unused_dependencies ?? 0}`);
          lines.push(`- 高置信度：${stats.high_confidence ?? 0}`);
        } else {
          const summary = ((resultPayload.report || {}).build_time_report || {}).summary || {};
          lines.push(`## 构建耗时摘要`, ``);
          lines.push(`- 总耗时：${summary.total_duration_seconds ?? 0}s`);
          lines.push(`- 分析耗时：${summary.analysis_time_seconds ?? 0}s`);
        }
      }

      return lines.join('\n');
    }

    function renderPerformanceCard(payload) {
      const performance = payload && payload.performance ? payload.performance : null;
      if (!performance) {
        return '';
      }

      return `
        <div class="card">
          <div class="card-header">
            <h3>性能观测</h3>
            <p>用于解释这次分析为什么快或慢，帮助定位瓶颈是在缓存、依赖准备还是报告生成。</p>
          </div>
          <div class="stack">
            <div class="item">
              <div class="metric-grid">
                ${renderMetric('依赖准备(ms)', Number(performance.dependency_prepare_ms || 0).toFixed(2))}
                ${renderMetric('分析执行(ms)', Number(performance.analysis_ms || 0).toFixed(2))}
                ${renderMetric('报告渲染(ms)', Number(performance.report_render_ms || 0).toFixed(2))}
                ${renderMetric('总耗时(ms)', Number(performance.total_ms || 0).toFixed(2))}
              </div>
              <div class="tag-list">
                <span class="tag ${performance.reused_dependency_context ? 'success' : 'warning'}">
                  ${performance.reused_dependency_context ? '复用了依赖上下文' : '重新构建依赖上下文'}
                </span>
                <span class="tag ${payload.cache_hit ? 'success' : 'warning'}">
                  ${payload.cache_hit ? '命中结果缓存' : '未命中结果缓存'}
                </span>
              </div>
            </div>
          </div>
        </div>`;
    }

    function renderEmptyState(title, description) {
      resultsEl.innerHTML = `
        <div class="empty-state">
          <div>
            <h3>${escapeHtml(title)}</h3>
            <p>${escapeHtml(description)}</p>
          </div>
        </div>`;
    }

    function renderCycleSummary(payload) {
      const report = payload.report.report || {};
      const cycles = report.cycles || [];
      const metricHtml = `
        <div class="metric-grid">
          ${renderMetric('模式', '循环依赖')}
          ${renderMetric('循环数量', report.total_cycles ?? 0)}
          ${renderMetric('Workspace', payload.workspace_path || '-')}
        </div>`;

      const overviewCard = `
        <div class="card">
          <div class="card-header">
            <h3>分析总览</h3>
            <p>优先查看循环数量和路径长度，快速判断结构性风险的严重程度。</p>
          </div>
          <div class="stack">
            <div class="item">
              <h3>摘要</h3>
              <p>共发现 ${escapeHtml(report.total_cycles ?? 0)} 条循环依赖链路。建议先处理长度短、可直接移除依赖的循环，再处理复杂链路。</p>
              <div class="tag-list">
                <span class="tag">结果时间戳：${escapeHtml(report.timestamp || '-')}</span>
                <span class="tag">${escapeHtml(payload.include_tests ? '包含测试目标' : '未包含测试目标')}</span>
              </div>
            </div>
          </div>
        </div>`;

      const cyclesCard = cycles.length
        ? `
          <div class="card">
            <div class="card-header">
              <h3>循环详情</h3>
              <p>展示循环路径、可移除依赖和修复建议，适合逐条排查。</p>
            </div>
            <div class="stack">
              ${cycles.map((cycle) => `
                <article class="item">
                  <h3>#${cycle.id} · ${escapeHtml(cycle.type || 'UNKNOWN')}</h3>
                  <p>路径长度：${escapeHtml(cycle.length ?? 0)} 个节点</p>
                  <div class="path-chip-list">
                    ${(cycle.path || []).map((node) => `<span class="tag">${escapeHtml(node)}</span>`).join('')}
                  </div>
                  ${(cycle.removable_dependencies || []).length ? `<ul>${cycle.removable_dependencies.map((dep) => `<li>${escapeHtml(dep.from_target)} → ${escapeHtml(dep.to_target)}${dep.reason ? `（${escapeHtml(dep.reason)}）` : ''}</li>`).join('')}</ul>` : '<p>没有可直接移除的依赖建议。</p>'}
                  ${(cycle.suggested_fixes || []).length ? `<ul>${cycle.suggested_fixes.map((fix) => `<li>${escapeHtml(fix)}</li>`).join('')}</ul>` : ''}
                </article>
              `).join('')}
            </div>
          </div>`
        : `
          <div class="card">
            <div class="card-header">
              <h3>循环详情</h3>
              <p>当前工作区没有检测到循环依赖。</p>
            </div>
            <div class="stack">
              <div class="item">
                <h3>未发现循环依赖</h3>
                <p>结构较健康，可以继续查看未使用依赖或构建耗时结果。</p>
              </div>
          </div>
        </div>`;

      resultsEl.innerHTML = `
        ${metricHtml}
        <div class="summary-grid">
          <div class="summary-main">${cyclesCard}</div>
          <div class="summary-side">${overviewCard}${renderPerformanceCard(payload)}</div>
        </div>`;
    }

    function renderUnusedSummary(payload) {
      const report = payload.report.unused_dependencies_report || {};
      const groups = report.grouped_dependencies || [];
      const stats = report.statistics || {};
      const metricHtml = `
        <div class="metric-grid">
          ${renderMetric('模式', '未使用依赖')}
          ${renderMetric('依赖数量', report.total_unused_dependencies ?? 0)}
          ${renderMetric('高置信度', stats.high_confidence ?? 0)}
          ${renderMetric('中置信度', stats.medium_confidence ?? 0)}
          ${renderMetric('低置信度', stats.low_confidence ?? 0)}
        </div>`;

      const groupedCard = groups.length
        ? `
          <div class="card">
            <div class="card-header">
              <h3>目标分组</h3>
              <p>按 from target 聚合，方便逐个 BUILD 文件进行清理。</p>
            </div>
            <div class="stack">
              ${groups.map((group) => `
                <article class="item">
                  <h3>${escapeHtml(group.from_target)}</h3>
                  <p>可移除依赖：${escapeHtml(group.count ?? 0)}</p>
                  <div class="list-table">
                    ${(group.dependencies || []).map((dep) => `
                      <div class="list-row">
                        <div>
                          <strong>${escapeHtml(dep.to_target)}</strong>
                          <span>${escapeHtml(dep.reason || '未提供原因')}</span>
                        </div>
                        <span class="tag ${severityTone(dep.confidence)}">${escapeHtml(dep.confidence || 'UNKNOWN')}</span>
                      </div>
                    `).join('')}
                  </div>
                </article>
              `).join('')}
            </div>
          </div>`
        : `
          <div class="card">
            <div class="card-header">
              <h3>目标分组</h3>
              <p>当前工作区没有发现可移除依赖。</p>
            </div>
            <div class="stack">
              <div class="item">
                <h3>没有发现未使用依赖</h3>
                <p>当前规则集下依赖关系较干净，可以继续检查循环依赖或构建耗时。</p>
              </div>
            </div>
          </div>`;

      const overviewCard = `
        <div class="card">
          <div class="card-header">
            <h3>置信度概览</h3>
            <p>优先清理高置信度依赖，中低置信度建议结合编译验证处理。</p>
          </div>
          <div class="stack">
            <div class="item">
              <h3>清理建议</h3>
              <div class="tag-list">
                <span class="tag success">高：${escapeHtml(stats.high_confidence ?? 0)}</span>
                <span class="tag warning">中：${escapeHtml(stats.medium_confidence ?? 0)}</span>
                <span class="tag danger">低：${escapeHtml(stats.low_confidence ?? 0)}</span>
              </div>
              <ul>
                <li>先处理高置信度项。</li>
                <li>每次删除后跑一次 Bazel 编译验证。</li>
                <li>对测试相关依赖保持谨慎。</li>
              </ul>
            </div>
          </div>
        </div>`;

      resultsEl.innerHTML = `
        ${metricHtml}
        <div class="summary-grid">
          <div class="summary-main">${groupedCard}</div>
          <div class="summary-side">${overviewCard}${renderPerformanceCard(payload)}</div>
        </div>`;
    }

    function renderBuildTimeSummary(payload) {
      const report = payload.report.build_time_report || {};
      const summary = report.summary || {};
      const phase = report.phase_stats || {};
      const criticalPaths = report.critical_paths || [];
      const suggestions = report.suggestions || [];
      const metricHtml = `
        <div class="metric-grid">
          ${renderMetric('模式', '构建耗时')}
          ${renderMetric('总耗时(s)', summary.total_duration_seconds ?? 0)}
          ${renderMetric('Profile(s)', summary.generation_time_seconds ?? 0)}
          ${renderMetric('分析(s)', summary.analysis_time_seconds ?? 0)}
          ${renderMetric('关键路径', criticalPaths.length)}
          ${renderMetric('优化建议', suggestions.length)}
        </div>`;

      const mainCard = `
        <div class="card">
          <div class="card-header">
            <h3>关键路径与阶段统计</h3>
            <p>将构建时间拆成 phase stats 与最慢路径，帮助快速判断瓶颈位置。</p>
          </div>
          <div class="stack">
            <div class="item">
              <h3>阶段统计</h3>
              <div class="list-table">
                ${[
                  ['loading', phase.loading_seconds],
                  ['analysis', phase.analysis_seconds],
                  ['execution', phase.execution_seconds],
                  ['vfs', phase.vfs_seconds],
                  ['other', phase.other_seconds]
                ].map(([name, value]) => `
                  <div class="list-row">
                    <div>
                      <strong>${escapeHtml(name)}</strong>
                      <span>阶段耗时</span>
                    </div>
                    <span class="tag">${escapeHtml(value ?? 0)}s</span>
                  </div>
                `).join('')}
              </div>
            </div>
            <div class="item">
              <h3>关键路径</h3>
              ${criticalPaths.length ? criticalPaths.map((path) => `
                <div class="item" style="margin-top:12px;">
                  <h3>${escapeHtml(path.event_name)}</h3>
                  <p>规则类型：${escapeHtml(path.rule_type || 'unknown')} · 累计耗时：${escapeHtml(path.cumulative_duration_seconds ?? 0)}s</p>
                  ${(path.path || []).length ? `<div class="path-chip-list">${path.path.map((node) => `<span class="tag">${escapeHtml(node)}</span>`).join('')}</div>` : ''}
                </div>
              `).join('') : '<p>没有关键路径数据。</p>'}
            </div>
          </div>
        </div>`;

      const sideCard = `
        <div class="card">
          <div class="card-header">
            <h3>优化建议</h3>
            <p>按建议严重程度浏览最值得先做的动作。</p>
          </div>
          <div class="stack">
            ${suggestions.length ? suggestions.map((item) => `
              <article class="item">
                <h3>${escapeHtml(item.issue)}</h3>
                <div class="tag-list">
                  <span class="tag ${severityTone(item.severity)}">${escapeHtml(item.severity || 'UNKNOWN')}</span>
                  ${item.estimated_improvement != null ? `<span class="tag">改善估计：${escapeHtml(item.estimated_improvement)}</span>` : ''}
                </div>
                <p style="margin-top:12px;">${escapeHtml(item.suggestion || '未提供建议说明')}</p>
                ${(item.affected_targets || []).length ? `<div class="path-chip-list">${item.affected_targets.map((target) => `<span class="tag">${escapeHtml(target)}</span>`).join('')}</div>` : ''}
              </article>
            `).join('') : `
              <div class="item">
                <h3>当前没有优化建议</h3>
                <p>可以先对 execution 和 critical path 做人工复核。</p>
              </div>`}
          </div>
        </div>`;

      resultsEl.innerHTML = `
        ${metricHtml}
        <div class="summary-grid">
          <div class="summary-main">${mainCard}</div>
          <div class="summary-side">${sideCard}${renderPerformanceCard(payload)}</div>
        </div>`;
    }

    function renderCompareSummary(currentPayload, previousPayload) {
      if (!currentPayload || !currentPayload.ok) {
        renderEmptyState('暂无可对比结果', '先执行一次分析，再切换到结果对比。');
        return;
      }
      if (!previousPayload || !previousPayload.ok) {
        renderEmptyState('缺少历史基线', '当前没有找到同 workspace + 同模式的历史成功任务。');
        return;
      }

      const cards = [];
      if (currentPayload.mode === 'cycle') {
        const currentReport = currentPayload.report.report || {};
        const previousReport = previousPayload.report.report || {};
        const cycleDelta = Number(currentReport.total_cycles || 0) - Number(previousReport.total_cycles || 0);
        cards.push(renderMetric('当前循环数', currentReport.total_cycles ?? 0));
        cards.push(renderMetric('基线循环数', previousReport.total_cycles ?? 0));
        cards.push(`<div class="metric-card"><div class="label">循环变化</div><div class="value" style="color:${cycleDelta > 0 ? 'var(--danger)' : cycleDelta < 0 ? 'var(--success)' : 'var(--text)'}">${escapeHtml(formatDelta(currentReport.total_cycles, previousReport.total_cycles))}</div></div>`);
      } else if (currentPayload.mode === 'unused') {
        const currentReport = currentPayload.report.unused_dependencies_report || {};
        const previousReport = previousPayload.report.unused_dependencies_report || {};
        cards.push(renderMetric('当前未使用依赖', currentReport.total_unused_dependencies ?? 0));
        cards.push(renderMetric('基线未使用依赖', previousReport.total_unused_dependencies ?? 0));
        cards.push(`<div class="metric-card"><div class="label">依赖变化</div><div class="value" style="color:${Number(currentReport.total_unused_dependencies || 0) - Number(previousReport.total_unused_dependencies || 0) > 0 ? 'var(--danger)' : Number(currentReport.total_unused_dependencies || 0) - Number(previousReport.total_unused_dependencies || 0) < 0 ? 'var(--success)' : 'var(--text)'}">${escapeHtml(formatDelta(currentReport.total_unused_dependencies, previousReport.total_unused_dependencies))}</div></div>`);
      } else {
        const currentReport = currentPayload.report.build_time_report || {};
        const previousReport = previousPayload.report.build_time_report || {};
        const currentSummary = currentReport.summary || {};
        const previousSummary = previousReport.summary || {};
        const durationDelta = Number(currentSummary.total_duration_seconds || 0) - Number(previousSummary.total_duration_seconds || 0);
        cards.push(renderMetric('当前总耗时(s)', currentSummary.total_duration_seconds ?? 0));
        cards.push(renderMetric('基线总耗时(s)', previousSummary.total_duration_seconds ?? 0));
        cards.push(`<div class="metric-card"><div class="label">耗时变化</div><div class="value" style="color:${durationDelta > 0 ? 'var(--danger)' : durationDelta < 0 ? 'var(--success)' : 'var(--text)'}">${escapeHtml(formatDelta(currentSummary.total_duration_seconds, previousSummary.total_duration_seconds, 's'))}</div></div>`);
      }

      const currentPerformance = currentPayload.performance || {};
      const previousPerformance = previousPayload.performance || {};

      resultsEl.innerHTML = `
        <div class="metric-grid">
          ${cards.join('')}
          <div class="metric-card">
            <div class="label">当前分析耗时(ms)</div>
            <div class="value">${escapeHtml(Number(currentPerformance.total_ms || 0).toFixed(2))}</div>
          </div>
          <div class="metric-card">
            <div class="label">基线分析耗时(ms)</div>
            <div class="value">${escapeHtml(Number(previousPerformance.total_ms || 0).toFixed(2))}</div>
          </div>
          <div class="metric-card">
            <div class="label">分析耗时变化</div>
            <div class="value" style="color:${Number(currentPerformance.total_ms || 0) - Number(previousPerformance.total_ms || 0) > 0 ? 'var(--danger)' : Number(currentPerformance.total_ms || 0) - Number(previousPerformance.total_ms || 0) < 0 ? 'var(--success)' : 'var(--text)'}">${escapeHtml(formatDelta(currentPerformance.total_ms, previousPerformance.total_ms, 'ms'))}</div>
          </div>
        </div>
        <div class="summary-grid">
          <div class="summary-main">
            <div class="card">
              <div class="card-header">
                <h3>对比说明</h3>
                <p>当前结果与最近一次同 workspace、同模式的成功任务做对比，帮助确认变化方向。</p>
              </div>
              <div class="stack">
                <div class="item">
                  <h3>当前任务</h3>
                  <div class="tag-list">
                    <span class="tag">${escapeHtml(currentPayload.workspace_path || '-')}</span>
                    <span class="tag">${escapeHtml(normalizeModeLabel(currentPayload.mode || 'cycle'))}</span>
                    <span class="tag ${currentPayload.cache_hit ? 'success' : 'warning'}">${currentPayload.cache_hit ? 'cache' : 'fresh'}</span>
                  </div>
                </div>
                <div class="item">
                  <h3>基线任务</h3>
                  <div class="tag-list">
                    <span class="tag">${escapeHtml(previousPayload.workspace_path || '-')}</span>
                    <span class="tag">${escapeHtml(normalizeModeLabel(previousPayload.mode || 'cycle'))}</span>
                    <span class="tag ${previousPayload.cache_hit ? 'success' : 'warning'}">${previousPayload.cache_hit ? 'cache' : 'fresh'}</span>
                  </div>
                </div>
              </div>
            </div>
          </div>
          <div class="summary-side">
            ${renderPerformanceCard(currentPayload)}
            ${renderPerformanceCard(previousPayload)}
          </div>
        </div>`;
    }

    function renderSummary(payload) {
      if (!payload || !payload.ok) {
        renderEmptyState('暂无结果', '先在左侧填写 workspace 和模式，然后运行一次分析。');
        return;
      }
      if (payload.mode === 'unused') {
        renderUnusedSummary(payload);
        return;
      }
      if (payload.mode === 'build-time') {
        renderBuildTimeSummary(payload);
        return;
      }
      renderCycleSummary(payload);
    }

    function renderHtmlPreview(payload) {
      if (!payload || !payload.ok || !payload.html_report) {
        renderEmptyState('没有 HTML 报告', '先执行一次分析，然后切换到 HTML 预览。');
        return;
      }
      resultsEl.innerHTML = `
        <div class="action-row">
          <button id="download-html-button" class="secondary-button" type="button">导出 HTML 报告</button>
        </div>
        <div class="preview-shell">
          <iframe title="HTML report preview"></iframe>
        </div>`;
      const iframe = resultsEl.querySelector('iframe');
      iframe.srcdoc = payload.html_report;
      document.getElementById('download-html-button').addEventListener('click', () => {
        downloadTextFile(buildDownloadFilename(payload, 'html'), payload.html_report, 'text/html;charset=utf-8');
      });
    }

    function renderJson(payload) {
      if (!payload) {
        renderEmptyState('暂无 JSON 结果', '执行分析后，这里会展示完整原始响应。');
        return;
      }
      resultsEl.innerHTML = `
        <div class="action-row">
          <button id="download-json-button" class="secondary-button" type="button">导出 JSON 结果</button>
        </div>
        <div class="caption">原始 JSON 可用于调试、导出或后续自动化处理。</div>
        <pre>${escapeHtml(JSON.stringify(payload, null, 2))}</pre>`;
      document.getElementById('download-json-button').addEventListener('click', () => {
        downloadTextFile(buildDownloadFilename(payload, 'json'), JSON.stringify(payload, null, 2), 'application/json;charset=utf-8');
      });
    }

    async function refreshView() {
      setToolbarState(currentView);
      updateResultsMeta(latestPayload);
      if (currentView === 'compare') {
        try {
          const baseline = await ensureComparePayload(latestPayload);
          renderCompareSummary(latestPayload, baseline ? baseline.result : null);
        } catch (error) {
          renderEmptyState('结果对比加载失败', error && error.message ? error.message : String(error));
        }
        return;
      }
      if (currentView === 'html') {
        renderHtmlPreview(latestPayload);
        return;
      }
      if (currentView === 'json') {
        renderJson(latestPayload);
        return;
      }
      renderSummary(latestPayload);
    }

    function setMode(mode) {
      modeSelectEl.value = mode;
      modeButtons.forEach((button) => {
        button.classList.toggle('active', button.dataset.mode === mode);
      });
      if (preferencesReady) {
        savePreferences();
      }
    }

    modeButtons.forEach((button) => {
      button.addEventListener('click', () => setMode(button.dataset.mode));
    });

    [workspaceInputEl, bazelInputEl, includeTestsEl, forceRefreshEl].forEach((element) => {
      element.addEventListener('change', () => {
        if (preferencesReady) {
          savePreferences();
        }
      });
    });

    toolbarButtons.summary.addEventListener('click', () => {
      currentView = 'summary';
      refreshView();
    });
    toolbarButtons.compare.addEventListener('click', () => {
      currentView = 'compare';
      refreshView();
    });
    toolbarButtons.html.addEventListener('click', () => {
      currentView = 'html';
      refreshView();
    });
    toolbarButtons.json.addEventListener('click', () => {
      currentView = 'json';
      refreshView();
    });

    resetButton.addEventListener('click', () => {
      stopPolling();
      workspaceInputEl.value = '';
      bazelInputEl.value = 'bazel';
      includeTestsEl.checked = false;
      forceRefreshEl.checked = false;
      setMode('cycle');
      latestPayload = null;
      latestPayloadTaskId = '';
      currentView = 'summary';
      setStatus('表单已重置。', '');
      refreshView();
      refreshCacheStatus();
      taskFilters = { q: '', mode: '', status: '', limit: 8, offset: 0 };
      refreshTaskHistory(true);
      savePreferences();
      renderPresetPanel();
    });

    savePresetButtonEl.addEventListener('click', () => {
      saveCurrentAsPreset();
    });
    closeTaskDrawerButtonEl.addEventListener('click', () => {
      closeTaskDrawer();
    });
    taskDrawerBackdropEl.addEventListener('click', () => {
      closeTaskDrawer();
    });

    async function pollTask(taskId) {
      try {
        activeTaskPollAttempts += 1;
        const response = await fetch(`/api/tasks/${encodeURIComponent(taskId)}`);
        const data = await response.json();
        if (!response.ok || !data.ok) {
          throw new Error(data.error || '任务查询失败');
        }

        if (data.status === 'completed') {
          stopPolling();
          if (data.result_ready) {
            const resultResponse = await fetch(`/api/tasks/${encodeURIComponent(taskId)}?include_result=1`);
            const resultData = await resultResponse.json();
            if (!resultResponse.ok || !resultData.ok || !resultData.result) {
              throw new Error(resultData.error || resultData.message || '读取分析结果失败');
            }
            latestPayload = resultData.result;
            latestPayloadTaskId = taskId;
            comparePayload = null;
          } else {
            latestPayload = { ok: false, error: '任务已完成，但结果不可用。' };
            latestPayloadTaskId = '';
          }
          currentView = 'summary';
          setStatus('分析完成。', 'success');
          refreshView();
          refreshCacheStatus();
          refreshTaskHistory(true);
          runButton.disabled = false;
          runButton.textContent = '运行分析';
          return;
        }

        if (data.status === 'failed') {
          throw new Error(data.message || '分析失败');
        }

        setStatus(data.message || '分析中，请稍候…', 'loading');
        activePollTimer = setTimeout(() => pollTask(taskId), nextPollDelay());
      } catch (error) {
        stopPolling();
        const message = error && error.message ? error.message : String(error);
        setStatus(message, 'error');
        latestPayload = { ok: false, error: message };
        latestPayloadTaskId = '';
        refreshView();
        runButton.disabled = false;
        runButton.textContent = '运行分析';
      }
    }

    async function submitAnalysis(payload) {
      stopPolling();
      runButton.disabled = true;
      runButton.textContent = '分析中…';
      setStatus('分析中，请稍候…', 'loading');
      savePreferences();

      try {
        const response = await fetch('/api/analyze', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload)
        });

        const data = await response.json();
        if (!response.ok) {
          throw new Error(data.error || '分析失败');
        }

        if (response.status === 202 && data.task_id) {
          activeTaskPollAttempts = 0;
          latestPayload = null;
          latestPayloadTaskId = '';
          renderEmptyState('分析任务已启动', '后台正在执行分析，结果会在完成后自动刷新。');
          setStatus(data.message || '任务已加入后台队列', 'loading');
          activePollTimer = setTimeout(() => pollTask(data.task_id), nextPollDelay());
          return;
        }

        if (!data.ok) {
          throw new Error(data.error || '分析失败');
        }

        latestPayload = data;
        latestPayloadTaskId = data.task_id || '';
        comparePayload = null;
        currentView = 'summary';
        pushRecentWorkspace(payload.workspace_path);
        setStatus(`分析完成：${normalizeModeLabel(data.mode)} · ${data.workspace_path}${data.cache_hit ? ' · 已命中缓存' : ''}`, 'success');
        refreshView();
        refreshTaskHistory(true);
      } catch (error) {
        const message = error && error.message ? error.message : String(error);
        setStatus(message, 'error');
        latestPayload = { ok: false, error: message };
        latestPayloadTaskId = '';
        refreshView();
      } finally {
        if (!activePollTimer) {
          runButton.disabled = false;
          runButton.textContent = '运行分析';
        }
      }
    }

    formEl.addEventListener('submit', async (event) => {
      event.preventDefault();
      await submitAnalysis({
        ...buildCurrentFormPayload()
      });
    });

    loadPreferences();
    if (!modeSelectEl.value) {
      setMode('cycle');
    } else {
      modeButtons.forEach((button) => {
        button.classList.toggle('active', button.dataset.mode === modeSelectEl.value);
      });
    }
    preferencesReady = true;
    renderRecentWorkspaces();
    renderPresetPanel();
    refreshView();
    refreshCacheStatus();
    refreshTaskHistory(true);
  </script>
</body>
</html>)HTML");
}
