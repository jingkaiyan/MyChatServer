#!/usr/bin/env bash
set -euo pipefail

HOST="127.0.0.1"
PORT="3306"
DB_USER="chat"
DB_PASS="chat123"
DB_NAME="chat"

BASELINE_TAG=""
TARGET_TAG=""
BASELINE_SECONDS=""
TARGET_SECONDS=""
EXPORT_MD=""
EXPORT_CSV=""

usage() {
  cat <<'EOF'
用法:
  ./scripts/compare_benchmark_runs.sh [options]

必选参数:
  --baseline-tag <tag>         基线批次标识（例如 bench_20260304_150317_21492）
  --target-tag <tag>           优化后批次标识

可选参数:
  --host <host>                MySQL 主机 (默认: 127.0.0.1)
  --port <port>                MySQL 端口 (默认: 3306)
  --db-user <user>             MySQL 用户 (默认: chat)
  --db-pass <password>         MySQL 密码 (默认: chat123)
  --db-name <name>             数据库名 (默认: chat)
  --baseline-seconds <sec>     基线压测总时长（秒）
  --target-seconds <sec>       优化后压测总时长（秒）
  --export-md <file>           导出 Markdown 对比报告
  --export-csv <file>          导出 CSV 对比报告（metric,baseline,target）
  --help                       查看帮助

示例:
  ./scripts/compare_benchmark_runs.sh \
    --baseline-tag bench_20260304_150317_21492 \
    --target-tag bench_20260305_103211_18201

  ./scripts/compare_benchmark_runs.sh \
    --baseline-tag bench_20260304_150317_21492 \
    --target-tag bench_20260305_103211_18201 \
    --baseline-seconds 300 --target-seconds 300 \
    --export-md ./exports/benchmark_compare.md \
    --export-csv ./exports/benchmark_compare.csv
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --host)
      HOST="$2"; shift 2 ;;
    --port)
      PORT="$2"; shift 2 ;;
    --db-user)
      DB_USER="$2"; shift 2 ;;
    --db-pass)
      DB_PASS="$2"; shift 2 ;;
    --db-name)
      DB_NAME="$2"; shift 2 ;;
    --baseline-tag)
      BASELINE_TAG="$2"; shift 2 ;;
    --target-tag)
      TARGET_TAG="$2"; shift 2 ;;
    --baseline-seconds)
      BASELINE_SECONDS="$2"; shift 2 ;;
    --target-seconds)
      TARGET_SECONDS="$2"; shift 2 ;;
    --export-md)
      EXPORT_MD="$2"; shift 2 ;;
    --export-csv)
      EXPORT_CSV="$2"; shift 2 ;;
    -h|--help)
      usage; exit 0 ;;
    *)
      echo "未知参数: $1"
      usage
      exit 1 ;;
  esac
done

if [[ -z "$BASELINE_TAG" || -z "$TARGET_TAG" ]]; then
  echo "--baseline-tag 和 --target-tag 必须提供"
  usage
  exit 1
fi

for t in "$BASELINE_TAG" "$TARGET_TAG"; do
  if [[ ! "$t" =~ ^bench_[0-9]{8}_[0-9]{6}_[0-9]+$ ]]; then
    echo "tag 格式非法: $t"
    exit 1
  fi
done

for sec in "$BASELINE_SECONDS" "$TARGET_SECONDS"; do
  if [[ -n "$sec" && ! "$sec" =~ ^[0-9]+([.][0-9]+)?$ ]]; then
    echo "时长参数必须为数字，当前值: $sec"
    exit 1
  fi
done

mysql_cmd=(mysql -h "$HOST" -P "$PORT" -u "$DB_USER" --default-character-set=utf8mb4 -N -s "$DB_NAME")

scalar_sql() {
  MYSQL_PWD="$DB_PASS" "${mysql_cmd[@]}" -e "$1"
}

