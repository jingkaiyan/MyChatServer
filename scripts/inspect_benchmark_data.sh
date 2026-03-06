#!/usr/bin/env bash
set -euo pipefail

HOST="127.0.0.1"
PORT="3306"
DB_USER="chat"
DB_PASS="chat123"
DB_NAME="chat"

RUN_TAG=""
LIMIT=10
ONLY_TOTAL=false
EXCLUDE_BENCH=false
EXPORT_DIR=""
EXPORT_XLSX=""
TEMP_EXPORT_DIR=""

usage() {
  cat <<'EOF'
用法:
  ./scripts/inspect_benchmark_data.sh [options]

可选参数:
  --host <host>             MySQL 主机 (默认: 127.0.0.1)
  --port <port>             MySQL 端口 (默认: 3306)
  --db-user <user>          MySQL 用户 (默认: chat)
  --db-pass <password>      MySQL 密码 (默认: chat123)
  --db-name <name>          数据库名 (默认: chat)
  --run-tag <tag>           查看指定批次(例如: bench_20260304_150317_21492)
  --limit <n>               样例行数 (默认: 10)
  --only-total              仅显示总量统计
  --exclude-bench           仅查看非 bench_ 前缀的业务数据
  --export <dir>            导出 CSV 到指定目录
  --export-xlsx <file>      导出 XLSX 文件（会包含多个 sheet）
  --help                    查看帮助

示例:
  ./scripts/inspect_benchmark_data.sh
  ./scripts/inspect_benchmark_data.sh --run-tag bench_20260304_150317_21492 --limit 20
  ./scripts/inspect_benchmark_data.sh --only-total
  ./scripts/inspect_benchmark_data.sh --exclude-bench --limit 30
  ./scripts/inspect_benchmark_data.sh --run-tag bench_20260304_150317_21492 --export ./exports/bench_check
  ./scripts/inspect_benchmark_data.sh --run-tag bench_20260304_150317_21492 --export-xlsx ./exports/bench_check.xlsx
EOF
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
    --limit)
      LIMIT="$2"
      shift 2
      ;;
    --only-total)
      ONLY_TOTAL=true
      shift
      ;;
    --exclude-bench)
      EXCLUDE_BENCH=true
      shift
      ;;
    --export)
      EXPORT_DIR="$2"
      shift 2
      ;;
    --export-xlsx)
      EXPORT_XLSX="$2"
      shift 2
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

if ! [[ "$LIMIT" =~ ^[0-9]+$ ]] || (( LIMIT == 0 )); then
  echo "--limit 必须是正整数"
  exit 1
fi

if [[ -n "$RUN_TAG" ]] && [[ ! "$RUN_TAG" =~ ^bench_[0-9]{8}_[0-9]{6}_[0-9]+$ ]]; then
  echo "--run-tag 格式非法: $RUN_TAG"
  echo "示例: bench_20260304_150317_21492"
  exit 1
fi

if [[ -n "$RUN_TAG" && "$EXCLUDE_BENCH" == true ]]; then
  echo "--run-tag 与 --exclude-bench 不能同时使用"
  exit 1
fi

if [[ -n "$EXPORT_XLSX" ]] && [[ "$EXPORT_XLSX" != *.xlsx ]]; then
  echo "--export-xlsx 必须以 .xlsx 结尾"
  exit 1
fi

cleanup_temp_dir() {
  if [[ -n "$TEMP_EXPORT_DIR" && -d "$TEMP_EXPORT_DIR" ]]; then
    rm -rf "$TEMP_EXPORT_DIR"
  fi
}

trap cleanup_temp_dir EXIT

mysql_cmd=(mysql -h "$HOST" -P "$PORT" -u "$DB_USER" --default-character-set=utf8mb4 -D "$DB_NAME")
mysql_raw_cmd=(mysql -h "$HOST" -P "$PORT" -u "$DB_USER" --default-character-set=utf8mb4 -D "$DB_NAME" -N -B --raw)

