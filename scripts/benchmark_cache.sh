#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_OUTPUT="${BUILD_OUTPUT:-/tmp/bazel-deps-analyzer}"
PORT="${PORT:-18093}"
WORKSPACE_PATH="${WORKSPACE_PATH:-$ROOT_DIR}"
SAMPLES="${SAMPLES:-3}"

cleanup() {
  if [[ -n "${SERVER_PID:-}" ]]; then
    kill "${SERVER_PID}" >/dev/null 2>&1 || true
    wait "${SERVER_PID}" >/dev/null 2>&1 || true
  fi
}

trap cleanup EXIT

echo "[1/5] Compiling binary..."
clang++ -std=c++17 -Wall -Wextra -Wpedantic \
  -Isrc -Isrc/common -Isrc/core -Isrc/runtime -Ithirds \
  $(find "$ROOT_DIR/src" -name '*.cpp' | tr '\n' ' ') \
  -o "$BUILD_OUTPUT"

echo "[2/5] Starting Web UI on port ${PORT}..."
"$BUILD_OUTPUT" --ui --port "$PORT" >/tmp/bazel-deps-checker-benchmark.log 2>&1 &
SERVER_PID=$!
sleep 1

echo "[3/5] Clearing caches before measurement..."
curl -fsS -X POST "http://127.0.0.1:${PORT}/api/cache/clear" >/dev/null

clear_caches() {
  curl -fsS -X POST "http://127.0.0.1:${PORT}/api/cache/clear" >/dev/null
}

measure_task() {
  local payload="$1"
  local start_ns end_ns task_id response status
  start_ns="$(date +%s%N)"
  response="$(
    curl -fsS -X POST "http://127.0.0.1:${PORT}/api/analyze" \
      -H 'Content-Type: application/json' \
      -d "$payload"
  )"

  status="$(printf '%s' "$response" | sed -n 's/.*"status": "\([^"]*\)".*/\1/p')"
  if printf '%s' "$response" | grep -q '"ok": true' && [[ "$status" != "queued" ]]; then
    end_ns="$(date +%s%N)"
    python3 - <<PY
start_ns = int("${start_ns}")
end_ns = int("${end_ns}")
print(f"{(end_ns - start_ns)/1_000_000:.2f}")
PY
    return 0
  fi

  task_id="$(printf '%s' "$response" | sed -n 's/.*"task_id": "\(task-[^"]*\)".*/\1/p')"

  if [[ -z "$task_id" ]]; then
    echo "missing task id" >&2
    printf '%s\n' "$response" >&2
    return 1
  fi

  for _ in $(seq 1 30); do
    response="$(curl -fsS "http://127.0.0.1:${PORT}/api/tasks/${task_id}")"
    status="$(printf '%s' "$response" | sed -n 's/.*"status": "\([^"]*\)".*/\1/p')"
    if [[ "$status" == "completed" ]]; then
      end_ns="$(date +%s%N)"
      python3 - <<PY
start_ns = int("${start_ns}")
end_ns = int("${end_ns}")
print(f"{(end_ns - start_ns)/1_000_000:.2f}")
PY
      return 0
    fi
    if [[ "$status" == "failed" ]]; then
      printf '%s\n' "$response" >&2
      return 1
    fi
    sleep 1
  done

  echo "timeout" >&2
  return 1
}

measure_cold_cycle() {
  clear_caches
  measure_task "$PAYLOAD_CYCLE"
}

measure_warm_unused() {
  clear_caches
  measure_task "$PAYLOAD_CYCLE" >/dev/null
  measure_task "$PAYLOAD_UNUSED"
}

collect_samples() {
  local label="$1"
  local func_name="$2"
  local -a values=()
  local value

  echo "${label} (${SAMPLES} samples):"
  for index in $(seq 1 "$SAMPLES"); do
    value="$($func_name)"
    values+=("$value")
    echo "  - sample ${index}: ${value} ms"
  done

  python3 - "$label" "${values[@]}" <<'PY'
import statistics
import sys

label = sys.argv[1]
values = [float(v) for v in sys.argv[2:]]
print(f"{label} median: {statistics.median(values):.2f} ms")
print(f"{label} mean:   {statistics.fmean(values):.2f} ms")
print(f"{statistics.median(values):.2f}")
PY
}

collect_samples_to_file() {
  local label="$1"
  local func_name="$2"
  local output_file="$3"
  local median

  median="$(collect_samples "$label" "$func_name" | tee "$output_file" | tail -n 1)"
  sed '$d' "$output_file" >&2
  printf '%s' "$median"
}

PAYLOAD_CYCLE="{\"mode\":\"cycle\",\"workspace_path\":\"${WORKSPACE_PATH}\",\"bazel_binary\":\"bazel\",\"include_tests\":false,\"force_refresh\":true}"
PAYLOAD_UNUSED="{\"mode\":\"unused\",\"workspace_path\":\"${WORKSPACE_PATH}\",\"bazel_binary\":\"bazel\",\"include_tests\":false}"

echo "[4/5] Measuring cold cycle request..."
COLD_MS="$(collect_samples_to_file "Cold cycle" measure_cold_cycle /tmp/bazel-deps-checker-cold-benchmark.log)"

echo "[5/5] Measuring warm unused request after cycle warm-up..."
WARM_MS="$(collect_samples_to_file "Warm unused" measure_warm_unused /tmp/bazel-deps-checker-warm-benchmark.log)"

python3 - <<PY
cold_ms = float("${COLD_MS}")
warm_ms = float("${WARM_MS}")
ratio = warm_ms / cold_ms if cold_ms else 0.0
print(f"Warm/Cold ratio: {ratio:.3f}")
PY
