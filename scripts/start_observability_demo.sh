#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXPORTER_PID_FILE="${ROOT_DIR}/exports/metrics_exporter.pid"
EXPORTER_LOG_FILE="${ROOT_DIR}/exports/metrics_exporter.log"

mkdir -p "${ROOT_DIR}/exports"

if [[ -f "${EXPORTER_PID_FILE}" ]]; then
  old_pid="$(cat "${EXPORTER_PID_FILE}")"
  if kill -0 "${old_pid}" >/dev/null 2>&1; then
    echo "metrics exporter already running (pid=${old_pid})"
  else
    rm -f "${EXPORTER_PID_FILE}"
  fi
fi

if [[ ! -f "${EXPORTER_PID_FILE}" ]]; then
  (
    cd "${ROOT_DIR}"
    nohup python3 ./scripts/metrics_exporter.py > "${EXPORTER_LOG_FILE}" 2>&1 &
    echo $! > "${EXPORTER_PID_FILE}"
  )
  echo "started metrics exporter (pid=$(cat "${EXPORTER_PID_FILE}"))"
fi

(
  cd "${ROOT_DIR}/monitoring"
  docker compose up -d
)

echo "Prometheus: http://127.0.0.1:9090"
echo "Grafana:    http://127.0.0.1:3000  (admin/admin123)"
