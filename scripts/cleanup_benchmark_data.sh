#!/usr/bin/env bash
set -euo pipefail

HOST="127.0.0.1"
PORT="3306"
DB_USER="chat"
DB_PASS="chat123"
DB_NAME="chat"

RUN_TAG=""
DELETE_ALL_BENCH=false
DRY_RUN=false

usage() {
  cat <<'EOF'
用法:
  ./scripts/cleanup_benchmark_data.sh [options]

可选参数:
  --host <host>             MySQL 主机 (默认: 127.0.0.1)
  --port <port>             MySQL 端口 (默认: 3306)
  --db-user <user>          MySQL 用户 (默认: chat)
  --db-pass <password>      MySQL 密码 (默认: chat123)
  --db-name <name>          数据库名 (默认: chat)
  --run-tag <tag>           删除指定批次(例如: bench_20260304_150317_21492)
  --all-bench               删除所有 bench_ 前缀压测数据
  --dry-run                 只统计不删除
  --list-tags               列出可清理批次及数量概览
  -h, --help                查看帮助

示例:
  ./scripts/cleanup_benchmark_data.sh --run-tag bench_20260304_150317_21492
  ./scripts/cleanup_benchmark_data.sh --all-bench
  ./scripts/cleanup_benchmark_data.sh --all-bench --dry-run
EOF
}

mysql_cmd=(mysql -h "$HOST" -P "$PORT" -u "$DB_USER" --default-character-set=utf8mb4 -N -s "$DB_NAME")

run_scalar_sql() {
  MYSQL_PWD="$DB_PASS" "${mysql_cmd[@]}" -e "$1"
}

list_tags() {
  echo "批次概览（tag users groups）:"
  MYSQL_PWD="$DB_PASS" "${mysql_cmd[@]}" -e "
SELECT tag,
       SUM(user_cnt) AS users,
       SUM(group_cnt) AS groups
FROM (
  SELECT SUBSTRING_INDEX(name, '_u_', 1) AS tag, COUNT(*) AS user_cnt, 0 AS group_cnt
  FROM user
  WHERE name LIKE 'bench\_%\_u\_%' ESCAPE '\\'
  GROUP BY SUBSTRING_INDEX(name, '_u_', 1)

  UNION ALL

  SELECT SUBSTRING_INDEX(groupname, '_g_', 1) AS tag, 0 AS user_cnt, COUNT(*) AS group_cnt
  FROM allgroup
  WHERE groupname LIKE 'bench\_%\_g\_%' ESCAPE '\\'
  GROUP BY SUBSTRING_INDEX(groupname, '_g_', 1)
) t
GROUP BY tag
ORDER BY tag;
"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --host)
      HOST="$2"
      shift 2
      ;;
    --port)
      PORT="$2"
      shift 2
      ;;
    --db-user)
      DB_USER="$2"
      shift 2
      ;;
    --db-pass)
      DB_PASS="$2"
      shift 2
      ;;
    --db-name)
      DB_NAME="$2"
      shift 2
      ;;
    --run-tag)
      RUN_TAG="$2"
      shift 2
      ;;
    --all-bench)
      DELETE_ALL_BENCH=true
      shift
      ;;
    --dry-run)
      DRY_RUN=true
      shift
      ;;
    --list-tags)
      list_tags
      exit 0
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "未知参数: $1"
      usage
      exit 1
      ;;
  esac
done

# 参数解析后再重建 mysql_cmd（确保 host/port/user/db 生效）
mysql_cmd=(mysql -h "$HOST" -P "$PORT" -u "$DB_USER" --default-character-set=utf8mb4 -N -s "$DB_NAME")

if [[ -n "$RUN_TAG" && "$DELETE_ALL_BENCH" == true ]]; then
  echo "--run-tag 和 --all-bench 不能同时使用"
  exit 1
fi

if [[ -z "$RUN_TAG" && "$DELETE_ALL_BENCH" == false ]]; then
  echo "请指定 --run-tag <tag> 或 --all-bench"
  usage
  exit 1
fi

if [[ -n "$RUN_TAG" && ! "$RUN_TAG" =~ ^bench_[0-9]{8}_[0-9]{6}_[0-9]+$ ]]; then
  echo "--run-tag 格式非法: $RUN_TAG"
  echo "示例: bench_20260304_150317_21492"
  exit 1
fi

echo "[1/4] 检查数据库连接..."
run_scalar_sql "SELECT 1;" >/dev/null

if [[ -n "$RUN_TAG" ]]; then
  user_like="${RUN_TAG}_u_%"
  group_like="${RUN_TAG}_g_%"
else
  user_like="bench_%_u_%"
  group_like="bench_%_g_%"
fi

echo "[2/4] 统计待清理数据..."
target_users="$(run_scalar_sql "SELECT COUNT(*) FROM user WHERE name LIKE '${user_like}';")"
target_groups="$(run_scalar_sql "SELECT COUNT(*) FROM allgroup WHERE groupname LIKE '${group_like}';")"

target_friend_rows="$(run_scalar_sql "
SELECT COUNT(*)
FROM friend f
JOIN user u ON f.userid = u.id
WHERE u.name LIKE '${user_like}';
")"

target_groupuser_rows="$(run_scalar_sql "
SELECT COUNT(*)
FROM groupuser gu
JOIN allgroup g ON gu.groupid = g.id
WHERE g.groupname LIKE '${group_like}';
")"

echo "待清理统计:"
echo "  用户: ${target_users}"
echo "  好友关系(按 userid 落在目标用户): ${target_friend_rows}"
echo "  群组: ${target_groups}"
echo "  群成员关系(按 groupid 落在目标群组): ${target_groupuser_rows}"

if [[ "$DRY_RUN" == true ]]; then
  echo "[3/4] dry-run 模式，不执行删除。"
  exit 0
fi

echo "[3/4] 执行删除..."
tmp_sql="$(mktemp /tmp/mychat_cleanup_benchmark_XXXXXX.sql)"
trap 'rm -f "$tmp_sql"' EXIT

cat >"$tmp_sql" <<SQL
SET NAMES utf8mb4;
START TRANSACTION;

DELETE gu
FROM groupuser gu
JOIN allgroup g ON gu.groupid = g.id
WHERE g.groupname LIKE '${group_like}';

DELETE f
FROM friend f
JOIN user u ON f.userid = u.id
WHERE u.name LIKE '${user_like}';

DELETE f
FROM friend f
JOIN user u ON f.friendid = u.id
WHERE u.name LIKE '${user_like}';

DELETE FROM allgroup
WHERE groupname LIKE '${group_like}';

DELETE FROM user
WHERE name LIKE '${user_like}';

COMMIT;
SQL

MYSQL_PWD="$DB_PASS" "${mysql_cmd[@]}" <"$tmp_sql"

echo "[4/4] 清理后复核..."
left_users="$(run_scalar_sql "SELECT COUNT(*) FROM user WHERE name LIKE '${user_like}';")"
left_groups="$(run_scalar_sql "SELECT COUNT(*) FROM allgroup WHERE groupname LIKE '${group_like}';")"

echo "完成!"
if [[ -n "$RUN_TAG" ]]; then
  echo "  清理批次: ${RUN_TAG}"
else
  echo "  清理范围: 所有 bench 批次"
fi
echo "  剩余目标用户: ${left_users}"
echo "  剩余目标群组: ${left_groups}"