count_users() {
  local tag="$1"
  scalar_sql "SELECT COUNT(*) FROM user WHERE name LIKE '${tag}_u_%';"
}

count_groups() {
  local tag="$1"
  scalar_sql "SELECT COUNT(*) FROM allgroup WHERE groupname LIKE '${tag}_g_%';"
}

count_friends() {
  local tag="$1"
  scalar_sql "SELECT COUNT(*) FROM friend f JOIN user u ON f.userid=u.id WHERE u.name LIKE '${tag}_u_%';"
}

count_groupusers() {
  local tag="$1"
  scalar_sql "SELECT COUNT(*) FROM groupuser gu JOIN allgroup g ON gu.groupid=g.id WHERE g.groupname LIKE '${tag}_g_%';"
}

fmt_delta() {
  local base="$1"
  local target="$2"
  awk -v b="$base" -v t="$target" 'BEGIN {
    d = t - b;
    if (b == 0) {
      if (t == 0) { printf("0 (0.00%%)"); }
      else { printf("%d (N/A)", d); }
    } else {
      p = d * 100.0 / b;
      printf("%d (%.2f%%)", d, p);
    }
  }'
}

fmt_rate() {
  local value="$1"
  local sec="$2"
  awk -v v="$value" -v s="$sec" 'BEGIN {
    if (s == 0) { printf("N/A"); }
    else { printf("%.2f", v / s); }
  }'
}

echo "[1/3] 检查数据库连接..."
scalar_sql "SELECT 1;" >/dev/null

echo "[2/3] 统计批次数据..."
B_USERS="$(count_users "$BASELINE_TAG")"
B_GROUPS="$(count_groups "$BASELINE_TAG")"
B_FRIENDS="$(count_friends "$BASELINE_TAG")"
B_GROUPUSERS="$(count_groupusers "$BASELINE_TAG")"

T_USERS="$(count_users "$TARGET_TAG")"
T_GROUPS="$(count_groups "$TARGET_TAG")"
T_FRIENDS="$(count_friends "$TARGET_TAG")"
T_GROUPUSERS="$(count_groupusers "$TARGET_TAG")"

cat <<EOF

================ 压测批次对比报告 ================
Baseline Tag : $BASELINE_TAG
Target Tag   : $TARGET_TAG

| 指标 | Baseline | Target | Δ (Target-Baseline) |
|---|---:|---:|---:|
| users | $B_USERS | $T_USERS | $(fmt_delta "$B_USERS" "$T_USERS") |
| groups | $B_GROUPS | $T_GROUPS | $(fmt_delta "$B_GROUPS" "$T_GROUPS") |
| friend_rows | $B_FRIENDS | $T_FRIENDS | $(fmt_delta "$B_FRIENDS" "$T_FRIENDS") |
| groupuser_rows | $B_GROUPUSERS | $T_GROUPUSERS | $(fmt_delta "$B_GROUPUSERS" "$T_GROUPUSERS") |
EOF

if [[ -n "$BASELINE_SECONDS" && -n "$TARGET_SECONDS" ]]; then
  cat <<EOF