export_csv() {
  local header="$1"
  local query="$2"
  local out_file="$3"

  printf "%s\n" "$header" >"$out_file"
  MYSQL_PWD="$DB_PASS" "${mysql_raw_cmd[@]}" -e "$query" | awk -F'\t' 'BEGIN{OFS=","} {
    for (i=1; i<=NF; i++) {
      gsub(/"/, "\"\"", $i)
      $i = "\"" $i "\""
    }
    print
  }' >>"$out_file"
}

echo "[1/4] 检查数据库连接..."
MYSQL_PWD="$DB_PASS" "${mysql_cmd[@]}" -e "SELECT 1;" >/dev/null

if [[ -n "$EXPORT_XLSX" && -z "$EXPORT_DIR" ]]; then
  TEMP_EXPORT_DIR="$(mktemp -d /tmp/mychat_inspect_csv_XXXXXX)"
  EXPORT_DIR="$TEMP_EXPORT_DIR"
fi

if [[ -n "$EXPORT_DIR" ]]; then
  mkdir -p "$EXPORT_DIR"
  echo "导出目录: $EXPORT_DIR"
fi

echo "[2/4] 显示全库总量..."
MYSQL_PWD="$DB_PASS" "${mysql_cmd[@]}" -e "
SELECT 'user_total' AS item, COUNT(*) AS cnt FROM user
UNION ALL SELECT 'friend_total', COUNT(*) FROM friend
UNION ALL SELECT 'allgroup_total', COUNT(*) FROM allgroup
UNION ALL SELECT 'groupuser_total', COUNT(*) FROM groupuser;
"

if [[ "$EXCLUDE_BENCH" == true ]]; then
  echo "[2.1/4] 显示非 bench 业务数据总量..."
  MYSQL_PWD="$DB_PASS" "${mysql_cmd[@]}" -e "
SELECT 'biz_user_total' AS item, COUNT(*) AS cnt
FROM user
WHERE name NOT LIKE 'bench_%'
UNION ALL
SELECT 'biz_friend_total', COUNT(*)
FROM friend f
JOIN user u1 ON f.userid=u1.id
JOIN user u2 ON f.friendid=u2.id
WHERE u1.name NOT LIKE 'bench_%' AND u2.name NOT LIKE 'bench_%'
UNION ALL
SELECT 'biz_allgroup_total', COUNT(*)
FROM allgroup
WHERE groupname NOT LIKE 'bench_%'
UNION ALL
SELECT 'biz_groupuser_total', COUNT(*)
FROM groupuser gu
JOIN allgroup g ON gu.groupid=g.id
JOIN user u ON gu.userid=u.id
WHERE g.groupname NOT LIKE 'bench_%' AND u.name NOT LIKE 'bench_%';
"

  if [[ -n "$EXPORT_DIR" ]]; then
    export_csv "item,cnt" "
SELECT 'biz_user_total' AS item, COUNT(*) AS cnt
FROM user
WHERE name NOT LIKE 'bench_%'
UNION ALL
SELECT 'biz_friend_total', COUNT(*)
FROM friend f
JOIN user u1 ON f.userid=u1.id
JOIN user u2 ON f.friendid=u2.id
WHERE u1.name NOT LIKE 'bench_%' AND u2.name NOT LIKE 'bench_%'
UNION ALL
SELECT 'biz_allgroup_total', COUNT(*)
FROM allgroup
WHERE groupname NOT LIKE 'bench_%'
UNION ALL
SELECT 'biz_groupuser_total', COUNT(*)
FROM groupuser gu
JOIN allgroup g ON gu.groupid=g.id
JOIN user u ON gu.userid=u.id
WHERE g.groupname NOT LIKE 'bench_%' AND u.name NOT LIKE 'bench_%';
" "$EXPORT_DIR/business_counts.csv"
  fi
fi

if [[ -n "$EXPORT_DIR" ]]; then
  export_csv "item,cnt" "
SELECT 'user_total' AS item, COUNT(*) AS cnt FROM user
UNION ALL SELECT 'friend_total', COUNT(*) FROM friend
UNION ALL SELECT 'allgroup_total', COUNT(*) FROM allgroup
UNION ALL SELECT 'groupuser_total', COUNT(*) FROM groupuser;
" "$EXPORT_DIR/total_counts.csv"
fi

