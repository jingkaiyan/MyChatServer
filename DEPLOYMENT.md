# MyChatServer 部署与使用指南

## 📋 目录
1. [项目架构说明](#项目架构说明)
2. [环境要求](#环境要求)
3. [Linux 服务端编译与部署](#linux-服务端编译与部署)
4. [Qt 客户端编译与运行](#qt-客户端编译与运行)
5. [数据库配置](#数据库配置)
6. [Redis 配置(可选)](#redis-配置可选)
7. [功能测试指南](#功能测试指南)
8. [常见问题解决](#常见问题解决)

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

### 服务端环境(Linux)
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y g++ cmake make
sudo apt install -y libmysqlclient-dev
sudo apt install -y libhiredis-dev  # 可选,用于Redis
sudo apt install -y libmuduo-dev    # Muduo网络库

# CentOS/RHEL
sudo yum install -y gcc-c++ cmake make
sudo yum install -y mysql-devel
sudo yum install -y hiredis-devel
# Muduo需要手动编译安装或使用包管理器
```

### 客户端环境(跨平台)
- **Qt5 或 Qt6**: 包含 Widgets 和 Network 模块
- **C++11 编译器**: g++/clang/MSVC 均可
- **CMake 3.10+**

**Windows 安装**:
1. 下载 [Qt 在线安装器](https://www.qt.io/download-qt-installer)
2. 选择 Qt5.15 或 Qt6.x 版本,勾选 `Qt Network` 和 `Qt Widgets`

**Linux 安装**:
```bash
# Ubuntu
sudo apt install -y qtbase5-dev qtbase5-dev-tools
# 或
sudo apt install -y qt6-base-dev qt6-base-dev-tools
```

---

## Linux 服务端编译与部署

### 1. 安装Muduo库(如果系统没有)
```bash
# 克隆Muduo源码
git clone https://github.com/chenshuo/muduo.git
cd muduo

# 编译安装
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
```

### 2. 配置MySQL数据库
```bash
# 登录MySQL
mysql -u root -p

# 创建数据库和表
source /path/to/MyChatServer/scripts/chat.sql
```

### 3. 修改数据库连接信息
编辑 [src/server/db/db.cpp](src/server/db/db.cpp):
```cpp
MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}

bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(_conn, 
        "127.0.0.1",    // 数据库地址
        "root",         // 用户名
        "your_password", // 密码
        "chat",         // 数据库名
        3306,           // 端口
        nullptr, 0);
    // ...
}
```

### 4. 编译服务端
```bash
cd /path/to/MyChatServer
chmod +x autobuild.sh

# 编译服务端(默认)
./autobuild.sh

# 或手动构建
mkdir build && cd build
cmake -DBUILD_SERVER=ON -DBUILD_CLIENT=OFF ..
make -j$(nproc)
```

### 5. 运行服务端
```bash
# 在项目根目录执行
./bin/ChatServer

# 输出示例:
# [INFO] ChatService singleton created
# [INFO] ChatServer start at 8000
```

### 6. 配置Redis(可选,用于分布式部署)
如果需要多实例部署,可启用Redis:
```bash
# 安装Redis
sudo apt install redis-server

# 启动Redis
sudo systemctl start redis
sudo systemctl enable redis

# 修改服务端main.cpp中的Redis地址
# redis.init("127.0.0.1", 6379);
```

---

## Qt 客户端编译与运行

### Windows 编译(Qt Creator)
1. **启动 Qt Creator**
2. **打开项目**: 文件 → 打开文件或项目 → 选择 `MyChatServer/CMakeLists.txt`
3. **Configure Project**:
   - 取消勾选 `BUILD_SERVER` 选项
   - 勾选 `BUILD_CLIENT` 选项
   - Kit 选择对应的 Qt 版本
4. **编译**: Ctrl+B 或点击左下角绿色三角形
5. **运行**: Ctrl+R

### Windows 命令行编译
```powershell
# 确保Qt bin目录在PATH中
$env:Path += ";C:\Qt\6.5.0\msvc2019_64\bin;C:\Qt\Tools\CMake_64\bin"

cd C:\path\to\MyChatServer
mkdir build_client
cd build_client

cmake -G "Visual Studio 16 2019" -DBUILD_SERVER=OFF -DBUILD_CLIENT=ON ..
cmake --build . --config Release

# 运行
.\Release\ChatClient.exe
```

### Linux 编译
```bash
cd /path/to/MyChatServer

# 构建客户端
mkdir build_client && cd build_client
cmake -DBUILD_SERVER=OFF -DBUILD_CLIENT=ON ..
make -j$(nproc)

# 运行
./src/client/ChatClient
```

---

## 数据库配置

### chat.sql 脚本内容
```sql
CREATE DATABASE IF NOT EXISTS chat CHARACTER SET utf8mb4;
USE chat;

-- 用户表
CREATE TABLE IF NOT EXISTS User (
  id INT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(50) NOT NULL UNIQUE,
  password VARCHAR(50) NOT NULL,
  state ENUM('online', 'offline') DEFAULT 'offline'
);

-- 好友关系表
CREATE TABLE IF NOT EXISTS Friend (
  userid INT NOT NULL,
  friendid INT NOT NULL,
  PRIMARY KEY(userid, friendid)
);

-- 离线消息表
CREATE TABLE IF NOT EXISTS OfflineMessage (
  userid INT NOT NULL,
  message TEXT NOT NULL
);

-- 群组表
CREATE TABLE IF NOT EXISTS AllGroup (
  id INT PRIMARY KEY AUTO_INCREMENT,
  groupname VARCHAR(50) NOT NULL UNIQUE,
  groupdesc VARCHAR(200) DEFAULT ''
);

-- 群成员表
CREATE TABLE IF NOT EXISTS GroupUser (
  groupid INT NOT NULL,
  userid INT NOT NULL,
  grouprole ENUM('creator', 'normal') DEFAULT 'normal',
  PRIMARY KEY(groupid, userid)
);

-- 插入测试用户
INSERT INTO User(name, password, state) VALUES
('zhao', '123456', 'offline'),
('qian', '123456', 'offline'),
('sun', '123456', 'offline'),
('li', '123456', 'offline');

-- 插入好友关系
INSERT INTO Friend VALUES(1, 2), (1, 3), (2, 3);

-- 创建测试群组
INSERT INTO AllGroup(groupname, groupdesc) VALUES
('C++学习群', '讨论C++编程技术'),
('项目组', '项目开发协作群');

INSERT INTO GroupUser VALUES(1, 1, 'creator'), (1, 2, 'normal'), (2, 1, 'normal');
```

### 创建测试账号
```bash
# 方法1: 使用客户端注册功能(未实现UI)
# 方法2: 手动插入数据库
mysql -u root -p chat

INSERT INTO User(name, password) VALUES('testuser', '123456');
SELECT id, name FROM User WHERE name='testuser';
# 记住返回的 id,用于登录
```

---

## 功能测试指南

### 1. 启动服务端
```bash
# Linux
cd /path/to/MyChatServer
./bin/ChatServer

# 确认输出包含:
# [INFO] ChatServer start at 8000
```

### 2. 启动客户端
```bash
# Windows
ChatClient.exe

# Linux
./ChatClient
```

### 3. 登录测试
1. 在登录界面输入:
   - 账号: `1` (zhao的ID)
   - 密码: `123456`
2. 点击 **登录**
3. 观察是否成功进入主界面并显示好友列表

### 4. 聊天测试
#### 单聊测试:
1. 在主界面左侧 " 好友" 列表中选择一个在线好友
2. 在底部输入框输入消息
3. 点击 **发送**
4. 另一个客户端(登录该好友账号)应收到消息

#### 群聊测试:
1. 切换到"群组"标签
2. 选择一个群组
3. 发送消息
4. 同群组其他成员应收到消息

### 5. 离线消息测试
1. 用户A登录后发送消息给离线用户B
2. 用户B上线后,应自动接收到离线消息

---

## 常见问题解决

### Q1: 服务端编译失败 - "找不到muduo库"
**解决方案**:
```bash
# 检查muduo是否安装
pkg-config --libs muduo_net muduo_base

# 如果失败,手动编译安装muduo(见上文)
# 或指定muduo路径:
cmake -DMUDUO_INCLUDE_DIR=/path/to/muduo/include \
      -DMUDUO_LIBRARY_DIR=/path/to/muduo/lib ..
```

### Q2: 客户端连接失败 - "连接服务器失败"
**排查步骤**:
1. 确认服务端正在运行: `netstat -tnlp | grep 8000`
2. 检查防火墙:
   ```bash
   # Ubuntu
   sudo ufw allow 8000/tcp
   # CentOS
   sudo firewall-cmd --add-port=8000/tcp --permanent
   ```
3. 修改客户端连接地址(默认127.0.0.1:8000):
   编辑 [src/client/ui/LoginDialog.cpp](src/client/ui/LoginDialog.cpp#L73):
   ```cpp
   client.connectToServer("192.168.1.100", 8000); // 改为服务器IP
   ```

### Q3: Windows无法编译 - "CMAKE_CXX_COMPILER not set"
**解决方案**:
```powershell
# 方法1: 使用Qt Creator(推荐)
# 方法2: 指定生成器
cmake -G "Visual Studio 16 2019" ..
# 或
cmake -G "MinGW Makefiles" ..
```

### Q4: 客户端编译失败 - "找不到Qt"
**解决方案**:
```bash
# Linux
sudo apt install qtbase5-dev

# Windows
# 确保Qt bin目录在PATH中:
set path C:\Qt\6.5.0\msvc2019_64\bin;%PATH%

# 或指定Qt路径:
cmake -DCMAKE_PREFIX_PATH="C:/Qt/6.5.0/msvc2019_64" ..
```

### Q5: 登录失败 - "用户不存在"
**解决方案**:
```sql
-- 检查用户是否存在
mysql -u root -p chat
SELECT id, name, password FROM User;

-- 如果不存在,手动添加
INSERT INTO User(name, password) VALUES('testuser', '123456');
```

### Q6: MySQL连接失败 - "Access denied"
**解决方案**:
```bash
# 重置root密码
sudo mysql
ALTER USER 'root'@'localhost' IDENTIFIED WITH mysql_native_password BY 'newpassword';
FLUSH PRIVILEGES;
EXIT;

# 修改服务端代码src/server/db/db.cpp中的密码
```

---

## 高级配置

### 多实例部署(使用Redis)
```bash
# 启动Redis
redis-server

# 修改服务端main.cpp启用Redis
# redis.init("127.0.0.1", 6379);

# 启动多个服务端实例
./ChatServer --port 8000 &
./ChatServer --port 8001 &
./ChatServer --port 8002 &
```

### 性能调优
- **连接池**: 优化MySQL连接池大小(db.cpp)
- **线程数**: 调整Muduo线程池大小
- **日志级别**: 生产环境使用ERROR级别日志

### 代码调试
```bash
# 服务端调试
gdb ./ChatServer
(gdb) b chatservice.cpp:123
(gdb) run

# 客户端调试(Qt Creator)
# 设置断点 → F5调试运行
```

---

## 协议文档

### 消息类型(public.hpp)
```cpp
LOGIN_MSG = 1           // 登录
LOG_MSG_ACK = 2         // 登录响应
LOGINOUT_MSG = 3        // 注销
REG_MSG = 4             // 注册
REG_MSG_ACK = 5         // 注册响应
ONE_CHAT_MSG = 6        // 单聊
ADD_FRIEND_MSG = 7      // 添加好友
CREATE_GROUP_MSG = 8    // 创建群组
ADD_GROUP_MSG = 9       // 加入群组
GROUP_CHAT_MSG = 13     // 群聊
FRIEND_STATE_MSG = 14   // 好友状态变更
```

### 示例消息格式
**登录请求**:
```json
{
  "msgid": 1,
  "id": 123,
  "password": "123456"
}
```

**登录响应(成功)**:
```json
{
  "msgid": 2,
  "errno": 0,
  "id": 123,
  "name": "testuser",
  "friends": [
    {"id": 2, "name": "friend1", "state": "online"},
    {"id": 3, "name": "friend2", "state": "offline"}
  ],
  "groups": [
    {"id": 1, "groupname": "C++群", "users": [...]}
  ],
  "offlinemsg": [
    {"from": 2, "msg": "hello", "time": "2025-01-10 10:30:00"}
  ]
}
```

---

## 项目维护

### 版本管理
```bash
git tag v1.1.0
git push origin v1.1.0
```

### 持续集成(示例)
```yaml
# .github/workflows/build.yml
name: Build
on: [push]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: sudo apt install -y g++ cmake libmysqlclient-dev libmuduo-dev
      - name: Build server
        run: |
          mkdir build && cd build
          cmake -DBUILD_SERVER=ON ..
          make
```

---

## 联系方式
- 项目地址: https://github.com/jingkaiyan/MyChatServer
- 问题反馈: Issues
- 技术交流: [加入讨论群]

---

**最后更新**: 2025-01-10  
**文档版本**: 1.1.0
