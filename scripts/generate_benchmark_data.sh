#!/usr/bin/env bash
set -euo pipefail

HOST="127.0.0.1"
PORT="3306"
DB_USER="chat"
DB_PASS="chat123"
DB_NAME="chat"

USER_COUNT=20000
FRIENDS_PER_USER=20
GROUP_COUNT=3000
MEMBERS_PER_GROUP=60

USER_BATCH=1000

RUN_TAG="bench_$(date +%Y%m%d_%H%M%S)_$RANDOM"

usage() {
  cat <<'EOF'
用法:
  ./scripts/generate_benchmark_data.sh [options]

可选参数:
  --host <host>                   MySQL 主机 (默认: 127.0.0.1)
  --port <port>                   MySQL 端口 (默认: 3306)
  --db-user <user>                MySQL 用户 (默认: chat)
  --db-pass <password>            MySQL 密码 (默认: chat123)
  --db-name <name>                数据库名 (默认: chat)
  --users <count>                 新增用户数量 (默认: 20000)
  --friends-per-user <count>      每个用户新增好友数量 (默认: 20)
  --groups <count>                新增群组数量 (默认: 3000)
  --members-per-group <count>     每个群新增成员数量(不含群主) (默认: 60)

示例:
  ./scripts/generate_benchmark_data.sh
  ./scripts/generate_benchmark_data.sh --users 50000 --groups 8000
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
    --users)
      USER_COUNT="$2"
      shift 2
      ;;
    --friends-per-user)
      FRIENDS_PER_USER="$2"
      shift 2
      ;;
    --groups)
      GROUP_COUNT="$2"
      shift 2
      ;;
    --members-per-group)
      MEMBERS_PER_GROUP="$2"
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

for val_name in USER_COUNT FRIENDS_PER_USER GROUP_COUNT MEMBERS_PER_GROUP; do
  val="${!val_name}"
  if ! [[ "$val" =~ ^[0-9]+$ ]]; then
    echo "$val_name 必须是非负整数，当前值: $val"
    exit 1
  fi
done

if (( USER_COUNT == 0 )); then
  echo "--users 不能为 0"
  exit 1
fi

mysql_cmd=(mysql -h "$HOST" -P "$PORT" -u "$DB_USER" --default-character-set=utf8mb4 -N -s "$DB_NAME")

run_scalar_sql() {
  MYSQL_PWD="$DB_PASS" "${mysql_cmd[@]}" -e "$1"
}

echo "[1/5] 检查数据库连接..."
run_scalar_sql "SELECT 1;" >/dev/null

echo "[2/5] 生成本次批次标识: ${RUN_TAG}"

tmp_sql="$(mktemp /tmp/mychat_benchmark_data_XXXXXX.sql)"
trap 'rm -f "$tmp_sql"' EXIT

echo "[3/5] 生成 SQL 文件: $tmp_sql"
{
  echo "SET NAMES utf8mb4;"
  echo "START TRANSACTION;"
} >"$tmp_sql"

# 1) 批量插入用户
for ((base = 0; base < USER_COUNT; base += USER_BATCH)); do
  end=$((base + USER_BATCH))
  if (( end > USER_COUNT )); then
    end=$USER_COUNT
  fi

  printf "INSERT INTO user(name,password,state) VALUES" >>"$tmp_sql"
  first=1
  for ((i = base; i < end; ++i)); do
    serial=$((i + 1))
    username="${RUN_TAG}_u_$(printf '%06d' "$serial")"
    if (( first == 1 )); then
      printf "('%s','123456','offline')" "$username" >>"$tmp_sql"
      first=0
    else
      printf ",('%s','123456','offline')" "$username" >>"$tmp_sql"
    fi
  done
  echo ";" >>"$tmp_sql"
done

# 2) 批量插入好友关系（双向）
if (( FRIENDS_PER_USER > 0 )); then
  cat >>"$tmp_sql" <<SQL
CREATE TEMPORARY TABLE tmp_seed_user AS
SELECT id, (@u_rn:=@u_rn+1) AS rn
FROM (SELECT id FROM user WHERE name LIKE '${RUN_TAG}_u_%' ORDER BY id) x,
     (SELECT @u_rn:=0) vars;

CREATE TEMPORARY TABLE tmp_seed_user_b AS
SELECT id, rn FROM tmp_seed_user;