if [[ "$ONLY_TOTAL" == true ]]; then
  echo "[3/4] 已按 --only-total 结束。"
  exit 0
fi

if [[ "$EXCLUDE_BENCH" == true ]]; then
  echo "[3/4] 查看非 bench 业务数据"
  MYSQL_PWD="$DB_PASS" "${mysql_cmd[@]}" -e "
SELECT id,name,state
FROM user
WHERE name NOT LIKE 'bench_%'
ORDER BY id DESC
LIMIT ${LIMIT};

SELECT id,groupname,groupdesc
FROM allgroup
WHERE groupname NOT LIKE 'bench_%'
ORDER BY id DESC
LIMIT ${LIMIT};

SELECT f.userid, u1.name AS user_name, f.friendid, u2.name AS friend_name
FROM friend f
JOIN user u1 ON f.userid=u1.id
JOIN user u2 ON f.friendid=u2.id
WHERE u1.name NOT LIKE 'bench_%' AND u2.name NOT LIKE 'bench_%'
ORDER BY f.userid DESC, f.friendid DESC
LIMIT ${LIMIT};

SELECT gu.groupid, g.groupname, gu.userid, u.name AS member_name, gu.grouprole
FROM groupuser gu
JOIN allgroup g ON gu.groupid=g.id
JOIN user u ON gu.userid=u.id
WHERE g.groupname NOT LIKE 'bench_%' AND u.name NOT LIKE 'bench_%'
ORDER BY gu.groupid DESC, gu.userid DESC
LIMIT ${LIMIT};
"

  if [[ -n "$EXPORT_DIR" ]]; then
    export_csv "id,name,state" "
SELECT id,name,state
FROM user
WHERE name NOT LIKE 'bench_%'
ORDER BY id DESC
LIMIT ${LIMIT};
" "$EXPORT_DIR/sample_users.csv"

    export_csv "id,groupname,groupdesc" "
SELECT id,groupname,groupdesc
FROM allgroup
WHERE groupname NOT LIKE 'bench_%'
ORDER BY id DESC
LIMIT ${LIMIT};
" "$EXPORT_DIR/sample_groups.csv"

    export_csv "userid,user_name,friendid,friend_name" "
SELECT f.userid, u1.name AS user_name, f.friendid, u2.name AS friend_name
FROM friend f
JOIN user u1 ON f.userid=u1.id
JOIN user u2 ON f.friendid=u2.id
WHERE u1.name NOT LIKE 'bench_%' AND u2.name NOT LIKE 'bench_%'
ORDER BY f.userid DESC, f.friendid DESC
LIMIT ${LIMIT};
" "$EXPORT_DIR/sample_friends.csv"

    export_csv "groupid,groupname,userid,member_name,grouprole" "
SELECT gu.groupid, g.groupname, gu.userid, u.name AS member_name, gu.grouprole
FROM groupuser gu
JOIN allgroup g ON gu.groupid=g.id
JOIN user u ON gu.userid=u.id
WHERE g.groupname NOT LIKE 'bench_%' AND u.name NOT LIKE 'bench_%'
ORDER BY gu.groupid DESC, gu.userid DESC
LIMIT ${LIMIT};
" "$EXPORT_DIR/sample_group_members.csv"
  fi

  if [[ -n "$EXPORT_DIR" ]]; then
    echo "CSV 导出完成: $EXPORT_DIR"
  fi

  if [[ -n "$EXPORT_XLSX" ]]; then
    mkdir -p "$(dirname "$EXPORT_XLSX")"
    if ! command -v python3 >/dev/null 2>&1; then
    echo "未找到 python3，无法生成 XLSX。"
    echo "你可以先安装 python3，或仅使用 --export 导出 CSV。"
    exit 1
    fi

    echo "开始生成 XLSX: $EXPORT_XLSX"
    if ! python3 - "$EXPORT_DIR" "$EXPORT_XLSX" <<'PY'; then
