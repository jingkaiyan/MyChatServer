# MyChatServer 部署与运行指南（全项目版）

## 目录
1. 项目组成与目录结构
2. 环境要求
3. 数据库初始化与迁移
4. 构建与启动
5. 运行参数与配置
6. 可观测与压测工具
7. AI 模型接入（可选）
8. 常见问题排查

---

## 1) 项目组成与目录结构

```text
MyChatServer/
├── ai/                         # AI 模型编排
├── bin/                        # 二进制产物
├── build/                      # CMake 构建目录
├── exports/                    # 指标与报告输出
├── include/                    # 头文件
├── logs/                       # 服务日志
├── monitoring/                 # Prometheus/Grafana 配置
├── run/                        # PID 与运行时文件
├── scripts/                    # 启停、检查、压测、导出脚本
├── src/client/                 # Qt 客户端
├── src/server/                 # ChatServer 服务端
├── thirdparty/                 # 第三方头文件
├── autobuild.sh                # 一键构建
├── README.md
├── DEPLOYMENT.md
└── PROJECT_RESUME.md
```

---

## 2) 环境要求

### Linux（推荐 Ubuntu 20.04+）

```bash
sudo apt update
sudo apt install -y build-essential cmake git \
  mysql-server redis-server default-libmysqlclient-dev libhiredis-dev
```

### Qt 客户端依赖

```bash
# Qt5
sudo apt install -y qtbase5-dev qtbase5-dev-tools

# 或 Qt6
sudo apt install -y qt6-base-dev qt6-base-dev-tools
```

> Muduo 需确保已安装并可被链接（`-lmuduo_net -lmuduo_base`）。

---

## 3) 数据库初始化与迁移

### 3.1 创建数据库与表

```sql
CREATE DATABASE IF NOT EXISTS chat CHARACTER SET utf8mb4;
USE chat;

CREATE TABLE IF NOT EXISTS user (
  id INT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(50) NOT NULL UNIQUE,
  password VARCHAR(64) NOT NULL,
  state ENUM('online','offline') DEFAULT 'offline'
);

CREATE TABLE IF NOT EXISTS friend (
  userid INT NOT NULL,
  friendid INT NOT NULL,
  PRIMARY KEY(userid, friendid)
);

CREATE TABLE IF NOT EXISTS allgroup (
  id INT PRIMARY KEY AUTO_INCREMENT,
  groupname VARCHAR(50) NOT NULL UNIQUE,
  groupdesc VARCHAR(200) DEFAULT ''
);

CREATE TABLE IF NOT EXISTS groupuser (
  groupid INT NOT NULL,
  userid INT NOT NULL,
  grouprole ENUM('creator','normal') DEFAULT 'normal',
  PRIMARY KEY(groupid, userid)
);

CREATE TABLE IF NOT EXISTS offlinemessage (
  userid INT NOT NULL,
  message TEXT NOT NULL
);
```

### 3.2 老库迁移（密码列长度）

```sql
ALTER TABLE user MODIFY COLUMN password VARCHAR(64) NOT NULL;
```

### 3.3 一次性明文密码批量迁移（可选）

```sql
UPDATE user
SET password = SHA2(password, 256)
WHERE password NOT REGEXP '^[0-9a-fA-F]{64}$';
```

---

## 4) 构建与启动

### 4.1 编译

```bash
cd MyChatServer
BUILD_SERVER=ON BUILD_CLIENT=ON ./autobuild.sh
```

### 4.2 启动服务端（推荐）

```bash
cd MyChatServer
cp ./scripts/server.env.example ./scripts/server.env
./scripts/run_server.sh --daemon --env-file ./scripts/server.env
```

### 4.3 健康检查

```bash
./scripts/health_check.sh
```

### 4.4 启动客户端

```bash
./bin/ChatClient
```

### 4.5 停止服务端

```bash
./scripts/stop_server.sh
```

---

## 5) 运行参数与配置

### 服务端地址
- `MYCHAT_SERVER_HOST`（默认 `127.0.0.1`）
- `MYCHAT_SERVER_PORT`（默认 `8000`）

### MySQL 连接参数
- `MYCHAT_DB_HOST`
- `MYCHAT_DB_PORT`
- `MYCHAT_DB_USER`
- `MYCHAT_DB_PASS`
- `MYCHAT_DB_NAME`

### AI 参数（可选）
- `MYCHAT_AI_HOST`
- `MYCHAT_AI_PORT`
- `MYCHAT_AI_MODEL`
- `MYCHAT_AI_MAX_ROUNDS`
- `MYCHAT_AI_TIMEOUT_SEC`

---

## 6) 可观测与压测工具

### 指标快照

```bash
./scripts/metrics_snapshot.sh ./exports/runtime_metrics.prom
```

### 启动本地观测演示栈

```bash
./scripts/start_observability_demo.sh
./scripts/stop_observability_demo.sh
```

访问：
- Prometheus: `http://127.0.0.1:9090`
- Grafana: `http://127.0.0.1:3000`（`admin/admin123`）

### 压测检查与报告

```bash
./scripts/generate_benchmark_data.sh --help
./scripts/inspect_benchmark_data.sh --help
./scripts/compare_benchmark_runs.sh --help
./scripts/generate_benchmark_visual_report.sh --help
```

---

## 7) AI 模型接入（可选）

```bash
./scripts/start_ai_model.sh
./scripts/ask_ai_model.sh "请介绍该项目"
./scripts/stop_ai_model.sh
```

---

## 8) 常见问题排查

### 8.1 服务端启动失败
- 检查端口占用：`ss -ltnp | grep 8000`
- 检查日志：`tail -n 200 ./logs/chatserver.log`

### 8.2 客户端无法连接
- 确认服务端监听地址与端口
- 检查防火墙与本机网络

### 8.3 MySQL / Redis 异常
- MySQL：`systemctl status mysql`
- Redis：`systemctl status redis-server`

### 8.4 指标导出失败
- 确认服务端进程存在
- 检查 `./exports/` 写权限

---

如需完整项目介绍与能力边界，请同步参考 `README.md` 与 `PROJECT_RESUME.md`。
