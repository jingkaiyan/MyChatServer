# MyChatServer 部署与使用完全指南

<p align="center">
  <b>从环境配置到生产部署的详细步骤</b><br/>
  <sub>包含单机部署、多实例分布式部署、容器化部署、常见问题排查</sub>
</p>

---

## 📋 快速导航

| 章节 | 用途 | 适用场景 |
|---|---|---|
| [环境要求](#环境要求) | 系统依赖检查清单 | 首次部署 |
| [Linux 服务端编译](#linux-服务端编译与部署) | 服务端编译与启动 | 生产环境部署 |
| [Qt 客户端编译](#qt-客户端编译与运行) | 客户端各平台编译指南 | 客户端开发 |
| [数据库配置](#数据库配置) | MySQL 初始化与表结构 | 数据库初始化 |
| [Redis 配置](#redis配置-多实例部署) | Redis Pub/Sub 设置 | 分布式部署 |
| [功能测试](#功能测试指南) | 端到端功能验证 | 联调测试 |
| [常见问题](#常见问题与解决方案) | 排查与解决 | 遇到问题 |
| [高级配置](#高级配置与优化) | 性能调优、容器化 | 进阶优化 |
| [协议文档](#协议文档) | JSON 消息格式详解 | 客户端开发 |

---

## 项目架构说明

### 技术栈
- **服务端**: C++11 + Muduo 网络库 + MySQL + Redis(可选)
- **客户端**: Qt5/Qt6 + QTcpSocket + nlohmann/json
- **通信协议**: JSON over TCP
- **构建系统**: CMake

### 目录结构
```
MyChatServer/
├── src/
│   ├── server/          # 服务端代码(Linux专用)
│   │   ├── main.cpp
│   │   ├── chatserver.cpp
│   │   ├── chatservice.cpp
│   │   ├── db/          # MySQL数据库操作
│   │   ├── model/       # 数据模型(User, Friend, Group)
│   │   └── redis/       # Redis Pub/Sub(可选)
│   └── client/          # Qt客户端代码(跨平台)
│       ├── main.cpp
│       ├── ui/          # UI界面(LoginDialog, MainWindow)
│       └── network/     # 网络通信层(NetworkClient)
├── include/
│   ├── public.hpp       # 协议消息类型定义
│   └── server/          # 服务端头文件
├── thirdparty/
│   └── json.hpp         # nlohmann::json库
├── scripts/
│   └── chat.sql         # 数据库初始化脚本
└── CMakeLists.txt
```

---

## 环境要求
### 系统要求

| 组件 | 要求 | 备注 |
|---|---|---|
| 操作系统 | Linux（Ubuntu 20.04+ 推荐）或 WSL2 | 服务端 Linux 专用；客户端跨平台 |
| C++ 标准 | C++11 或更高 | GCC 5.0+、Clang 3.8+、MSVC 2017+ |
| CMake | 3.10+ | 构建系统 |
| RAM | 最低 512MB，推荐 2GB+ | 开发编译环境 |
| 磁盘 | 最少 2GB 空闲空间 | 编译与运行时所需 |

### 网络要求

| 端口 | 用途 | 范例 |
|---|---|---|
| 8000 | ChatServer 默认端口 | 可配置 |
| 3306 | MySQL 服务端口 | 默认 |
| 6379 | Redis 服务端口 | 可选（分布式部署时需要） |

### 依赖软件清单

```
服务端依赖树
├─ 编译工具
│  ├─ GCC/G++ 5.0+
│  ├─ CMake 3.10+
│  └─ Make 3.80+
├─ 运行时库
│  ├─ Muduo 网络库 ⭐ （关键）
│  ├─ MySQL 客户端库
│  └─ Redis 客户端库 (libhiredis) — 可选
└─ 数据库服务
   ├─ MySQL 5.7+ 或 8.0
   └─ Redis 4.0+ — 可选（分布式部署）

客户端依赖树
├─ 编译工具
│  ├─ GCC/G++ / Clang / MSVC
│  └─ CMake 3.10+
└─ 运行时库
   ├─ Qt5.15+ 或 Qt6.x
   │  ├─ QtCore
   │  ├─ QtGui
   │  ├─ QtWidgets
   │  └─ QtNetwork ⭐
   └─ C++ 标准库
```

### 安装指令汇总

#### Ubuntu/Debian (推荐)

```bash
# 系统包管理器安装
sudo apt update

# 方案 A：逐个安装
sudo apt install -y \
  build-essential cmake \
  default-libmysqlclient-dev \
  libhiredis-dev \
  redis-server \
  mysql-server

# 方案 B：使用脚本快速安装（见下文）
bash scripts/install-deps-ubuntu.sh

# Muduo（需要手动编译）
cd /tmp && git clone https://github.com/chenshuo/muduo.git
cd muduo && ./build.sh
cd build/release-cpp11 && sudo make install && sudo ldconfig
```

#### CentOS/RHEL

```bash
sudo yum update

# 开发工具组
sudo yum groupinstall -y "Development Tools"

# 依赖包
sudo yum install -y \
  cmake \
  mysql-devel \
  hiredis-devel \
  redis \
  mysql-server

# Muduo（需要手动编译）
# 参考 Ubuntu 安装步骤
```

#### Windows (WSL2 + Ubuntu)

```powershell
# 在 PowerShell 中启动 WSL2
wsl.exe -d Ubuntu-20.04

# 然后在 Ubuntu 终端中执行上述 Ubuntu 安装命令
```

#### macOS

```bash
# 使用 Homebrew
brew install cmake mysql@8.0 redis qt

# 或从源码编译 Muduo（不支持 macOS 原生编译，需 Docker）
```

---

## Linux 服务端编译与部署

### 第 1 步：安装 Muduo 库

Muduo 是网络框架的核心，需要从源码编译。这是最容易出问题的步骤。

```bash
# 保存当前目录
WORK_DIR=$(pwd)

# 进入临时目录编译
cd /tmp

# 克隆源码
git clone https://github.com/chenshuo/muduo.git
cd muduo

# 生成编译产物
./build.sh

# 编译并安装
cd build/release-cpp11
sudo make install

# 更新系统动态链接库缓存（重要！否则链接失败）
sudo ldconfig

# 验证安装是否成功
pkg-config --libs muduo_net muduo_base  # 应该输出库文件路径
ls -l /usr/local/lib | grep muduo      # 应该能看到 libmuduo* 文件

# 返回原目录
cd $WORK_DIR
```

**验证 Muduo 安装**：
```bash
# 检查头文件
ls /usr/local/include/muduo/

# 检查库文件
pkg-config --list-all | grep muduo

# 如果 pkg-config 找不到，检查 lib 目录
sudo find /usr/local -name "libmuduo*"
```

**常见问题**：
- ❌ `Cannot find -lmuduo_net`：运行 `sudo ldconfig`
- ❌ `pkg-config: command not found`：安装 `pkg-config`

### 第 2 步：配置数据库

```bash
# 启动 MySQL 服务
sudo systemctl start mysql

# 进入 MySQL
mysql -u root -p  # 输入密码，默认可能为空

# 在 MySQL 终端中执行
source /path/to/MyChatServer/scripts/chat.sql
EXIT;

# 验证数据库创建
mysql -u root -p -e "USE chat; SHOW TABLES;"
```

### 第 3 步：修改数据库连接参数

编辑 [MyChatServer/src/server/db/db.cpp](src/server/db/db.cpp)，找到以下代码：

```cpp
bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(_conn, 
        "127.0.0.1",          // ← 改为你的数据库服务器 IP
        "root",               // ← 改为你的 MySQL 用户名
        "your_password",      // ← 改为你的 MySQL 密码（重要！）
        "chat",               // ← 数据库名（通常不需要改）
        3306,                 // ← 端口（通常不需要改）
        nullptr, 0);
    
    if (p != nullptr)
    {
        // mysql_set_character_set(_conn, "utf8");
        LOG_INFO << "mysql connect success";
    }
    else
    {
        LOG_ERROR << "mysql connect fail";
    }
    return p != nullptr;
}
```

**重要提示**：
- 如果 MySQL 密码为空，改为 `""`（空字符串）
- 如果 MySQL 不在默认端口，改为实际端口号
- 本地开发通常保持 `127.0.0.1` 和 `3306`

### 第 4 步：编译服务端

```bash
cd /path/to/MyChatServer

# 方式 A：使用自动化脚本（推荐）
chmod +x autobuild.sh
./autobuild.sh
# 输出应该包含: "ChatServer" 并在 bin 目录下生成可执行文件

# 方式 B：手动 CMake 构建
mkdir -p build && cd build
cmake -DBUILD_SERVER=ON -DBUILD_CLIENT=OFF \
       -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)  # -j 参数使用多核编译，加快速度

# 验证编译成功
ls -lh ../bin/ChatServer
```

**编译参数说明**：
```bash
-DBUILD_SERVER=ON      # 编译服务端
-DBUILD_CLIENT=OFF     # 不编译客户端
-DCMAKE_BUILD_TYPE=Release  # 发布版本（优化），Debug 版本去掉或改为 Debug
-j$(nproc)             # 多核并行编译
```

**常见编译错误**：

| 错误信息 | 原因 | 解决方案 |
|---|---|---|
| `cannot find -lmuduo_net` | Muduo 未安装或链接路径错误 | 执行 `sudo ldconfig` 或重新编译 Muduo |
| `undefined reference to 'mysql_init'` | MySQL 客户端库未找到 | `sudo apt install default-libmysqlclient-dev` |
| `CMake Error: generator not found` | CMake 版本过旧或生成器名称错误 | 升级 CMake 或改用 `Unix Makefiles` |

### 第 5 步：启动服务端

```bash
# 启动数据库服务（如果未运行）
sudo systemctl start mysql
sudo systemctl start redis-server  # 如果需要 Redis 分布式

# 启动服务端
cd /path/to/MyChatServer
./bin/ChatServer

# 预期输出（示例）：
# 20250110 14:30:45.123456Z  7534 INFO  [muduo] net - Acceptor::listen - EventLoop: 0x5555555b9e20 listening on *:8000
# 20250110 14:30:45.123597Z  7534 INFO  [muduo] net - TcpServer::start() - started at 8000 with 4 accepting threads.
# [INFO] ChatService singleton created
# [INFO] redis init ok
```

**如果启动失败**：
1. 检查 MySQL 是否运行：`systemctl status mysql`
2. 检查数据库连接参数是否正确
3. 查看错误输出，通常会打印 MySQL 连接失败信息
4. 检查端口 8000 是否被占用：`netstat -tnlp | grep 8000`

### 第 6 步：配置 Redis（可选，用于分布式部署）

如果需要多实例部署和跨节点消息转发，启用 Redis：

```bash
# 启动 Redis
sudo systemctl start redis-server
sudo systemctl enable redis-server  # 开机自启

# 验证 Redis 运行
redis-cli ping  # 应该返回 PONG

# 修改服务端 [src/server/main.cpp](src/server/main.cpp)
# 取消注释或确保启用以下代码：
# redis.init("127.0.0.1", 6379);
```

### 第 7 步：文件夹权限和日志

```bash
# 如果启动失败提示权限问题
sudo chown -R $(whoami):$(whoami) /path/to/MyChatServer/bin

# 查看运行日志
# 日志通常输出到标准输出，如需保存到文件：
./bin/ChatServer > server.log 2>&1 &
tail -f server.log  # 实时查看日志

# 后台运行
nohup ./bin/ChatServer > server.log 2>&1 &
ps aux | grep ChatServer  # 查看进程
```

---

## 多实例部署（使用 Nginx 负载均衡）

### 完整部署架构

```
Client 1 ──┐
Client 2 ──┤
...  ──┼─→ [Nginx stream module]
Client N ──┤
           └─→ Server 1 (port 8000) ──┐
           └─→ Server 2 (port 8001) ──┼─→ [Redis Pub/Sub] ──→ MySQL
           └─→ Server 3 (port 8002) ──┘
```

### Nginx 配置示例

创建 `/etc/nginx/conf.d/chatserver.conf`：

```nginx
upstream chat_backend {
    # 服务器列表
    server 127.0.0.1:8000;
    server 127.0.0.1:8001;
    server 127.0.0.1:8002;
}

server {
    listen 9000;  # Nginx 监听端口
    proxy_pass chat_backend;
    proxy_protocol on;  # 网络协议支持
}
```

### 启动多实例

```bash
cd /path/to/MyChatServer

# 启动实例 1（端口 8000）
./bin/ChatServer &

# 启动实例 2（端口 8001）
./bin/ChatServer --port 8001 &

# 启动实例 3（端口 8002）
./bin/ChatServer --port 8002 &

# 查看所有实例
ps aux | grep ChatServer

# 关闭所有实例
pkill -f ChatServer

# 查看监听端口
netstat -tnlp | grep ChatServer
ss -tnlp | grep 800  # 应该看到 8000, 8001, 8002
```

**客户端连接配置**：

修改 [src/client/ui/LoginDialog.cpp](src/client/ui/LoginDialog.cpp#L73)：

```cpp
// 改为 Nginx 负载均衡地址
client.connectToServer("127.0.0.1", 9000);  // 连接 Nginx
```

---

## Qt 客户端编译与运行

### Windows 编译（使用 Qt Creator）

**前置条件**：
- 已安装 Qt5.15+ 或 Qt6.x
- 已安装 Visual Studio 2017+ 或 MinGW（C++ 编译器）
- 已安装 CMake 3.10+

**详细步骤**：

1. **启动 Qt Creator**

2. **打开项目**
   - 文件 → 打开文件或项目（Open File or Project）
   - 选择 `MyChatServer/CMakeLists.txt`

3. **配置项目**
   - 出现 "Configure Project" 对话框
   - 选择你的 Qt Kit（如 Qt 6.5.0 MSVC2019 64位）
   - 取消勾选 `BUILD_SERVER`（因为 Windows 不支持 Muduo）
   - 勾选 `BUILD_CLIENT`
   - 点击 "Configure Project"

4. **编译**
   - 使用快捷键 `Ctrl+B` 或点击左下角绿色三角形
   - 等待编译完成，输出日志显示 "Build successful"

5. **运行**
   - 使用快捷键 `Ctrl+R` 或点击绿色运行按钮
   - 应该弹出登录窗口

### Windows 命令行编译

```powershell
# 设置 Qt 和 CMake 环境变量
$env:Path += ";C:\Qt\6.5.0\msvc2019_64\bin;C:\Qt\Tools\CMake_64\bin"

# 进入项目目录
cd C:\path\to\MyChatServer

# 创建构建目录
mkdir build_client
cd build_client

# 生成 Visual Studio 项目
cmake -G "Visual Studio 16 2019" `
       -DBUILD_SERVER=OFF `
       -DBUILD_CLIENT=ON `
       -DCMAKE_PREFIX_PATH="C:\Qt\6.5.0\msvc2019_64" `
       ..

# 编译（Release 版本）
cmake --build . --config Release

# 运行
Release\ChatClient.exe
```

**参数说明**：
- `-G "Visual Studio 16 2019"` —— 生成器（可改为 MinGW Makefiles）
- `-DCMAKE_PREFIX_PATH` —— Qt 安装目录路径

**常见问题**：

| 错误 | 原因 | 解决方案 |
|---|---|---|
| `Could not find Qt` | CMake 找不到 Qt | 设置 `-DCMAKE_PREFIX_PATH` 指定 Qt 路径 |
| `cl.exe not found` | MSVC 编译器未找到 | 打开 "Developer Command Prompt for VS" |
| `undefined reference to 'mysql_init'` | Windows 通常不需要 MySQL | 这个错误不应该出现（客户端无需 MySQL） |

### Linux 编译

**前置条件**：
```bash
# 安装 Qt
sudo apt install -y qtbase5-dev qtbase5-dev-tools
# 或 Qt6
sudo apt install -y qt6-base-dev qt6-base-dev-tools

# 验证
qmake --version
```

**编译步骤**：

```bash
cd /path/to/MyChatServer

# 方式 A：使用自动化脚本
chmod +x autobuild.sh
BUILD_SERVER=OFF BUILD_CLIENT=ON ./autobuild.sh

# 方式 B：手动编译
mkdir build_client && cd build_client
cmake -DBUILD_SERVER=OFF -DBUILD_CLIENT=ON \
       -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# 运行客户端
./src/client/ChatClient
```

### macOS 编译

```bash
# 安装 Qt
brew install qt

# 编译
cd /path/to/MyChatServer
mkdir build_client && cd build_client

# 指定 Qt 路径
cmake -DBUILD_SERVER=OFF -DBUILD_CLIENT=ON \
       -DCMAKE_PREFIX_PATH=$(brew --prefix qt) ..

make -j$(sysctl -n hw.ncpu)

# 运行
./src/client/ChatClient.app/Contents/MacOS/ChatClient
```

### Docker 容器化编译（推荐，跨平台）

```dockerfile
# Dockerfile for ChatClient build
FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y \
    build-essential cmake qtbase5-dev qtbase5-dev-tools git

WORKDIR /app
COPY . .

# 编译
RUN mkdir build_client && cd build_client && \
    cmake -DBUILD_SERVER=OFF -DBUILD_CLIENT=ON .. && \
    make -j$(nproc)

ENTRYPOINT ["./build_client/src/client/ChatClient"]
```

**使用 Docker 构建**：
```bash
docker build -t mychat-client .
docker run -e DISPLAY=$(echo $DISPLAY) --net=host mychat-client
```

---

## 数据库配置详解

---

## 常见问题与解决方案

### 编译相关

#### ❌ 服务端编译：`cannot find -lmuduo_net`

**原因**：Muduo 库未正确安装或链接路径配置错误

**解决方案**：

```bash
# 方案 1：刷新系统库缓存（最常见原因）
sudo ldconfig
sudo ldconfig -p | grep muduo  # 验证

# 方案 2：检查 Muduo 安装位置
pkg-config --libs muduo_net muduo_base  # 应该输出库路径
ls -l /usr/local/lib | grep muduo       # 应该看到 libmuduo*

# 方案 3：重新编译安装 Muduo
cd /tmp/muduo/build/release-cpp11
sudo make install
sudo ldconfig

# 方案 4：手动指定 Muduo 路径
# 编辑 CMakeLists.txt，添加：
# set(MUDUO_INCLUDE_DIR "/usr/local/include")
# set(MUDUO_LIBRARY_DIR "/usr/local/lib")
```

#### ❌ 客户端编译：`Could not find Qt`

**原因**：CMake 找不到 Qt 安装路径

**解决方案**：

```bash
# Linux
# 方案 1：安装 Qt 开发包
sudo apt install -y qtbase5-dev qtbase5-dev-tools

# 方案 2：检查 qmake 位置
which qmake
# 如果找到，设置环境变量
export CMAKE_PREFIX_PATH=$(dirname $(dirname $(which qmake)))

# Windows
# 方案：在 CMake 命令中指定 Qt 路径
cmake -DCMAKE_PREFIX_PATH="C:\Qt\6.5.0\msvc2019_64" ..

# macOS
export CMAKE_PREFIX_PATH=$(brew --prefix qt)  # Homebrew 安装
# 或使用绝对路径：-DCMAKE_PREFIX_PATH="/path/to/Qt"
```

#### ❌ `CMakeLists.txt: project() not found`

**原因**：CMake 版本过旧或 CMakeLists.txt 位置错误

**解决方案**：
```bash
# 升级 CMake
sudo apt install cmake  # 确保版本 >= 3.10
cmake --version

# 确认在正确目录
ls MyChatServer/CMakeLists.txt  # 应该存在
```

### 数据库相关

#### ❌ 登录失败：`User not found in database`

**原因**：用户表中不存在该用户

**解决方案**：

```sql
-- 登录 MySQL
mysql -u root -p chat

-- 查看现有用户
SELECT id, name, password FROM User;

-- 如果表为空，执行初始化脚本
SOURCE /path/to/MyChatServer/scripts/chat.sql;

-- 手动添加测试用户
INSERT INTO User(name, password, state) VALUES('testuser', '123456', 'offline');

-- 验证
SELECT * FROM User WHERE name='testuser';
```

#### ❌ `Access denied MySQL user`

**原因**：数据库用户名或密码错误

**解决方案**：

```bash
# 验证 MySQL 连接
mysql -u root -p  # 输入当前密码

# 如果不知道密码，重置 root 用户
sudo mysql  # 无需密码进入（sudo 权限下）

# 在 MySQL 终端中
ALTER USER 'root'@'localhost' IDENTIFIED WITH mysql_native_password BY 'newpassword';
FLUSH PRIVILEGES;
EXIT;

# 更新服务端代码中的密码（db.cpp）
```

#### ❌ `Can't connect to MySQL server on '127.0.0.1'`

**原因**：MySQL 服务未运行或端口配置错误

**解决方案**：
```bash
# 启动 MySQL
sudo systemctl start mysql
sudo systemctl status mysql  # 验证

# 如果未安装 MySQL，先安装
sudo apt install mysql-server

# 检查监听端口（应该是 3306）
sudo netstat -tlnp | grep mysql
```

### 网络连接相关

#### ❌ 客户端连接失败：`Connection refused`

**原因**：
1. 服务端未启动
2. 服务端监听地址或端口不对
3. 防火墙阻止

**排查步骤**：

```bash
# 1. 检查服务端是否运行
ps aux | grep ChatServer
# 或
netstat -tnlp | grep 8000  # 检查端口监听

# 2. 检查防火墙
sudo ufw status
# 如需开放端口
sudo ufw allow 8000/tcp
# 或（CentOS）
sudo firewall-cmd --add-port=8000/tcp --permanent
sudo firewall-cmd --reload

# 3. 修改客户端连接地址
# 编辑 src/client/ui/LoginDialog.cpp 或配置文件
client.connectToServer("127.0.0.1", 8000);  # 本机
# 或改为服务器 IP
client.connectToServer("192.168.1.100", 8000);  # 远程
```

#### ❌ `Port 8000 already in use`

**原因**：端口被其他进程占用

**解决方案**：
```bash
# 查看占用 8000 端口的进程
sudo lsof -i :8000
# 或
sudo netstat -tnlp | grep 8000

# 杀死占用进程
sudo kill -9 <PID>

# 或改用其他端口（编辑 main.cpp）
```

### Redis 相关

#### ❌ Redis 连接失败

**原因**：Redis 服务未运行

**解决方案**：
```bash
# 启动 Redis
sudo systemctl start redis-server
sudo systemctl status redis-server

# 验证 Redis 运行
redis-cli ping  # 应该返回 PONG
redis-cli  # 进入交互式客户端

# 如果未安装
sudo apt install redis-server
```

### 日志和调试

#### 🔍 查看详细日志

```bash
# 运行服务端时输出详细日志
./bin/ChatServer --loglevel=DEBUG

# 保存日志到文件
./bin/ChatServer > server.log 2>&1 &

# 实时查看日志
tail -f server.log

# 后台运行（nohup）
nohup ./bin/ChatServer > server.log 2>&1 &
# 查看后台进程
jobs -l

# Systemd 日志（如果用 systemd 启动）
sudo journalctl -u chatserver -f
```

#### 🔍 GDB 调试服务端

```bash
# 编译时启用调试符号
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# 使用 GDB 调试
gdb ./bin/ChatServer

# GDB 命令
(gdb) b chatservice.cpp:123     # 在行号设置断点
(gdb) run                        # 运行程序
(gdb) c                          # 继续执行
(gdb) bt                         # 打印堆栈跟踪
(gdb) p variable_name            # 打印变量值
(gdb) q                          # 退出
```

---

## 性能测试与优化

### 压力测试

使用 Apache Bench 或自定义测试工具：

```bash
# 使用 wrk（需要单独安装）
git clone https://github.com/wg/wrk.git
cd wrk
make
./wrk -t4 -c100 -d30s http://127.0.0.1:8000

# 使用 ab
ab -n 1000 -c 100 http://127.0.0.1:8000/
```

### 性能优化建议

| 优化项 | 方法 | 影响 |
|---|---|---|
| **数据库连接池** | 增大连接池大小（db.cpp） | 减少连接开销 |
| **Muduo 线程数** | 调整 EventLoopThreadPool 大小 | 提高并发处理能力 |
| **日志级别** | 生产环境改为 ERROR 级别 | 减少 I/O 和锁竞争 |
| **消息批处理** | Flush 多条消息一起发送 | 减少系统调用 |
| **内存优化** | 删除调试代码，编译优化选项 | 减少内存占用 |

---

## 容器化部署（Docker）

### Docker 镜像构建

创建 `Dockerfile`：

```dockerfile
# 构建阶段
FROM ubuntu:20.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y \
    build-essential cmake git \
    default-libmysqlclient-dev libhiredis-dev \
    && rm -rf /var/lib/apt/lists/*

# 编译 Muduo
RUN cd /tmp && git clone https://github.com/chenshuo/muduo.git && \
    cd muduo && ./build.sh && \
    cd build/release-cpp11 && make install && ldconfig

# 复制项目并编译
COPY . /app
WORKDIR /app/build
RUN cmake -DBUILD_SERVER=ON -DBUILD_CLIENT=OFF .. && make -j$(nproc)

# 运行阶段（最小镜像）
FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y \
    libmysqlclient21 libhiredis0.14 libstdc++6 \
    mysql-client redis-tools \
    && rm -rf /var/lib/apt/lists/*

# 从构建阶段复制库
COPY --from=builder /usr/local/lib/libmuduo* /usr/local/lib/
RUN ldconfig

# 从构建阶段复制二进制
COPY --from=builder /app/bin/ChatServer /app/ChatServer

EXPOSE 8000
ENTRYPOINT ["/app/ChatServer"]
```

### 使用 Docker Compose

创建 `docker-compose.yml`：

```yaml
version: '3.8'

services:
  mysql:
    image: mysql:8.0
    environment:
      MYSQL_ROOT_PASSWORD: root123
      MYSQL_DATABASE: chat
    ports:
      - "3306:3306"
    volumes:
      - ./scripts/chat.sql:/docker-entrypoint-initdb.d/chat.sql

  redis:
    image: redis:7-alpine
    ports:
      - "6379:6379"

  chatserver:
    build: .
    ports:
      - "8000:8000"
    depends_on:
      - mysql
      - redis
    environment:
      DB_HOST: mysql
      REDIS_HOST: redis
```

### 部署

```bash
# 构建镜像
docker build -t mychat-server:latest .

# 运行容器
docker run -p 8000:8000 \
           --link mysql:mysql_host \
           mychat-server:latest

# 或使用 Docker Compose（推荐）
docker-compose up -d
docker-compose logs -f chatserver  # 查看日志
docker-compose ps                  # 查看状态
```