SET @user_total := (SELECT COUNT(*) FROM tmp_seed_user);
SQL

  for ((k = 1; k <= FRIENDS_PER_USER; ++k)); do
    cat >>"$tmp_sql" <<SQL
INSERT IGNORE INTO friend(userid,friendid)
SELECT u1.id, u2.id
FROM tmp_seed_user u1
JOIN tmp_seed_user_b u2 ON u2.rn = ((u1.rn + ${k} - 1) % @user_total) + 1;

INSERT IGNORE INTO friend(userid,friendid)
SELECT u2.id, u1.id
FROM tmp_seed_user u1
JOIN tmp_seed_user_b u2 ON u2.rn = ((u1.rn + ${k} - 1) % @user_total) + 1;
SQL
  done
fi

# 3) 批量插入群组
if (( GROUP_COUNT > 0 )); then
  for ((g = 0; g < GROUP_COUNT; g += USER_BATCH)); do
    gend=$((g + USER_BATCH))
    if (( gend > GROUP_COUNT )); then
      gend=$GROUP_COUNT
    fi

    printf "INSERT INTO allgroup(groupname,groupdesc) VALUES" >>"$tmp_sql"
    first=1
    for ((i = g; i < gend; ++i)); do
      serial=$((i + 1))
      gname="${RUN_TAG}_g_$(printf '%06d' "$serial")"
      if (( first == 1 )); then
        printf "('%s','benchmark group %s')" "$gname" "$gname" >>"$tmp_sql"
        first=0
      else
        printf ",('%s','benchmark group %s')" "$gname" "$gname" >>"$tmp_sql"
      fi
    done
    echo ";" >>"$tmp_sql"
  done

  cat >>"$tmp_sql" <<SQL
CREATE TEMPORARY TABLE tmp_seed_group AS
SELECT id, (@g_rn:=@g_rn+1) AS rn
FROM (SELECT id FROM allgroup WHERE groupname LIKE '${RUN_TAG}_g_%' ORDER BY id) x,
     (SELECT @g_rn:=0) vars;

SET @group_total := (SELECT COUNT(*) FROM tmp_seed_group);

INSERT IGNORE INTO groupuser(groupid,userid,grouprole)
SELECT g.id, u.id, 'creator'
FROM tmp_seed_group g
JOIN tmp_seed_user u ON u.rn = ((g.rn - 1) % @user_total) + 1;
SQL

  for ((m = 1; m <= MEMBERS_PER_GROUP; ++m)); do
    cat >>"$tmp_sql" <<SQL
INSERT IGNORE INTO groupuser(groupid,userid,grouprole)
SELECT g.id, u.id, 'normal'
FROM tmp_seed_group g
JOIN tmp_seed_user u ON u.rn = ((g.rn + ${m} - 1) % @user_total) + 1;
SQL
  done
fi

echo "COMMIT;" >>"$tmp_sql"

echo "[4/5] 执行 SQL（可能需要几分钟）..."
MYSQL_PWD="$DB_PASS" "${mysql_cmd[@]}" <"$tmp_sql"

echo "[5/5] 统计结果..."
new_user_count="$(run_scalar_sql "SELECT COUNT(*) FROM user WHERE name LIKE '${RUN_TAG}_u_%';")"
new_friend_count="$(run_scalar_sql "SELECT COUNT(*) FROM friend f JOIN user u ON f.userid=u.id WHERE u.name LIKE '${RUN_TAG}_u_%';")"

echo "完成!"
echo "  批次标识: ${RUN_TAG}"
echo "  新增用户: ${new_user_count}"
echo "  相关好友关系行(按 userid 在新用户范围统计): ${new_friend_count}"

if (( GROUP_COUNT > 0 )); then
  new_group_count="$(run_scalar_sql "SELECT COUNT(*) FROM allgroup WHERE groupname LIKE '${RUN_TAG}_g_%';")"
  new_groupuser_count="$(run_scalar_sql "SELECT COUNT(*) FROM groupuser gu JOIN allgroup g ON gu.groupid=g.id WHERE g.groupname LIKE '${RUN_TAG}_g_%';")"
  echo "  新增群组: ${new_group_count}"
  echo "  新增群成员关系: ${new_groupuser_count}"
fi
