#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_OUTPUT="${BUILD_OUTPUT:-/tmp/bazel-deps-analyzer}"
PORT="${PORT:-18092}"
WORKSPACE_PATH="${WORKSPACE_PATH:-$ROOT_DIR}"

cleanup() {
  if [[ -n "${SERVER_PID:-}" ]]; then
    kill "${SERVER_PID}" >/dev/null 2>&1 || true
    wait "${SERVER_PID}" >/dev/null 2>&1 || true
  fi
}

trap cleanup EXIT

echo "[1/4] Compiling binary..."
clang++ -std=c++17 -Wall -Wextra -Wpedantic \
  -Isrc -Isrc/common -Isrc/core -Isrc/runtime -Ithirds \
  $(find "$ROOT_DIR/src" -name '*.cpp' | tr '\n' ' ') \
  -o "$BUILD_OUTPUT"

echo "[2/4] Starting Web UI on port ${PORT}..."
"$BUILD_OUTPUT" --ui --port "$PORT" >/tmp/bazel-deps-checker-smoke.log 2>&1 &
SERVER_PID=$!
sleep 1

echo "[3/4] Checking health endpoint..."
curl -fsS "http://127.0.0.1:${PORT}/api/health" >/tmp/bazel-deps-checker-health.json
cat /tmp/bazel-deps-checker-health.json

echo "[4/4] Creating and polling one analysis task..."
TASK_ID="$(
  curl -fsS -X POST "http://127.0.0.1:${PORT}/api/analyze" \
    -H 'Content-Type: application/json' \
    -d "{\"mode\":\"cycle\",\"workspace_path\":\"${WORKSPACE_PATH}\",\"bazel_binary\":\"bazel\",\"include_tests\":false}" |
  sed -n 's/.*"task_id": "\(task-[^"]*\)".*/\1/p'
)"

if [[ -z "$TASK_ID" ]]; then
  echo "Failed to capture task_id" >&2
  exit 1
fi

echo "Task: $TASK_ID"

for _ in $(seq 1 20); do
  RESPONSE="$(curl -fsS "http://127.0.0.1:${PORT}/api/tasks/${TASK_ID}")"
  STATUS="$(printf '%s' "$RESPONSE" | sed -n 's/.*"status": "\([^"]*\)".*/\1/p')"
  echo "Current status: ${STATUS}"
  if [[ "$STATUS" == "completed" ]]; then
    printf '%s\n' "$RESPONSE"
    exit 0
  fi
  if [[ "$STATUS" == "failed" ]]; then
    printf '%s\n' "$RESPONSE" >&2
    exit 1
  fi
  sleep 1
done

echo "Task did not complete in time" >&2
exit 1