import csv
import os
import sys

csv_dir = sys.argv[1]
xlsx_file = sys.argv[2]

try:
  from openpyxl import Workbook
except Exception:
  print("缺少 Python 依赖 openpyxl，先执行: python3 -m pip install --user openpyxl")
  sys.exit(1)

files = [
  ("total_counts.csv", "total_counts"),
  ("business_counts.csv", "business_counts"),
  ("sample_users.csv", "sample_users"),
  ("sample_groups.csv", "sample_groups"),
  ("sample_friends.csv", "sample_friends"),
  ("sample_group_members.csv", "sample_group_members"),
]

wb = Workbook()
first_sheet = True

for file_name, sheet_name in files:
  path = os.path.join(csv_dir, file_name)
  if not os.path.exists(path):
    continue

  if first_sheet:
    ws = wb.active
    ws.title = sheet_name
    first_sheet = False
  else:
    ws = wb.create_sheet(title=sheet_name)

  with open(path, "r", encoding="utf-8", newline="") as f:
    reader = csv.reader(f)
    for row in reader:
      ws.append(row)

if not wb.sheetnames:
  ws = wb.active
  ws.title = "empty"
  ws.append(["no data"])

wb.save(xlsx_file)
print(f"XLSX generated: {xlsx_file}")
PY
    exit 1
    fi

    echo "XLSX 导出完成: $EXPORT_XLSX"
  fi

  exit 0
fi

if [[ -n "$RUN_TAG" ]]; then
  tag_filter_user="name LIKE '${RUN_TAG}_u_%'"
  tag_filter_group="groupname LIKE '${RUN_TAG}_g_%'"
  echo "[3/4] 查看指定批次: ${RUN_TAG}"
else
  tag_filter_user="name REGEXP '^bench_[0-9]{8}_[0-9]{6}_[0-9]+_u_'"
  tag_filter_group="groupname REGEXP '^bench_[0-9]{8}_[0-9]{6}_[0-9]+_g_'"
  echo "[3/4] 查看所有压测批次概览"
fi

MYSQL_PWD="$DB_PASS" "${mysql_cmd[@]}" -e "
SELECT tag,
       SUM(user_cnt) AS users,
  SUM(group_cnt) AS group_count
FROM (
  SELECT SUBSTRING_INDEX(name, '_u_', 1) AS tag, COUNT(*) AS user_cnt, 0 AS group_cnt
  FROM user
  WHERE ${tag_filter_user}
  GROUP BY SUBSTRING_INDEX(name, '_u_', 1)

  UNION ALL

  SELECT SUBSTRING_INDEX(groupname, '_g_', 1) AS tag, 0 AS user_cnt, COUNT(*) AS group_cnt
  FROM allgroup
  WHERE ${tag_filter_group}
  GROUP BY SUBSTRING_INDEX(groupname, '_g_', 1)
) t
GROUP BY tag
ORDER BY tag DESC;
"

if [[ -n "$EXPORT_DIR" ]]; then
  export_csv "tag,users,group_count" "
SELECT tag,
       SUM(user_cnt) AS users,
       SUM(group_cnt) AS group_count
FROM (
  SELECT SUBSTRING_INDEX(name, '_u_', 1) AS tag, COUNT(*) AS user_cnt, 0 AS group_cnt
  FROM user
  WHERE ${tag_filter_user}
  GROUP BY SUBSTRING_INDEX(name, '_u_', 1)

  UNION ALL

  SELECT SUBSTRING_INDEX(groupname, '_g_', 1) AS tag, 0 AS user_cnt, COUNT(*) AS group_cnt
  FROM allgroup
  WHERE ${tag_filter_group}
  GROUP BY SUBSTRING_INDEX(groupname, '_g_', 1)
) t
GROUP BY tag
ORDER BY tag DESC;
" "$EXPORT_DIR/tag_summary.csv"
fi

echo "[4/4] 样例数据 (limit=${LIMIT})"

if [[ -n "$RUN_TAG" ]]; then
  MYSQL_PWD="$DB_PASS" "${mysql_cmd[@]}" -e "