| 吞吐指标 (rows/s) | Baseline | Target | Δ |
|---|---:|---:|---:|
| users/s | $(fmt_rate "$B_USERS" "$BASELINE_SECONDS") | $(fmt_rate "$T_USERS" "$TARGET_SECONDS") | $(awk -v b="$(fmt_rate "$B_USERS" "$BASELINE_SECONDS")" -v t="$(fmt_rate "$T_USERS" "$TARGET_SECONDS")" 'BEGIN{printf("%.2f", t-b)}') |
| groups/s | $(fmt_rate "$B_GROUPS" "$BASELINE_SECONDS") | $(fmt_rate "$T_GROUPS" "$TARGET_SECONDS") | $(awk -v b="$(fmt_rate "$B_GROUPS" "$BASELINE_SECONDS")" -v t="$(fmt_rate "$T_GROUPS" "$TARGET_SECONDS")" 'BEGIN{printf("%.2f", t-b)}') |
| friend_rows/s | $(fmt_rate "$B_FRIENDS" "$BASELINE_SECONDS") | $(fmt_rate "$T_FRIENDS" "$TARGET_SECONDS") | $(awk -v b="$(fmt_rate "$B_FRIENDS" "$BASELINE_SECONDS")" -v t="$(fmt_rate "$T_FRIENDS" "$TARGET_SECONDS")" 'BEGIN{printf("%.2f", t-b)}') |
| groupuser_rows/s | $(fmt_rate "$B_GROUPUSERS" "$BASELINE_SECONDS") | $(fmt_rate "$T_GROUPUSERS" "$TARGET_SECONDS") | $(awk -v b="$(fmt_rate "$B_GROUPUSERS" "$BASELINE_SECONDS")" -v t="$(fmt_rate "$T_GROUPUSERS" "$TARGET_SECONDS")" 'BEGIN{printf("%.2f", t-b)}') |
EOF
fi

echo "================================================"

if [[ -n "$EXPORT_MD" ]]; then
  mkdir -p "$(dirname "$EXPORT_MD")"
  {
    echo "# Benchmark Compare Report"
    echo
    echo "- baseline: $BASELINE_TAG"
    echo "- target: $TARGET_TAG"
    if [[ -n "$BASELINE_SECONDS" && -n "$TARGET_SECONDS" ]]; then
      echo "- baseline_seconds: $BASELINE_SECONDS"
      echo "- target_seconds: $TARGET_SECONDS"
    fi
    echo
    echo "| metric | baseline | target | delta |"
    echo "|---|---:|---:|---:|"
    echo "| users | $B_USERS | $T_USERS | $(fmt_delta "$B_USERS" "$T_USERS") |"
    echo "| groups | $B_GROUPS | $T_GROUPS | $(fmt_delta "$B_GROUPS" "$T_GROUPS") |"
    echo "| friend_rows | $B_FRIENDS | $T_FRIENDS | $(fmt_delta "$B_FRIENDS" "$T_FRIENDS") |"
    echo "| groupuser_rows | $B_GROUPUSERS | $T_GROUPUSERS | $(fmt_delta "$B_GROUPUSERS" "$T_GROUPUSERS") |"
  } > "$EXPORT_MD"
  echo "[3/3] Markdown 报告已导出: $EXPORT_MD"
else
  echo "[3/3] 对比完成。"
fi

if [[ -n "$EXPORT_CSV" ]]; then
  mkdir -p "$(dirname "$EXPORT_CSV")"
  {
    echo "metric,baseline,target"
    echo "users,$B_USERS,$T_USERS"
    echo "groups,$B_GROUPS,$T_GROUPS"
    echo "friend_rows,$B_FRIENDS,$T_FRIENDS"
    echo "groupuser_rows,$B_GROUPUSERS,$T_GROUPUSERS"
    if [[ -n "$BASELINE_SECONDS" && -n "$TARGET_SECONDS" ]]; then
      echo "users_per_sec,$(fmt_rate "$B_USERS" "$BASELINE_SECONDS"),$(fmt_rate "$T_USERS" "$TARGET_SECONDS")"
      echo "groups_per_sec,$(fmt_rate "$B_GROUPS" "$BASELINE_SECONDS"),$(fmt_rate "$T_GROUPS" "$TARGET_SECONDS")"
      echo "friend_rows_per_sec,$(fmt_rate "$B_FRIENDS" "$BASELINE_SECONDS"),$(fmt_rate "$T_FRIENDS" "$TARGET_SECONDS")"
      echo "groupuser_rows_per_sec,$(fmt_rate "$B_GROUPUSERS" "$BASELINE_SECONDS"),$(fmt_rate "$T_GROUPUSERS" "$TARGET_SECONDS")"
    fi
  } > "$EXPORT_CSV"
  echo "CSV 报告已导出: $EXPORT_CSV"
fi
