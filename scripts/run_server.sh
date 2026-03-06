#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SERVER_BIN="${PROJECT_ROOT}/bin/ChatServer"

ENV_FILE="${PROJECT_ROOT}/scripts/server.env"
DAEMON=false
START_DEPS=true
LOG_FILE="${PROJECT_ROOT}/logs/chatserver.log"
PID_FILE="${PROJECT_ROOT}/run/chatserver.pid"

usage() {
  cat <<'EOF'
Usage:
  ./scripts/run_server.sh [options]

Options:
  --env-file <file>      env 配置文件 (默认: ./scripts/server.env)
  --daemon               后台启动 (日志写入 --log-file)
  --log-file <file>      后台日志路径 (默认: ./logs/chatserver.log)
  --pid-file <file>      PID 文件路径 (默认: ./run/chatserver.pid)
  --no-deps              不自动拉起 mysql/redis-server
  -h, --help             显示帮助

Environment (可由 env-file 覆盖):
  MYCHAT_SERVER_HOST     默认 127.0.0.1
  MYCHAT_SERVER_PORT     默认 8000
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --env-file)
      ENV_FILE="$2"
      shift 2
      ;;
    --daemon)
      DAEMON=true
      shift
      ;;
    --log-file)
      LOG_FILE="$2"
      shift 2
      ;;
    --pid-file)
      PID_FILE="$2"
      shift 2
      ;;
    --no-deps)
      START_DEPS=false
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1"
      usage
      exit 1
      ;;
  esac
done

if [[ -f "${ENV_FILE}" ]]; then
  set -a
  source "${ENV_FILE}"
  set +a
fi

HOST="${MYCHAT_SERVER_HOST:-127.0.0.1}"
PORT="${MYCHAT_SERVER_PORT:-8000}"

if [[ ! -x "${SERVER_BIN}" ]]; then
  echo "ChatServer not found. Build first:"
  echo "  cd ${PROJECT_ROOT} && BUILD_SERVER=ON BUILD_CLIENT=OFF ./autobuild.sh"
  exit 1
fi

mkdir -p "$(dirname "${LOG_FILE}")" "$(dirname "${PID_FILE}")"

if [[ -f "${PID_FILE}" ]]; then
  existing_pid="$(cat "${PID_FILE}" 2>/dev/null || true)"
  if [[ -n "${existing_pid}" ]] && kill -0 "${existing_pid}" 2>/dev/null; then
    echo "ChatServer already running, pid=${existing_pid}"
    exit 0
  fi
  rm -f "${PID_FILE}"
fi

occupied_pid="$(ss -ltnp 2>/dev/null | awk -v hp="${HOST}:${PORT}" '$4 ~ hp {print $NF}' | sed -n 's/.*pid=\([0-9]\+\).*/\1/p' | head -n1)"
if [[ -n "${occupied_pid}" ]]; then
  echo "Port ${HOST}:${PORT} already in use by pid=${occupied_pid}"
  exit 1
fi

if [[ "${START_DEPS}" == true ]]; then
  sudo systemctl start mysql >/dev/null 2>&1 || true
  sudo systemctl start redis-server >/dev/null 2>&1 || true
fi

echo "Starting ChatServer at ${HOST}:${PORT}"
if [[ "${DAEMON}" == true ]]; then
  nohup "${SERVER_BIN}" "${HOST}" "${PORT}" >>"${LOG_FILE}" 2>&1 &
  echo $! > "${PID_FILE}"
  echo "Started in daemon mode, pid=$(cat "${PID_FILE}")"
  echo "Log: ${LOG_FILE}"
else
  exec "${SERVER_BIN}" "${HOST}" "${PORT}"
fi
