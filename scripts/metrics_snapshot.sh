#!/usr/bin/env bash
set -euo pipefail

HOST="${MYCHAT_SERVER_HOST:-127.0.0.1}"
PORT="${MYCHAT_SERVER_PORT:-8000}"
DB_HOST="${MYCHAT_DB_HOST:-127.0.0.1}"
DB_PORT="${MYCHAT_DB_PORT:-3306}"
DB_USER="${MYCHAT_DB_USER:-chat}"
DB_PASS="${MYCHAT_DB_PASS:-chat123}"
DB_NAME="${MYCHAT_DB_NAME:-chat}"
OUT_FILE="${1:-./exports/runtime_metrics.prom}"

mkdir -p "$(dirname "$OUT_FILE")"

mysql_scalar() {
  MYSQL_PWD="${DB_PASS}" mysql -N -s -h "${DB_HOST}" -P "${DB_PORT}" -u "${DB_USER}" -D "${DB_NAME}" -e "$1"
}

mysql_up=1
if ! mysql_scalar "SELECT 1" >/dev/null 2>&1; then
  mysql_up=0
fi

redis_up=0
if systemctl is-active --quiet redis-server; then
  redis_up=1
fi

chatserver_listen=0
chatserver_pid=""
if ss -ltnp 2>/dev/null | grep -q "${HOST}:${PORT}"; then
  chatserver_listen=1
  chatserver_pid="$(ss -ltnp 2>/dev/null | awk -v hp="${HOST}:${PORT}" '$4 ~ hp {print $NF}' | sed -n 's/.*pid=\([0-9]\+\).*/\1/p' | head -n1)"
fi

tcp_established=0
if [[ "$chatserver_listen" -eq 1 ]]; then
  tcp_established="$(ss -tnp 2>/dev/null | awk -v hp="${HOST}:${PORT}" '$4 ~ hp && $1=="ESTAB" {c++} END{print c+0}')"
fi

user_total=0
friend_total=0
group_total=0
groupuser_total=0
offline_total=0

if [[ "$mysql_up" -eq 1 ]]; then
  user_total="$(mysql_scalar "SELECT COUNT(*) FROM user;")"
  friend_total="$(mysql_scalar "SELECT COUNT(*) FROM friend;")"
  group_total="$(mysql_scalar "SELECT COUNT(*) FROM allgroup;")"
  groupuser_total="$(mysql_scalar "SELECT COUNT(*) FROM groupuser;")"
  offline_total="$(mysql_scalar "SELECT COUNT(*) FROM offlinemessage;")"
fi

cat > "$OUT_FILE" <<EOF
# HELP mychat_mysql_up Whether MySQL connectivity is healthy (1/0)
# TYPE mychat_mysql_up gauge
mychat_mysql_up $mysql_up

# HELP mychat_redis_up Whether redis-server service is active (1/0)
# TYPE mychat_redis_up gauge
mychat_redis_up $redis_up

# HELP mychat_chatserver_listen Whether ChatServer is listening on configured host/port (1/0)
# TYPE mychat_chatserver_listen gauge
mychat_chatserver_listen $chatserver_listen

# HELP mychat_chatserver_tcp_established Active established TCP sessions on ChatServer listener
# TYPE mychat_chatserver_tcp_established gauge
mychat_chatserver_tcp_established $tcp_established

# HELP mychat_db_user_total Total rows in user table
# TYPE mychat_db_user_total gauge
mychat_db_user_total $user_total

# HELP mychat_db_friend_total Total rows in friend table
# TYPE mychat_db_friend_total gauge
mychat_db_friend_total $friend_total

# HELP mychat_db_group_total Total rows in allgroup table
# TYPE mychat_db_group_total gauge
mychat_db_group_total $group_total

# HELP mychat_db_groupuser_total Total rows in groupuser table
# TYPE mychat_db_groupuser_total gauge
mychat_db_groupuser_total $groupuser_total

# HELP mychat_db_offline_total Total rows in offlinemessage table
# TYPE mychat_db_offline_total gauge
mychat_db_offline_total $offline_total
EOF

echo "Metrics snapshot written: $OUT_FILE"
if [[ -n "$chatserver_pid" ]]; then
  echo "ChatServer pid: $chatserver_pid"
fi