SELECT id,name,state
FROM user
WHERE name LIKE '${RUN_TAG}_u_%'
ORDER BY id DESC
LIMIT ${LIMIT};

SELECT id,groupname,groupdesc
FROM allgroup
WHERE groupname LIKE '${RUN_TAG}_g_%'
ORDER BY id DESC
LIMIT ${LIMIT};

SELECT f.userid, u1.name AS user_name, f.friendid, u2.name AS friend_name
FROM friend f
JOIN user u1 ON f.userid=u1.id
JOIN user u2 ON f.friendid=u2.id
WHERE u1.name LIKE '${RUN_TAG}_u_%'
ORDER BY f.userid DESC, f.friendid DESC
LIMIT ${LIMIT};

SELECT gu.groupid, g.groupname, gu.userid, u.name AS member_name, gu.grouprole
FROM groupuser gu
JOIN allgroup g ON gu.groupid=g.id
JOIN user u ON gu.userid=u.id
WHERE g.groupname LIKE '${RUN_TAG}_g_%'
ORDER BY gu.groupid DESC, gu.userid DESC
LIMIT ${LIMIT};
"

  if [[ -n "$EXPORT_DIR" ]]; then
    export_csv "id,name,state" "
SELECT id,name,state
FROM user
WHERE name LIKE '${RUN_TAG}_u_%'
ORDER BY id DESC
LIMIT ${LIMIT};
" "$EXPORT_DIR/sample_users.csv"

    export_csv "id,groupname,groupdesc" "
SELECT id,groupname,groupdesc
FROM allgroup
WHERE groupname LIKE '${RUN_TAG}_g_%'
ORDER BY id DESC
LIMIT ${LIMIT};
" "$EXPORT_DIR/sample_groups.csv"

    export_csv "userid,user_name,friendid,friend_name" "
SELECT f.userid, u1.name AS user_name, f.friendid, u2.name AS friend_name
FROM friend f
JOIN user u1 ON f.userid=u1.id
JOIN user u2 ON f.friendid=u2.id
WHERE u1.name LIKE '${RUN_TAG}_u_%'
ORDER BY f.userid DESC, f.friendid DESC
LIMIT ${LIMIT};
" "$EXPORT_DIR/sample_friends.csv"

    export_csv "groupid,groupname,userid,member_name,grouprole" "
SELECT gu.groupid, g.groupname, gu.userid, u.name AS member_name, gu.grouprole
FROM groupuser gu
JOIN allgroup g ON gu.groupid=g.id
JOIN user u ON gu.userid=u.id
WHERE g.groupname LIKE '${RUN_TAG}_g_%'
ORDER BY gu.groupid DESC, gu.userid DESC
LIMIT ${LIMIT};
" "$EXPORT_DIR/sample_group_members.csv"
  fi
else
  MYSQL_PWD="$DB_PASS" "${mysql_cmd[@]}" -e "
SELECT id,name,state
FROM user
WHERE name REGEXP '^bench_[0-9]{8}_[0-9]{6}_[0-9]+_u_'
ORDER BY id DESC
LIMIT ${LIMIT};

SELECT id,groupname,groupdesc
FROM allgroup
WHERE groupname REGEXP '^bench_[0-9]{8}_[0-9]{6}_[0-9]+_g_'
ORDER BY id DESC
LIMIT ${LIMIT};

SELECT f.userid, u1.name AS user_name, f.friendid, u2.name AS friend_name
FROM friend f
JOIN user u1 ON f.userid=u1.id
JOIN user u2 ON f.friendid=u2.id
WHERE u1.name REGEXP '^bench_[0-9]{8}_[0-9]{6}_[0-9]+_u_'
ORDER BY f.userid DESC, f.friendid DESC
LIMIT ${LIMIT};

SELECT gu.groupid, g.groupname, gu.userid, u.name AS member_name, gu.grouprole
FROM groupuser gu
JOIN allgroup g ON gu.groupid=g.id
JOIN user u ON gu.userid=u.id
WHERE g.groupname REGEXP '^bench_[0-9]{8}_[0-9]{6}_[0-9]+_g_'
ORDER BY gu.groupid DESC, gu.userid DESC
LIMIT ${LIMIT};
"

  if [[ -n "$EXPORT_DIR" ]]; then
    export_csv "id,name,state" "
