#!/usr/bin/env bash
set -euo pipefail

HOST="${MYCHAT_SERVER_HOST:-127.0.0.1}"
PORT="${MYCHAT_SERVER_PORT:-8000}"
DB_HOST="${MYCHAT_DB_HOST:-127.0.0.1}"
DB_PORT="${MYCHAT_DB_PORT:-3306}"
DB_USER="${MYCHAT_DB_USER:-chat}"
DB_PASS="${MYCHAT_DB_PASS:-chat123}"
DB_NAME="${MYCHAT_DB_NAME:-chat}"

ok() { echo "[OK] $*"; }
fail() { echo "[FAIL] $*"; exit 1; }

if systemctl is-active --quiet mysql; then
  ok "mysql active"
else
  fail "mysql not active"
fi

if systemctl is-active --quiet redis-server; then
  ok "redis-server active"
else
  fail "redis-server not active"
fi

if ss -ltnp | grep -q "${HOST}:${PORT}"; then
  ok "ChatServer listening on ${HOST}:${PORT}"
else
  fail "ChatServer not listening on ${HOST}:${PORT}"
fi

if MYSQL_PWD="${DB_PASS}" mysql -h "${DB_HOST}" -P "${DB_PORT}" -u "${DB_USER}" -D "${DB_NAME}" -e "SELECT 1" >/dev/null 2>&1; then
  ok "MySQL connectivity (${DB_USER}@${DB_HOST}:${DB_PORT}/${DB_NAME})"
else
  fail "MySQL connectivity failed"
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
METRICS_OUT="${MYCHAT_METRICS_OUT:-./exports/runtime_metrics.prom}"
if "${SCRIPT_DIR}/metrics_snapshot.sh" "${METRICS_OUT}" >/dev/null 2>&1; then
  ok "metrics snapshot generated (${METRICS_OUT})"
else
  fail "metrics snapshot generation failed"
fi

echo "All checks passed."
