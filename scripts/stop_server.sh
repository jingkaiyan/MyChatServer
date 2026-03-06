#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PID_FILE="${PROJECT_ROOT}/run/chatserver.pid"

if [[ -f "${PID_FILE}" ]]; then
  pid="$(cat "${PID_FILE}" 2>/dev/null || true)"
  if [[ -n "${pid}" ]] && kill -0 "${pid}" 2>/dev/null; then
    kill "${pid}"
    sleep 1
    if kill -0 "${pid}" 2>/dev/null; then
      kill -9 "${pid}" || true
    fi
    echo "Stopped ChatServer pid=${pid}"
  else
    echo "PID file exists but process not running"
  fi
  rm -f "${PID_FILE}"
  exit 0
fi

pid_from_port="$(ss -ltnp 2>/dev/null | awk '/127.0.0.1:8000/ {print $NF}' | sed -n 's/.*pid=\([0-9]\+\).*/\1/p' | head -n1)"
if [[ -n "${pid_from_port}" ]]; then
  kill "${pid_from_port}" || true
  echo "Stopped ChatServer by port, pid=${pid_from_port}"
else
  echo "ChatServer is not running"
fi