SELECT id,name,state
FROM user
WHERE name REGEXP '^bench_[0-9]{8}_[0-9]{6}_[0-9]+_u_'
ORDER BY id DESC
LIMIT ${LIMIT};
" "$EXPORT_DIR/sample_users.csv"

    export_csv "id,groupname,groupdesc" "
SELECT id,groupname,groupdesc
FROM allgroup
WHERE groupname REGEXP '^bench_[0-9]{8}_[0-9]{6}_[0-9]+_g_'
ORDER BY id DESC
LIMIT ${LIMIT};
" "$EXPORT_DIR/sample_groups.csv"

    export_csv "userid,user_name,friendid,friend_name" "
SELECT f.userid, u1.name AS user_name, f.friendid, u2.name AS friend_name
FROM friend f
JOIN user u1 ON f.userid=u1.id
JOIN user u2 ON f.friendid=u2.id
WHERE u1.name REGEXP '^bench_[0-9]{8}_[0-9]{6}_[0-9]+_u_'
ORDER BY f.userid DESC, f.friendid DESC
LIMIT ${LIMIT};
" "$EXPORT_DIR/sample_friends.csv"

    export_csv "groupid,groupname,userid,member_name,grouprole" "
SELECT gu.groupid, g.groupname, gu.userid, u.name AS member_name, gu.grouprole
FROM groupuser gu
JOIN allgroup g ON gu.groupid=g.id
JOIN user u ON gu.userid=u.id
WHERE g.groupname REGEXP '^bench_[0-9]{8}_[0-9]{6}_[0-9]+_g_'
ORDER BY gu.groupid DESC, gu.userid DESC
LIMIT ${LIMIT};
" "$EXPORT_DIR/sample_group_members.csv"
  fi
fi

if [[ -n "$EXPORT_DIR" ]]; then
  echo "CSV 导出完成: $EXPORT_DIR"
fi

if [[ -n "$EXPORT_XLSX" ]]; then
  mkdir -p "$(dirname "$EXPORT_XLSX")"
  if ! command -v python3 >/dev/null 2>&1; then
  echo "未找到 python3，无法生成 XLSX。"
  echo "你可以先安装 python3，或仅使用 --export 导出 CSV。"
  exit 1
  fi

  echo "开始生成 XLSX: $EXPORT_XLSX"
  if ! python3 - "$EXPORT_DIR" "$EXPORT_XLSX" <<'PY'; then
import csv
import os
import sys

csv_dir = sys.argv[1]
xlsx_file = sys.argv[2]

try:
  from openpyxl import Workbook
except Exception:
  print("缺少 Python 依赖 openpyxl，先执行: python3 -m pip install --user openpyxl")
  sys.exit(1)

files = [
  ("total_counts.csv", "total_counts"),
  ("tag_summary.csv", "tag_summary"),
  ("sample_users.csv", "sample_users"),
  ("sample_groups.csv", "sample_groups"),
  ("sample_friends.csv", "sample_friends"),
  ("sample_group_members.csv", "sample_group_members"),
]

wb = Workbook()
first_sheet = True

for file_name, sheet_name in files:
  path = os.path.join(csv_dir, file_name)
  if not os.path.exists(path):
    continue

  if first_sheet:
    ws = wb.active
    ws.title = sheet_name
    first_sheet = False
  else:
    ws = wb.create_sheet(title=sheet_name)

  with open(path, "r", encoding="utf-8", newline="") as f:
    reader = csv.reader(f)
    for row in reader:
      ws.append(row)

if not wb.sheetnames:
  ws = wb.active
  ws.title = "empty"
  ws.append(["no data"])

wb.save(xlsx_file)
print(f"XLSX generated: {xlsx_file}")
PY
  exit 1
  fi

  echo "XLSX 导出完成: $EXPORT_XLSX"
fi
