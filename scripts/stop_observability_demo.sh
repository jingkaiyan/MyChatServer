#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXPORTER_PID_FILE="${ROOT_DIR}/exports/metrics_exporter.pid"

(
  cd "${ROOT_DIR}/monitoring"
  docker compose down
)

if [[ -f "${EXPORTER_PID_FILE}" ]]; then
  pid="$(cat "${EXPORTER_PID_FILE}")"
  if kill -0 "${pid}" >/dev/null 2>&1; then
    kill "${pid}" || true
    echo "stopped metrics exporter (pid=${pid})"
  fi
  rm -f "${EXPORTER_PID_FILE}"
fi

echo "observability demo stopped"
