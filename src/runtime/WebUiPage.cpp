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
      max-width: 1460px;
      margin: 0 auto;
      padding: 20px 20px 48px;
    }
    .hero {
      position: relative;
      overflow: hidden;
      padding: 24px 26px;
      border-radius: var(--radius-xl);
      border: 1px solid var(--border);
      background:
        linear-gradient(135deg, rgba(120, 168, 255, 0.14), rgba(18, 26, 43, 0.96) 42%),
        rgba(18, 26, 43, 0.92);
      box-shadow: var(--shadow);
      margin-bottom: 16px;
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
      gap: 16px;
      align-items: stretch;
      flex-wrap: wrap;
    }
    .hero-copy {
      max-width: 780px;
    }
    .hero-kicker {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      padding: 6px 10px;
      margin-bottom: 12px;
      border-radius: 999px;
      border: 1px solid rgba(120, 168, 255, 0.2);
      background: rgba(120, 168, 255, 0.1);
      color: #d9e8ff;
      font-size: 12px;
      font-weight: 700;
      letter-spacing: .08em;
      text-transform: uppercase;
    }
    .hero h1 {
      margin: 0 0 8px;
      font-size: clamp(26px, 3.6vw, 36px);
      line-height: 1.14;
      letter-spacing: -.03em;
    }
    .hero p {
      margin: 0;
      max-width: 700px;
      color: var(--text-soft);
      line-height: 1.65;
      font-size: 14px;
    }
    .hero-badges, .hero-stats {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
    }
    .hero-badges { margin-top: 14px; }
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
      align-content: flex-start;
      max-width: 340px;
    }
    .hero-stat {
      min-width: 150px;
      padding: 12px 14px;
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
      margin-top: 6px;
      font-size: 18px;
      font-weight: 800;
      line-height: 1.3;
    }
    .layout {
      display: grid;
      grid-template-columns: minmax(360px, 460px) minmax(0, 1fr);
      gap: 16px;
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
      min-height: 720px;
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
    .form-section {
      display: grid;
      gap: 14px;
      padding: 16px;
      border-radius: 18px;
      border: 1px solid rgba(255,255,255,.06);
      background: rgba(255,255,255,.028);
    }
    .form-section-head {
      display: flex;
      justify-content: space-between;
      gap: 12px;
      align-items: flex-start;
      flex-wrap: wrap;
    }
    .form-section-head strong {
      display: block;
      color: var(--text);
      font-size: 14px;
    }
    .form-section-head span {
      display: block;
      margin-top: 4px;
      color: var(--muted);
      font-size: 12px;
      line-height: 1.55;
      max-width: 420px;
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
      padding: 12px 14px;
      border: 1px solid var(--border);
      border-radius: 16px;
      background: rgba(255,255,255,.025);
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
    .action-row.compact {
      gap: 8px;
      margin-top: 0;
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
      margin-top: 14px;
    }
    .support-stack {
      display: grid;
      gap: 12px;
      margin-top: 14px;
    }
    .helper-card {
      padding: 12px 14px;
      border-radius: 16px;
      border: 1px solid var(--border);
      background: rgba(255,255,255,.03);
    }
    .helper-card.secondary {
      background: rgba(255,255,255,.024);
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
    .workspace-cluster {
      display: grid;
      gap: 8px;
      margin-top: 8px;
    }
    .workspace-cluster-label {
      color: var(--muted);
      font-size: 12px;
      line-height: 1.4;
    }
    .workspace-chip-row {
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
    }
    .workspace-chip {
      padding: 9px 12px;
      border-radius: 12px;
      border: 1px solid rgba(255,255,255,.08);
      background: rgba(255,255,255,.04);
      color: var(--text-soft);
      font-size: 12px;
      line-height: 1.2;
    }
    .workspace-chip.favorite {
      border-color: rgba(251,191,36,.22);
      background: rgba(251,191,36,.08);
      color: #ffe7a9;
    }
    .workspace-chip.active {
      border-color: rgba(120,168,255,.28);
      background: rgba(120,168,255,.14);
      color: #edf4ff;
    }
    .preset-card {
      display: grid;
      gap: 10px;
      padding: 14px;
      border-radius: 16px;
      border: 1px solid rgba(255,255,255,.06);
      background: rgba(8,14,25,.26);
    }
    .preset-card-head {
      display: flex;
      justify-content: space-between;
      gap: 10px;
      align-items: flex-start;
      flex-wrap: wrap;
    }
    .preset-card-head strong {
      font-size: 14px;
      color: var(--text);
    }
    .preset-card-head span {
      margin-top: 4px;
      color: var(--text-soft);
      font-size: 12px;
      line-height: 1.5;
      word-break: break-word;
    }
    .preset-actions {
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
    }
    .preset-actions button {
      padding: 9px 12px;
      border-radius: 12px;
    }
    .drawer-meta {
      display: grid;
      gap: 6px;
    }
    .drawer-action-grid {
      display: grid;
      gap: 12px;
      margin-top: 12px;
    }
    .drawer-action-group {
      display: grid;
      gap: 8px;
    }
    .drawer-action-group strong {
      font-size: 13px;
      color: var(--text);
    }
    .drawer-action-group span {
      color: var(--muted);
      font-size: 12px;
      line-height: 1.5;
    }
    .task-history-surface {
      margin-top: 16px;
    }
    .task-history-card {
      padding: 18px;
      border-radius: 20px;
      border: 1px solid rgba(120, 168, 255, 0.14);
      background:
        linear-gradient(180deg, rgba(120, 168, 255, 0.08), rgba(255,255,255,.02) 56%),
        rgba(255,255,255,.025);
    }
    .task-history-head {
      display: flex;
      justify-content: space-between;
      gap: 12px;
      align-items: flex-start;
      flex-wrap: wrap;
    }
    .task-history-head strong {
      font-size: 16px;
    }
    .task-history-head span {
      display: block;
      margin-top: 6px;
      color: var(--muted);
      font-size: 13px;
      line-height: 1.6;
      max-width: 640px;
    }
    .glance-strip {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      margin-top: 14px;
    }
    .glance-pill {
      min-width: 120px;
      padding: 10px 12px;
      border-radius: 14px;
      border: 1px solid rgba(255,255,255,.06);
      background: rgba(8,14,25,.28);
    }
    .glance-pill .label {
      display: block;
      color: var(--muted);
      font-size: 11px;
      text-transform: uppercase;
      letter-spacing: .08em;
    }
    .glance-pill .value {
      display: block;
      margin-top: 6px;
      color: var(--text);
      font-size: 18px;
      font-weight: 800;
    }
    .task-section {
      margin-top: 14px;
      padding-top: 14px;
      border-top: 1px solid rgba(255,255,255,.06);
    }
    .task-section-head {
      display: flex;
      justify-content: space-between;
      gap: 10px;
      align-items: baseline;
      flex-wrap: wrap;
      margin-bottom: 10px;
    }
    .task-section-head strong {
      font-size: 14px;
      color: var(--text);
    }
    .task-section-head span {
      color: var(--muted);
      font-size: 12px;
    }
    .task-list {
      display: grid;
      gap: 10px;
    }
    .task-entry {
      display: grid;
      gap: 10px;
      padding: 14px;
      border-radius: 16px;
      border: 1px solid rgba(255,255,255,.06);
      background: rgba(8,14,25,.28);
    }
    .task-entry-head {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 10px;
      flex-wrap: wrap;
    }
    .task-entry-title {
      display: flex;
      align-items: center;
      gap: 8px;
      flex-wrap: wrap;
    }
    .task-entry-title strong {
      font-size: 15px;
      color: var(--text);
    }
    .task-entry-path {
      color: var(--text-soft);
      font-size: 13px;
      line-height: 1.55;
      word-break: break-word;
    }
    .task-entry-meta {
      color: var(--muted);
      font-size: 12px;
      line-height: 1.6;
    }
    .task-entry-actions {
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
    }
    .task-entry-actions button {
      padding: 9px 12px;
      border-radius: 12px;
    }
    .task-entry-actions .primary-lite-button {
      background: linear-gradient(135deg, rgba(120,168,255,.24), rgba(79,143,255,.18));
      border: 1px solid rgba(120,168,255,.28);
      color: #eff5ff;
    }
    .results-head {
      display: flex;
      justify-content: space-between;
      align-items: flex-start;
      gap: 12px;
      flex-wrap: wrap;
      margin-bottom: 16px;
    }
    .results-toolbar-wrap {
      display: grid;
      gap: 10px;
      justify-items: end;
    }
    .toolbar-copy {
      color: var(--muted);
      font-size: 12px;
      line-height: 1.5;
      text-align: right;
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
    .insight-banner {
      display: flex;
      justify-content: space-between;
      gap: 16px;
      align-items: flex-start;
      flex-wrap: wrap;
      padding: 16px 18px;
      border-radius: 18px;
      border: 1px solid rgba(255,255,255,.08);
      background: rgba(255,255,255,.03);
    }
    .insight-banner strong {
      display: block;
      margin-bottom: 4px;
      font-size: 15px;
    }
    .insight-banner span {
      color: var(--muted);
      line-height: 1.6;
      font-size: 13px;
      max-width: 760px;
    }
    .insight-banner.success {
      border-color: rgba(52,211,153,.2);
      background: rgba(52,211,153,.08);
    }
    .insight-banner.warning {
      border-color: rgba(251,191,36,.22);
      background: rgba(251,191,36,.08);
    }
    .insight-banner.danger {
      border-color: rgba(248,113,113,.22);
      background: rgba(248,113,113,.08);
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
    .empty-state-actions {
      display: flex;
      justify-content: center;
      flex-wrap: wrap;
      gap: 10px;
      margin-top: 16px;
    }
    .metric-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
      gap: 10px;
    }
    .metric-card {
      padding: 14px 16px;
      border-radius: 16px;
      border: 1px solid var(--border);
      background: linear-gradient(180deg, rgba(255,255,255,.045), rgba(255,255,255,.02));
    }
    .metric-card .label {
      color: var(--muted);
      font-size: 12px;
      letter-spacing: .08em;
      text-transform: uppercase;
    }
    .metric-card .value {
      margin-top: 8px;
      font-size: 22px;
      font-weight: 800;
      color: var(--text);
      word-break: break-word;
    }
    .summary-grid {
      display: grid;
      grid-template-columns: minmax(0, 1.25fr) minmax(300px, .75fr);
      gap: 14px;
      margin-top: 14px;
    }
    .summary-main, .summary-side {
      display: grid;
      gap: 14px;
    }
    .card {
      border-radius: 18px;
      border: 1px solid var(--border);
      background: rgba(255,255,255,.03);
      overflow: hidden;
    }
    .card-header {
      padding: 16px 16px 0;
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
      gap: 10px;
      padding: 16px;
    }
    .item {
      padding: 14px;
      border-radius: 16px;
      border: 1px solid rgba(255,255,255,.06);
      background: rgba(8,14,25,.3);
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
      gap: 8px;
      padding: 16px;
    }
    .list-row {
      display: grid;
      grid-template-columns: minmax(0, 1fr) auto;
      gap: 12px;
      align-items: center;
      padding: 12px 14px;
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
      position: fixed;
      inset: 0;
      background: rgba(2, 6, 14, 0.52);
      border-radius: 0;
      opacity: 0;
      pointer-events: none;
      transition: opacity .18s ease;
      z-index: 30;
    }
    .drawer-backdrop.visible {
      opacity: 1;
      pointer-events: auto;
    }
    .task-drawer {
      position: fixed;
      top: 18px;
      right: 18px;
      bottom: 18px;
      width: min(440px, calc(100vw - 32px));
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
      z-index: 40;
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
      .results-toolbar-wrap {
        justify-items: start;
      }
      .toolbar-copy {
        text-align: left;
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
        <div class="hero-copy">
          <div class="hero-kicker">Local UI · Bazel Analyzer</div>
          <h1>分析工作台只负责执行与查看结果</h1>
          <p>输入 workspace 后直接分析；结果、对比、HTML 报告和原始 JSON 都集中在这里，保持页面更轻、更聚焦。</p>
          <div class="hero-badges">
            <span class="badge">Cycle / Unused / Build-time</span>
            <span class="badge">单工作台聚焦体验</span>
            <span class="badge">统一 JSON / HTML 输出</span>
          </div>
        </div>
        <div class="hero-stats">
          <div class="hero-stat">
            <div class="label">当前定位</div>
            <div class="value">本地分析工作台</div>
          </div>
          <div class="hero-stat">
            <div class="label">交互方式</div>
            <div class="value">任务驱动 · 实时预览</div>
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
          <div class="form-section">
            <div class="form-section-head">
              <div>
                <strong>基础配置</strong>
                <span>先确定 workspace 和 Bazel 路径，保证分析能正确落到目标代码库。</span>
              </div>
            </div>
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
              <div id="bazel-binary-hint" class="field-tip">正在检测本机 Bazel 环境…</div>
            </div>
          </div>

          <div class="form-section">
            <div class="form-section-head">
              <div>
                <strong>分析策略</strong>
                <span>选择本次分析关注点，并决定是否复用缓存、是否纳入测试目标。</span>
              </div>
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
          </div>

          <div class="action-row">
            <button id="run-button" class="primary-button" type="submit">运行分析</button>
            <button id="reset-button" class="secondary-button" type="button">重置表单</button>
          </div>
        </form>

        <div id="status" class="status">
          <span class="status-dot"></span>
          <span id="status-text">等待执行。</span>
        </div>

        <div class="support-stack">
          <div class="helper-card">
            <strong>分析预设</strong>
            <span>保存常用 workspace / mode / bazel / tests 组合，减少重复输入。</span>
            <div class="action-row" style="margin-top:12px;">
              <button id="save-preset-button" class="secondary-button" type="button">保存当前为预设</button>
            </div>
            <div id="preset-panel" class="stack" style="padding:12px 0 0;"></div>
          </div>
          <div class="helper-grid">
            <div id="cache-panel"></div>
            <div class="helper-card secondary">
              <strong>推荐顺序</strong>
              <span>先看结构问题，再清理依赖，最后再看 build-time 热点。</span>
            </div>
            <div class="helper-card secondary">
              <strong>结果视图</strong>
              <span>摘要适合日常浏览；HTML 用于汇报；JSON 适合调试与导出。</span>
            </div>
          </div>
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
          <div class="results-toolbar-wrap">
            <div class="toolbar-copy">默认先看摘要，需要汇报时切 HTML，需要排查时切 JSON。</div>
            <div class="toolbar">
              <button id="show-summary" type="button" class="active">摘要</button>
              <button id="show-compare" type="button">对比</button>
              <button id="show-html" type="button">HTML</button>
              <button id="show-json" type="button">JSON</button>
            </div>
          </div>
        </div>
        <div id="results" class="results"></div>
      </section>
    </div>

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
    let latestTaskHistoryById = new Map();
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
    let environmentInfo = null;
    const workspaceInputEl = document.getElementById('workspace_path');
    const bazelInputEl = document.getElementById('bazel_binary');
    const bazelBinaryHintEl = document.getElementById('bazel-binary-hint');
    const includeTestsEl = document.getElementById('include_tests');
    const forceRefreshEl = document.getElementById('force_refresh');
    const recentWorkspacesPanelEl = document.getElementById('recent-workspaces-panel');
    const presetPanelEl = document.getElementById('preset-panel');
    const savePresetButtonEl = document.getElementById('save-preset-button');
    const cachePanelEl = document.getElementById('cache-panel');
    const taskHistoryPanelEl = document.getElementById('task-history-panel');
    const taskDrawerEl = document.getElementById('task-drawer');
    const taskDrawerBodyEl = document.getElementById('task-drawer-body');
    const taskDrawerBackdropEl = document.getElementById('task-drawer-backdrop');
    const closeTaskDrawerButtonEl = document.getElementById('close-task-drawer-button');
    const preferencesStorageKey = 'bazel-deps-checker-ui-preferences-v1';
    let selectedTask = null;
    let drawerContext = null;
    let lastDrawerTriggerEl = null;
    const taskResultCache = new Map();
    const renderState = {
      recentWorkspaceKey: '',
      presetKey: '',
      taskHistoryKey: '',
      resultsMetaKey: '',
      viewKey: ''
    };

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
        <article class="task-entry">
          <div class="task-entry-head">
            <div class="task-entry-title">
              <strong>${escapeHtml(normalizeModeLabel(task.mode || 'cycle'))}</strong>
              <span class="tag ${taskStatusTone(task.status)}">${escapeHtml(task.status)}</span>
            </div>
            <span class="field-tip">${escapeHtml(task.task_id)}</span>
          </div>
          <div class="task-entry-path">${escapeHtml(task.workspace_path || task.message || '未记录 workspace')}</div>
          <div class="task-entry-meta">更新时间：${escapeHtml(formatDateTime(task.updated_at_ms))}${task.total_ms ? ` · 总耗时 ${escapeHtml(Number(task.total_ms).toFixed(2))}ms` : ''}${task.bazel_binary ? ` · ${escapeHtml(task.bazel_binary)}` : ''}</div>
          <div class="tag-list">
            ${task.include_tests ? '<span class="tag">tests</span>' : ''}
            ${task.cache_hit ? '<span class="tag success">cache</span>' : ''}
          </div>
          <div class="task-entry-actions">
            <button class="primary-lite-button task-detail-button" type="button" data-task-id="${escapeHtml(task.task_id)}">详情</button>
            <button class="secondary-button task-open-button" type="button" data-task-id="${escapeHtml(task.task_id)}" ${task.status === 'completed' ? '' : 'disabled'}>打开结果</button>
            <button class="secondary-button task-rerun-button" type="button" data-task-id="${escapeHtml(task.task_id)}">重跑</button>
            <button class="secondary-button task-apply-button" type="button" data-task-id="${escapeHtml(task.task_id)}">载入配置</button>
          </div>
        </article>`;
    }

    function getTaskById(taskId) {
      if (!taskId) {
        return null;
      }
      return latestTaskHistoryById.get(taskId) || latestTaskHistory.find((item) => item.task_id === taskId) || null;
    }

    function invalidateViewRenderCache() {
      renderState.viewKey = '';
      renderState.resultsMetaKey = '';
    }

    function closeTaskDrawer() {
      selectedTask = null;
      drawerContext = null;
      taskDrawerEl.classList.remove('visible');
      taskDrawerBackdropEl.classList.remove('visible');
      taskDrawerEl.setAttribute('aria-hidden', 'true');
      if (lastDrawerTriggerEl && typeof lastDrawerTriggerEl.focus === 'function') {
        lastDrawerTriggerEl.focus();
      }
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
      window.setTimeout(() => closeTaskDrawerButtonEl?.focus(), 0);
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
      drawerContext = { task, resultPayload, baselineTask };

      taskDrawerBodyEl.innerHTML = `
        <div class="item">
          <div class="drawer-meta">
            <strong>${escapeHtml(normalizeModeLabel(task.mode || 'cycle'))}</strong>
            <span>${escapeHtml(task.workspace_path || '未记录 workspace')}</span>
          </div>
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
          <span>主动作放前面，导出动作放后面，减少视觉干扰。</span>
          <div class="drawer-action-grid">
            <div class="drawer-action-group">
              <strong>继续分析</strong>
              <span>复用任务配置、重新执行，或把结果切到主视图继续浏览。</span>
              <div class="action-row compact">
                <button class="secondary-button drawer-apply-button" type="button">载入配置</button>
                <button class="secondary-button drawer-rerun-button" type="button">重新运行</button>
                <button class="secondary-button drawer-open-button" type="button" ${task.status === 'completed' ? '' : 'disabled'}>打开结果</button>
                <button class="secondary-button drawer-compare-button" type="button" ${task.status === 'completed' ? '' : 'disabled'}>查看对比</button>
              </div>
            </div>
            <div class="drawer-action-group">
              <strong>分享与导出</strong>
              <span>复制摘要，或把当前任务导出为 JSON / Markdown 快照。</span>
              <div class="action-row compact">
                <button class="secondary-button drawer-copy-button" type="button">复制摘要</button>
                <button class="secondary-button drawer-export-json-button" type="button">导出 JSON</button>
                <button class="secondary-button drawer-export-md-button" type="button">导出 Markdown</button>
              </div>
            </div>
          </div>
        </div>`;
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
      if (taskResultCache.has(taskId)) {
        return taskResultCache.get(taskId);
      }
      const response = await fetch(`/api/tasks/${encodeURIComponent(taskId)}?include_result=1`);
      const data = await response.json();
      if (!response.ok || !data.ok || !data.result) {
        throw new Error(data.error || data.message || '读取任务结果失败');
      }
      taskResultCache.set(taskId, data.result);
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
      const presetKey = analysisPresets
        .map((preset) => `${preset.id}|${preset.name}|${preset.workspace_path}|${preset.mode}|${preset.last_used_ms}|${preset.include_tests ? 1 : 0}|${preset.force_refresh ? 1 : 0}`)
        .join('~');
      if (renderState.presetKey === presetKey) {
        return;
      }
      if (!analysisPresets.length) {
        presetPanelEl.innerHTML = `
          <div class="item" style="padding:12px;">
            <strong>暂无预设</strong>
            <span>先配置好参数，再点击“保存当前为预设”。</span>
          </div>`;
        renderState.presetKey = presetKey;
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
        <div class="preset-card">
          <div class="preset-card-head">
            <div>
              <strong>${escapeHtml(preset.name)}</strong>
              <span>${escapeHtml(preset.workspace_path)}</span>
            </div>
          </div>
          <div class="tag-list">
            <span class="tag">${escapeHtml(normalizeModeLabel(preset.mode))}</span>
            <span class="tag">${escapeHtml(preset.bazel_binary || 'bazel')}</span>
            ${preset.include_tests ? '<span class="tag">tests</span>' : ''}
            ${preset.force_refresh ? '<span class="tag warning">force</span>' : ''}
            ${preset.last_used_ms ? `<span class="tag">最近使用：${escapeHtml(formatDateTime(preset.last_used_ms))}</span>` : '<span class="tag">未使用</span>'}
          </div>
          <div class="preset-actions">
            <button class="secondary-button preset-run-button" type="button" data-preset-id="${escapeHtml(preset.id)}">一键运行</button>
            <button class="secondary-button preset-apply-button" type="button" data-preset-id="${escapeHtml(preset.id)}">载入预设</button>
            <button class="secondary-button preset-delete-button" type="button" data-preset-id="${escapeHtml(preset.id)}">删除</button>
          </div>
        </div>`).join('');
      renderState.presetKey = presetKey;
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
      const recentWorkspaceKey = [
        workspaceInputEl.value.trim(),
        favoriteWorkspaces.join('|'),
        recentWorkspaces.join('|')
      ].join('||');
      if (renderState.recentWorkspaceKey === recentWorkspaceKey) {
        return;
      }
      if (!recentWorkspaces.length && !favoriteWorkspaces.length) {
        recentWorkspacesPanelEl.innerHTML = `
          <div class="action-row" style="margin-top:8px;">
            <button class="secondary-button favorite-current-workspace-button" type="button">收藏当前工作区</button>
          </div>`;
        renderState.recentWorkspaceKey = recentWorkspaceKey;
        return;
      }
      recentWorkspacesPanelEl.innerHTML = `
        <div class="stack" style="padding:8px 0 0;">
          <div class="action-row" style="margin-top:0;">
            <button class="secondary-button favorite-current-workspace-button" type="button">${isFavoriteWorkspace(workspaceInputEl.value) ? '取消收藏当前工作区' : '收藏当前工作区'}</button>
          </div>
          ${favoriteWorkspaces.length ? `
            <div class="workspace-cluster">
              <span class="workspace-cluster-label">收藏工作区</span>
              <div class="workspace-chip-row">
              ${favoriteWorkspaces.map((workspace) => `
                <button class="secondary-button workspace-chip favorite ${workspace === workspaceInputEl.value.trim() ? 'active' : ''}" type="button" data-workspace="${escapeHtml(workspace)}" title="${escapeHtml(workspace)}">★ ${escapeHtml(workspace.split('/').filter(Boolean).pop() || workspace)}</button>
              `).join('')}
              </div>
            </div>` : ''}
          ${recentWorkspaces.length ? `
            <div class="workspace-cluster">
              <span class="workspace-cluster-label">最近工作区</span>
              <div class="workspace-chip-row">
              ${recentWorkspaces.map((workspace) => `
                <button class="secondary-button workspace-chip ${workspace === workspaceInputEl.value.trim() ? 'active' : ''}" type="button" data-workspace="${escapeHtml(workspace)}" title="${escapeHtml(workspace)}">${escapeHtml(workspace.split('/').filter(Boolean).pop() || workspace)}</button>
              `).join('')}
              </div>
            </div>` : ''}
        </div>`;
      renderState.recentWorkspaceKey = recentWorkspaceKey;
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
        latestTaskHistoryById = new Map(latestTaskHistory.map((task) => [task.task_id, task]));
        latestTaskHistoryTotal = Number(data.total || 0);
        renderState.taskHistoryKey = '';
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
        invalidateViewRenderCache();
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
      const panel = cachePanelEl;
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
    }

    )HTML") + std::string(R"HTML(    function renderTaskHistoryPanel() {
      const panel = taskHistoryPanelEl;
      if (!panel) {
        return;
      }
      const taskHistoryKey = [
        taskFilters.q,
        taskFilters.mode,
        taskFilters.status,
        taskFilters.offset,
        latestTaskHistoryTotal,
        latestTaskHistory.map((task) => `${task.task_id}:${task.status}:${task.updated_at_ms}`).join('|')
      ].join('||');
      if (renderState.taskHistoryKey === taskHistoryKey) {
        return;
      }

      if (!latestTaskHistory.length) {
        panel.innerHTML = `
          <div class="task-history-card">
            <div class="task-history-head">
              <div>
                <strong>最近任务</strong>
                <span>这里会保留最近分析记录，方便快速复用配置、查看结果和追踪趋势。</span>
              </div>
            </div>
            <div class="empty-state" style="min-height:220px;margin-top:14px;">
              <div>
                <h3>还没有任务记录</h3>
                <p>先在上方填入 workspace 并运行一次分析，后续结果会自动沉淀到这里。</p>
              </div>
            </div>
          </div>`;
        renderState.taskHistoryKey = taskHistoryKey;
        return;
      }

      const prioritizedTasks = sortTasksForDisplay(latestTaskHistory);
      const summary = summarizeTaskHistory(prioritizedTasks);
      const activeTasks = prioritizedTasks.filter((task) => task.status === 'running' || task.status === 'queued' || task.status === 'failed');
      const recentCompletedTasks = prioritizedTasks.filter((task) => task.status === 'completed');

      panel.innerHTML = `
        <div class="task-history-card">
          <div class="task-history-head">
            <div>
              <strong>最近任务</strong>
              <span>优先关注运行中和失败任务；已完成结果可以直接复用配置、打开结果或作为趋势基线。</span>
            </div>
            <span class="field-tip">已显示 ${escapeHtml(latestTaskHistory.length)} / ${escapeHtml(latestTaskHistoryTotal)} 条</span>
          </div>
          <div class="glance-strip">
            <div class="glance-pill"><span class="label">当前列表</span><span class="value">${escapeHtml(summary.total)}</span></div>
            <div class="glance-pill"><span class="label">运行 / 排队</span><span class="value">${escapeHtml(summary.running)}</span></div>
            <div class="glance-pill"><span class="label">失败任务</span><span class="value">${escapeHtml(summary.failed)}</span></div>
            <div class="glance-pill"><span class="label">缓存命中</span><span class="value">${escapeHtml(summary.total ? Math.round((summary.cacheHit / summary.total) * 100) : 0)}%</span></div>
          </div>
          <div class="tag-list" style="margin-top:14px;">
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
          <div class="task-section">
            ${activeTasks.length ? `
              <div>
                <div class="task-section-head">
                  <strong>优先关注</strong>
                  <span>先处理运行中、排队中和失败任务，缩短反馈回路。</span>
                </div>
                <div class="task-list">
                  ${activeTasks.map((task) => buildTaskItem(task)).join('')}
                </div>
              </div>` : ''}
            ${recentCompletedTasks.length ? `
              <div class="${activeTasks.length ? 'task-section' : ''}" style="${activeTasks.length ? '' : 'margin-top:0;padding-top:0;border-top:0;'}">
                <div class="task-section-head">
                  <strong>最近完成</strong>
                  <span>${summary.slowest ? `当前列表最慢任务：${escapeHtml(normalizeWorkspaceName(summary.slowest.workspace_path))} · ${escapeHtml(Number(summary.slowest.total_ms || 0).toFixed(2))}ms` : '可直接复用或重跑成功任务配置。'}</span>
                </div>
                <div class="task-list">
                  ${recentCompletedTasks.map((task) => buildTaskItem(task)).join('')}
                </div>
              </div>` : ''}
          </div>
          <div class="action-row" style="margin-top:12px;">
            <button id="task-load-more-button" class="secondary-button" type="button" ${latestTaskHistory.length < latestTaskHistoryTotal ? '' : 'disabled'}>加载更多</button>
          </div>
        </div>`;

      const searchInput = document.getElementById('task-search-input');
      const modeFilter = document.getElementById('task-mode-filter');
      const statusFilter = document.getElementById('task-status-filter');
      if (searchInput) {
        searchInput.value = taskFilters.q;
      }
      if (modeFilter) {
        modeFilter.value = taskFilters.mode;
      }
      if (statusFilter) {
        statusFilter.value = taskFilters.status;
      }
      renderState.taskHistoryKey = taskHistoryKey;
    }

    function updateResultsMeta(payload) {
      const metaKey = payload && payload.ok
        ? `${payload.mode}|${payload.workspace_path}|${payload.include_tests ? 1 : 0}|${payload.cache_hit ? 1 : 0}`
        : 'empty';
      if (renderState.resultsMetaKey === metaKey) {
        return;
      }
      if (!payload || !payload.ok) {
        resultsMetaEl.innerHTML = '<span class="mini-chip">尚未执行成功分析</span>';
        renderState.resultsMetaKey = metaKey;
        return;
      }

      const chips = [
        `模式：${normalizeModeLabel(payload.mode)}`,
        `Workspace：${payload.workspace_path || '-'}`,
        payload.include_tests ? '包含测试目标' : '不含测试目标',
        payload.cache_hit ? '结果来自缓存' : '结果为实时分析'
      ];
      resultsMetaEl.innerHTML = chips.map((item) => `<span class="mini-chip">${escapeHtml(item)}</span>`).join('');
      renderState.resultsMetaKey = metaKey;
    }

    function renderMetric(label, value) {
      return `<div class="metric-card"><div class="label">${escapeHtml(label)}</div><div class="value">${escapeHtml(value)}</div></div>`;
    }

    function renderBazelEnvironmentHint() {
      if (!bazelBinaryHintEl) {
        return;
      }
      if (!environmentInfo) {
        bazelBinaryHintEl.textContent = '正在检测本机 Bazel 环境…';
        return;
      }
      if (environmentInfo.bazel_available) {
        const detected = Array.isArray(environmentInfo.detected_bazel_binaries)
          ? environmentInfo.detected_bazel_binaries
          : [];
        const recommended = environmentInfo.recommended_bazel_binary || '';
        bazelBinaryHintEl.textContent = recommended
          ? `已检测到 Bazel：${recommended}${detected.length > 1 ? `（另有 ${detected.length - 1} 个候选）` : ''}`
          : '已检测到可用 Bazel 环境。';
        return;
      }
      bazelBinaryHintEl.textContent = '未检测到可用 Bazel；请安装 bazel / bazelisk，或在此填写 Bazel 可执行文件绝对路径。';
    }

    async function refreshEnvironmentInfo() {
      try {
        const response = await fetch('/api/environment');
        const data = await response.json();
        if (!response.ok || !data.ok) {
          return;
        }
        environmentInfo = data;
        if (data.recommended_bazel_binary &&
            (!bazelInputEl.value.trim() || bazelInputEl.value.trim() === 'bazel')) {
          bazelInputEl.value = data.recommended_bazel_binary;
        }
        renderBazelEnvironmentHint();
      } catch (error) {
        environmentInfo = {
          bazel_available: false,
          detected_bazel_binaries: [],
          recommended_bazel_binary: ''
        };
        renderBazelEnvironmentHint();
      }
    }

    function renderInsightBanner(title, description, tone = '') {
      return `
        <div class="insight-banner ${escapeHtml(tone)}">
          <div>
            <strong>${escapeHtml(title)}</strong>
            <span>${escapeHtml(description)}</span>
          </div>
        </div>`;
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
      const latestCompletedTask = sortTasksForDisplay(latestTaskHistory).find((task) => task.status === 'completed');
      const canRun = Boolean(workspaceInputEl.value.trim());
      resultsEl.innerHTML = `
        <div class="empty-state">
          <div>
            <h3>${escapeHtml(title)}</h3>
            <p>${escapeHtml(description)}</p>
            <div class="empty-state-actions">
              <button class="secondary-button empty-run-button" type="button" ${canRun ? '' : 'disabled'}>运行当前模式</button>
              <button class="secondary-button empty-open-latest-button" type="button" ${latestCompletedTask ? '' : 'disabled'}>打开最近结果</button>
            </div>
          </div>
        </div>`;
    }

    function renderCycleSummary(payload) {
      const report = payload.report.report || {};
      const cycles = report.cycles || [];
      const totalCycles = Number(report.total_cycles || 0);
      const metricHtml = `
        <div class="metric-grid">
          ${renderMetric('模式', '循环依赖')}
          ${renderMetric('循环数量', report.total_cycles ?? 0)}
          ${renderMetric('Workspace', payload.workspace_path || '-')}
        </div>`;
      const insightHtml = totalCycles > 0
        ? renderInsightBanner('先从短环开始拆', `本次共发现 ${totalCycles} 条循环依赖，优先处理节点少、可直接移除依赖的环，治理收益最高。`, totalCycles > 3 ? 'danger' : 'warning')
        : renderInsightBanner('当前结构健康', '这次没有发现循环依赖，可以把注意力切到未使用依赖或 build-time 热点。', 'success');

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
        ${insightHtml}
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
      const totalUnused = Number(report.total_unused_dependencies || 0);
      const metricHtml = `
        <div class="metric-grid">
          ${renderMetric('模式', '未使用依赖')}
          ${renderMetric('依赖数量', report.total_unused_dependencies ?? 0)}
          ${renderMetric('高置信度', stats.high_confidence ?? 0)}
          ${renderMetric('中置信度', stats.medium_confidence ?? 0)}
          ${renderMetric('低置信度', stats.low_confidence ?? 0)}
        </div>`;
      const insightHtml = totalUnused > 0
        ? renderInsightBanner('优先清理高置信度依赖', `当前识别到 ${totalUnused} 条可疑未使用依赖，建议先处理高置信度项，并在每轮变更后补一次 Bazel 编译验证。`, Number(stats.high_confidence || 0) > 0 ? 'warning' : 'danger')
        : renderInsightBanner('依赖干净度不错', '当前规则集下没有发现可移除依赖，可以继续检查循环依赖或 build-time 数据。', 'success');

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
        ${insightHtml}
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
      const totalDuration = Number(summary.total_duration_seconds || 0);
      const metricHtml = `
        <div class="metric-grid">
          ${renderMetric('模式', '构建耗时')}
          ${renderMetric('总耗时(s)', summary.total_duration_seconds ?? 0)}
          ${renderMetric('Profile(s)', summary.generation_time_seconds ?? 0)}
          ${renderMetric('分析(s)', summary.analysis_time_seconds ?? 0)}
          ${renderMetric('关键路径', criticalPaths.length)}
          ${renderMetric('优化建议', suggestions.length)}
        </div>`;
      const insightHtml = suggestions.length
        ? renderInsightBanner('先盯最慢 phase 与关键路径', `这次构建总耗时 ${totalDuration}s，报告给出了 ${suggestions.length} 条优化建议，优先处理 execution / critical path 上的高影响项。`, totalDuration > 60 ? 'danger' : 'warning')
        : renderInsightBanner('没有明显热点', '当前报告没有给出额外优化建议，可以结合 phase stats 和关键路径做人工复核。', 'success');

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
        ${insightHtml}
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
      let compareInsightHtml = renderInsightBanner('与最近一次成功任务对比', '你可以先看核心指标变化，再结合当前分析耗时判断缓存、依赖准备和分析本身的变化。');
      if (currentPayload.mode === 'cycle') {
        const currentReport = currentPayload.report.report || {};
        const previousReport = previousPayload.report.report || {};
        const delta = Number(currentReport.total_cycles || 0) - Number(previousReport.total_cycles || 0);
        compareInsightHtml = delta > 0
          ? renderInsightBanner('循环风险上升', `和基线相比，循环数增加了 ${delta} 条，建议优先查看新增环。`, 'danger')
          : delta < 0
            ? renderInsightBanner('循环风险下降', `和基线相比，循环数减少了 ${Math.abs(delta)} 条，说明治理方向有效。`, 'success')
            : renderInsightBanner('循环规模持平', '循环数量没有变化，可以重点比较具体路径是否更短、更容易治理。');
      } else if (currentPayload.mode === 'unused') {
        const currentReport = currentPayload.report.unused_dependencies_report || {};
        const previousReport = previousPayload.report.unused_dependencies_report || {};
        const delta = Number(currentReport.total_unused_dependencies || 0) - Number(previousReport.total_unused_dependencies || 0);
        compareInsightHtml = delta > 0
          ? renderInsightBanner('待清理依赖变多', `和基线相比，未使用依赖增加了 ${delta} 条，建议优先看高置信度变化。`, 'warning')
          : delta < 0
            ? renderInsightBanner('依赖清理有效', `和基线相比，未使用依赖减少了 ${Math.abs(delta)} 条。`, 'success')
            : renderInsightBanner('依赖规模持平', '未使用依赖数量变化不大，可以结合高/中/低置信度继续判断。');
      } else {
        const currentReport = currentPayload.report.build_time_report || {};
        const previousReport = previousPayload.report.build_time_report || {};
        const delta = Number((currentReport.summary || {}).total_duration_seconds || 0) - Number((previousReport.summary || {}).total_duration_seconds || 0);
        compareInsightHtml = delta > 0
          ? renderInsightBanner('构建变慢了', `和基线相比，总耗时增加了 ${delta.toFixed(2)}s，建议优先检查 execution 与 critical path。`, 'danger')
          : delta < 0
            ? renderInsightBanner('构建速度有改善', `和基线相比，总耗时下降了 ${Math.abs(delta).toFixed(2)}s。`, 'success')
            : renderInsightBanner('构建耗时基本持平', '总耗时没有明显变化，可以继续看建议数和分析耗时变化。');
      }

      resultsEl.innerHTML = `
        ${compareInsightHtml}
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
      const payloadBaseKey = latestPayload && latestPayload.ok
        ? `${latestPayloadTaskId}|${latestPayload.mode}|${latestPayload.workspace_path}|${latestPayload.cache_hit ? 1 : 0}|${Number((latestPayload.performance || {}).total_ms || 0).toFixed(2)}`
        : 'empty';
      if (currentView === 'compare') {
        try {
          const baseline = await ensureComparePayload(latestPayload);
          const compareKey = `${currentView}|${payloadBaseKey}|${baseline ? baseline.task_id : 'none'}`;
          if (renderState.viewKey === compareKey) {
            return;
          }
          renderCompareSummary(latestPayload, baseline ? baseline.result : null);
          renderState.viewKey = compareKey;
        } catch (error) {
          renderEmptyState('结果对比加载失败', error && error.message ? error.message : String(error));
          renderState.viewKey = `${currentView}|${payloadBaseKey}|error`;
        }
        return;
      }
      if (currentView === 'html') {
        const htmlKey = `${currentView}|${payloadBaseKey}|${latestPayload && latestPayload.html_report ? latestPayload.html_report.length : 0}`;
        if (renderState.viewKey === htmlKey) {
          return;
        }
        renderHtmlPreview(latestPayload);
        renderState.viewKey = htmlKey;
        return;
      }
      if (currentView === 'json') {
        const jsonKey = `${currentView}|${payloadBaseKey}`;
        if (renderState.viewKey === jsonKey) {
          return;
        }
        renderJson(latestPayload);
        renderState.viewKey = jsonKey;
        return;
      }
      const summaryKey = `${currentView}|${payloadBaseKey}`;
      if (renderState.viewKey === summaryKey) {
        return;
      }
      renderSummary(latestPayload);
      renderState.viewKey = summaryKey;
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

    resultsEl?.addEventListener('click', (event) => {
      const button = event.target.closest('button');
      if (!button) {
        return;
      }
      if (button.classList.contains('empty-run-button')) {
        if (!workspaceInputEl.value.trim()) {
          setStatus('请先填写 workspace 路径。', 'error');
          return;
        }
        submitAnalysis({
          ...buildCurrentFormPayload()
        });
        return;
      }
      if (button.classList.contains('empty-open-latest-button')) {
        const latestCompletedTask = sortTasksForDisplay(latestTaskHistory).find((task) => task.status === 'completed');
        if (!latestCompletedTask) {
          setStatus('当前没有可打开的历史成功任务。', 'error');
          return;
        }
        openTaskResult(latestCompletedTask.task_id);
      }
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
      comparePayload = null;
      invalidateViewRenderCache();
      currentView = 'summary';
      setStatus('表单已重置。', '');
      refreshView();
      refreshCacheStatus();
      taskFilters = { q: '', mode: '', status: '', limit: 8, offset: 0 };
      refreshTaskHistory(true);
      savePreferences();
      renderPresetPanel();
      workspaceInputEl.focus();
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

    cachePanelEl?.addEventListener('click', async (event) => {
      const button = event.target.closest('button');
      if (!button) {
        return;
      }
      if (button.id === 'refresh-cache-button') {
        refreshCacheStatus();
        return;
      }
      if (button.id === 'clear-cache-button') {
        try {
          const response = await fetch('/api/cache/clear', { method: 'POST' });
          const data = await response.json();
          if (!response.ok || !data.ok) {
            throw new Error(data.error || '清空缓存失败');
          }
          setStatus('缓存已清空。', 'success');
          latestPayload = null;
          latestPayloadTaskId = '';
          comparePayload = null;
          taskResultCache.clear();
          invalidateViewRenderCache();
          refreshView();
          await refreshCacheStatus();
          await refreshTaskHistory();
        } catch (error) {
          const message = error && error.message ? error.message : String(error);
          setStatus(message, 'error');
        }
      }
    });

    recentWorkspacesPanelEl?.addEventListener('click', (event) => {
      const button = event.target.closest('button');
      if (!button) {
        return;
      }
      if (button.classList.contains('favorite-current-workspace-button')) {
        toggleFavoriteWorkspace(workspaceInputEl.value);
        return;
      }
      if (button.classList.contains('favorite-workspace-button')) {
        workspaceInputEl.value = button.dataset.workspace || '';
        savePreferences();
        renderRecentWorkspaces();
        setStatus(`已选中收藏工作区：${workspaceInputEl.value}`, 'success');
        return;
      }
      if (button.classList.contains('recent-workspace-button')) {
        workspaceInputEl.value = button.dataset.workspace || '';
        savePreferences();
        renderRecentWorkspaces();
        setStatus(`已选中最近工作区：${workspaceInputEl.value}`, 'success');
      }
    });

    presetPanelEl?.addEventListener('click', async (event) => {
      const button = event.target.closest('button');
      if (!button) {
        return;
      }
      const presetId = button.dataset.presetId;
      const preset = analysisPresets.find((item) => item.id === presetId);
      if (!preset) {
        return;
      }
      if (button.classList.contains('preset-apply-button')) {
        applyTaskToForm(preset);
        forceRefreshEl.checked = Boolean(preset.force_refresh);
        savePreferences();
        renderPresetPanel();
        setStatus(`已载入预设：${preset.name}`, 'success');
        return;
      }
      if (button.classList.contains('preset-run-button')) {
        await runPreset(preset);
        return;
      }
      if (button.classList.contains('preset-delete-button')) {
        analysisPresets = analysisPresets.filter((item) => item.id !== presetId);
        savePreferences();
        renderPresetPanel();
        setStatus(`已删除预设：${preset.name}`, 'success');
      }
    });

    taskHistoryPanelEl?.addEventListener('input', (event) => {
      const target = event.target;
      if (!target || target.id !== 'task-search-input') {
        return;
      }
      taskFilters.q = target.value.trim();
      if (taskSearchDebounceTimer) {
        clearTimeout(taskSearchDebounceTimer);
      }
      taskSearchDebounceTimer = setTimeout(() => {
        refreshTaskHistory(true);
      }, 220);
    });

    taskHistoryPanelEl?.addEventListener('change', (event) => {
      const target = event.target;
      if (!target) {
        return;
      }
      if (target.id === 'task-mode-filter') {
        taskFilters.mode = target.value;
        refreshTaskHistory(true);
        return;
      }
      if (target.id === 'task-status-filter') {
        taskFilters.status = target.value;
        refreshTaskHistory(true);
      }
    });

    taskHistoryPanelEl?.addEventListener('click', (event) => {
      const button = event.target.closest('button, .task-filter-shortcut');
      if (!button) {
        return;
      }
      if (button.classList.contains('task-filter-shortcut')) {
        taskFilters.status = button.dataset.status || '';
        refreshTaskHistory(true);
        return;
      }
      if (button.id === 'task-load-more-button') {
        taskFilters.offset += taskFilters.limit;
        refreshTaskHistory();
        return;
      }
      const taskId = button.dataset.taskId;
      if (!taskId) {
        return;
      }
      if (button.classList.contains('task-open-button')) {
        openTaskResult(taskId);
        return;
      }
      if (button.classList.contains('task-detail-button')) {
        lastDrawerTriggerEl = button;
        openTaskDrawer(taskId);
        return;
      }
      if (button.classList.contains('task-apply-button')) {
        const task = getTaskById(taskId);
        applyTaskToForm(task);
        setStatus(`已载入任务 ${taskId} 的配置。`, 'success');
        return;
      }
      if (button.classList.contains('task-rerun-button')) {
        const task = getTaskById(taskId);
        if (!task) {
          setStatus('未找到任务配置，无法重新运行。', 'error');
          return;
        }
        rerunTask(task);
      }
    });

    document.addEventListener('keydown', (event) => {
      if (event.key === 'Escape' && taskDrawerEl.classList.contains('visible')) {
        closeTaskDrawer();
        return;
      }
      if ((event.metaKey || event.ctrlKey) && event.key === 'Enter') {
        if (runButton.disabled) {
          return;
        }
        if (!workspaceInputEl.value.trim()) {
          setStatus('请先填写 workspace 路径。', 'error');
          workspaceInputEl.focus();
          return;
        }
        event.preventDefault();
        submitAnalysis({
          ...buildCurrentFormPayload()
        });
      }
    });

    taskDrawerBodyEl?.addEventListener('click', async (event) => {
      const button = event.target.closest('button');
      if (!button || !drawerContext || !drawerContext.task) {
        return;
      }
      const { task, resultPayload, baselineTask } = drawerContext;
      if (button.classList.contains('drawer-apply-button')) {
        applyTaskToForm(task);
        setStatus(`已从任务详情载入配置：${task.task_id}`, 'success');
        return;
      }
      if (button.classList.contains('drawer-rerun-button')) {
        rerunTask(task);
        return;
      }
      if (button.classList.contains('drawer-open-button')) {
        openTaskResult(task.task_id);
        return;
      }
      if (button.classList.contains('drawer-compare-button')) {
        await openTaskResult(task.task_id);
        currentView = 'compare';
        refreshView();
        return;
      }
      if (button.classList.contains('drawer-copy-button')) {
        try {
          const markdown = buildTaskSnapshotMarkdown(task, resultPayload, baselineTask);
          await copyTextToClipboard(markdown);
          setStatus(`已复制任务摘要：${task.task_id}`, 'success');
        } catch (error) {
          setStatus(error && error.message ? error.message : '复制任务摘要失败', 'error');
        }
        return;
      }
      if (button.classList.contains('drawer-export-json-button')) {
        const snapshot = buildTaskSnapshot(task, resultPayload, baselineTask);
        downloadTextFile(
          `${normalizeWorkspaceName(task.workspace_path)}-${task.task_id}-snapshot.json`,
          JSON.stringify(snapshot, null, 2),
          'application/json;charset=utf-8');
        setStatus(`已导出任务快照 JSON：${task.task_id}`, 'success');
        return;
      }
      if (button.classList.contains('drawer-export-md-button')) {
        const markdown = buildTaskSnapshotMarkdown(task, resultPayload, baselineTask);
        downloadTextFile(
          `${normalizeWorkspaceName(task.workspace_path)}-${task.task_id}-snapshot.md`,
          markdown,
          'text/markdown;charset=utf-8');
        setStatus(`已导出任务快照 Markdown：${task.task_id}`, 'success');
      }
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
            invalidateViewRenderCache();
          } else {
            latestPayload = { ok: false, error: '任务已完成，但结果不可用。' };
            latestPayloadTaskId = '';
            comparePayload = null;
            invalidateViewRenderCache();
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
        comparePayload = null;
        invalidateViewRenderCache();
        refreshView();
        runButton.disabled = false;
        runButton.textContent = '运行分析';
      }
    }

    async function submitAnalysis(payload) {
      const normalizedBazelBinary = String(payload.bazel_binary || '').trim();
      const usingImplicitBazelCommand =
        !normalizedBazelBinary ||
        normalizedBazelBinary === 'bazel' ||
        normalizedBazelBinary === 'bazelisk';
      if (environmentInfo && !environmentInfo.bazel_available && usingImplicitBazelCommand) {
        renderBazelEnvironmentHint();
        setStatus('当前机器未检测到 Bazel。请先安装 bazel / bazelisk，或在“Bazel 路径”中填写可执行文件绝对路径。', 'error');
        bazelInputEl.focus();
        return;
      }
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
          comparePayload = null;
          invalidateViewRenderCache();
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
        invalidateViewRenderCache();
        currentView = 'summary';
        pushRecentWorkspace(payload.workspace_path);
        setStatus(`分析完成：${normalizeModeLabel(data.mode)} · ${data.workspace_path}${data.cache_hit ? ' · 已命中缓存' : ''}`, 'success');
        refreshView();
        refreshTaskHistory(true);
      } catch (error) {
        const rawMessage = error && error.message ? error.message : String(error);
        const message = rawMessage.includes('Bazel binary is not executable or not found')
          ? `${rawMessage}。请先安装 bazel / bazelisk，或在“Bazel 路径”中填写可执行文件绝对路径。`
          : rawMessage;
        setStatus(message, 'error');
        latestPayload = { ok: false, error: message };
        latestPayloadTaskId = '';
        comparePayload = null;
        invalidateViewRenderCache();
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
    renderBazelEnvironmentHint();
    renderPresetPanel();
    refreshView();
    refreshEnvironmentInfo();
    refreshCacheStatus();
    refreshTaskHistory(true);
  </script>
</body>
</html>)HTML");
}
