# MyChat 架构设计文档

本文档从整体到细节介绍 MyChat 即时通讯系统的架构设计，包括分层结构、核心模块、数据流和扩展方案。

---

## 整体架构

MyChat 采用经典的**三层架构**：网络层、业务层、数据层，服务端与客户端通过 JSON over TCP 协议通信。

```
┌─────────────────────────────────────────────────────────────┐
│                  Qt ChatClient（跨平台）                     │
│   LoginDialog  ──▶  MainWindow  ──▶  NetworkClient          │
│                     (Qt Widgets)      (QTcpSocket)           │
└──────────────────────────┬──────────────────────────────────┘
                           │  JSON over TCP
                           ▼
┌─────────────────────────────────────────────────────────────┐
│               Nginx stream（可选负载均衡）                    │
└──────────────┬───────────────────────────┬──────────────────┘
               │                           │
               ▼                           ▼
┌──────────────────────┐     ┌──────────────────────┐
│    ChatServer-1      │     │    ChatServer-2       │
│  ┌────────────────┐  │     │  ┌────────────────┐  │
│  │  ChatServer    │  │     │  │  ChatServer    │  │
│  │  (网络层)      │  │     │  │  (网络层)      │  │
│  └───────┬────────┘  │     │  └───────┬────────┘  │
│          │           │     │          │           │
│  ┌───────▼────────┐  │     │  ┌───────▼────────┐  │
│  │  ChatService   │  │     │  │  ChatService   │  │
│  │  (业务层)      │  │     │  │  (业务层)      │  │
│  └──┬──────────┬──┘  │     │  └──┬──────────┬──┘  │
│     │          │     │     │     │          │     │
│     ▼          ▼     │     │     ▼          ▼     │
│  ┌──────┐  ┌──────┐  │     │  ┌──────┐  ┌──────┐  │
│  │MySQL │  │Redis │  │     │  │MySQL │  │Redis │  │
│  │(数据)│  │(P/S) │  │     │  │(数据)│  │(P/S) │  │
│  └──────┘  └──┬───┘  │     │  └──────┘  └──┬───┘  │
└──────────────┼────────┘     └──────────────┼────────┘
               └──────────────────────────────┘
                    Redis Pub/Sub 跨节点同步
```

---

## 核心模块详解

### 1. 网络层（ChatServer）

**源文件**：`src/server/chatserver.cpp`、`include/server/chatserver.hpp`

**职责**：
- 封装 Muduo 的 `TcpServer`，管理 TCP 连接生命周期
- 监听端口，接受客户端连接
- 接收原始数据，解析 JSON，按 `msgid` 分发到业务层

**核心设计**：
- **Reactor 模型**：Muduo 使用 `one loop per thread + thread pool` 模式，I/O 事件由主 EventLoop 处理，业务逻辑分发到工作线程池
- **回调注册**：`onConnection()` 处理连接建立/断开，`onMessage()` 处理消息收发

```cpp
// 关键回调
void ChatServer::onConnection(const TcpConnectionPtr &conn) {
    if (!conn->connected()) {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buf, Timestamp time) {
    // 解析 JSON，获取 msgid，分发到对应 handler
    json js = json::parse(buf->retrieveAllAsString());
    auto handler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    handler(conn, js, time);
}
```

---

### 2. 业务层（ChatService）

**源文件**：`src/server/chatservice.cpp`、`include/server/chatservice.hpp`

**职责**：
- 维护消息 ID 到处理函数的映射表（`_msgHandlerMap`）
- 维护在线用户的连接映射表（`_userConnMap`）
- 实现所有业务逻辑（登录、注册、单聊、群聊等）
- 处理分布式场景下的 Redis 消息订阅与转发

**核心数据结构**：

```cpp
class ChatService {
    // 消息分发表：msgid → 处理函数
    unordered_map<int, MsgHandler> _msgHandlerMap;

    // 在线用户连接表：userid → TcpConnectionPtr
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 互斥锁，保护 _userConnMap 的线程安全
    mutex _connMutex;

    // 各数据模型
    UserModel         _userModel;
    FriendModel       _friendModel;
    GroupModel        _groupModel;
    OfflineMsgModel   _offlineMsgModel;

    // Redis 客户端
    Redis _redis;
};
```

**消息处理函数注册**：

```cpp
ChatService::ChatService() {
    _msgHandlerMap[LOGIN_MSG]         = bind(&ChatService::login,         this, _1, _2, _3);
    _msgHandlerMap[REG_MSG]           = bind(&ChatService::reg,           this, _1, _2, _3);
    _msgHandlerMap[ONE_CHAT_MSG]      = bind(&ChatService::oneChat,       this, _1, _2, _3);
    _msgHandlerMap[ADD_FRIEND_MSG]    = bind(&ChatService::addFriend,     this, _1, _2, _3);
    _msgHandlerMap[CREATE_GROUP_MSG]  = bind(&ChatService::createGroup,   this, _1, _2, _3);
    _msgHandlerMap[GROUP_CHAT_MSG]    = bind(&ChatService::groupChat,     this, _1, _2, _3);
    // ... 其余消息类型
}
```

**单聊消息流程**：

```
客户端 A ──ONE_CHAT_MSG──▶ ChatServer
                               │
                        ChatService::oneChat()
                               │
                    ┌──────────▼───────────┐
                    │ 查询 _userConnMap     │
                    └──────────┬───────────┘
                               │
               ┌───────────────▼────────────────┐
               │  用户 B 在线？                  │
               ├── 是：直接通过连接发送消息      │
               └── 否：存入离线消息表（MySQL）   │
                              │
               （分布式）B 在其他节点？
               └── 是：Redis publish(B_userid, msg)
                         → B 所在节点的 subscribe 回调转发
```

---

### 3. 数据层（Model + DB）

#### 3.1 MySQL 封装（db.h）

**源文件**：`src/server/db/db.cpp`、`include/server/db/db.h`

提供对 MySQL C API 的简单封装：

```cpp
class MySQL {
    bool connect();                   // 建立连接
    bool update(string sql);          // 执行增删改
    MYSQL_RES *query(string sql);     // 执行查询，返回结果集
};
```

#### 3.2 数据模型（Model 层）

每个 Model 类对应一张数据库表，实现 DAO（Data Access Object）模式：

| 模型类 | 对应表 | 主要操作 |
|---|---|---|
| `UserModel` | `user` | `insert`、`query`、`updateState`、`resetState` |
| `FriendModel` | `friend` | `insert`、`remove`、`query` |
| `GroupModel` | `allgroup`、`groupuser` | `createGroup`、`addGroup`、`quitGroup`、`queryGroups` |
| `OfflineMsgModel` | `offlinemessage` | `insert`、`remove`、`query` |

#### 3.3 数据库表结构

**用户表（user）**

```sql
CREATE TABLE `user` (
    `id`       INT AUTO_INCREMENT PRIMARY KEY,
    `name`     VARCHAR(50) NOT NULL UNIQUE,
    `password` VARCHAR(50) NOT NULL,
    `state`    ENUM('online', 'offline') DEFAULT 'offline'
);
```

**好友表（friend）**

```sql
CREATE TABLE `friend` (
    `userid`   INT NOT NULL,
    `friendid` INT NOT NULL,
    PRIMARY KEY (`userid`, `friendid`)
);
```

**群组表（allgroup）**

```sql
CREATE TABLE `allgroup` (
    `id`        INT AUTO_INCREMENT PRIMARY KEY,
    `groupname` VARCHAR(50) NOT NULL,
    `groupdesc` VARCHAR(200) DEFAULT ''
);
```

**群成员表（groupuser）**

```sql
CREATE TABLE `groupuser` (
    `groupid` INT NOT NULL,
    `userid`  INT NOT NULL,
    `grouprole` ENUM('creator', 'normal') DEFAULT 'normal',
    PRIMARY KEY (`groupid`, `userid`)
);
```

**离线消息表（offlinemessage）**

```sql
CREATE TABLE `offlinemessage` (
    `userid`  INT NOT NULL,
    `message` VARCHAR(500) NOT NULL
);
```

---

### 4. 分布式扩展（Redis 模块）

**源文件**：`src/server/redis/redis.cpp`、`include/server/redis/redis.hpp`

**设计目标**：支持多个 ChatServer 实例协同工作，实现跨节点消息路由。

**工作原理**：

```
用户 B（连接到 Server-2）
                ↑
Server-2 订阅频道 B_userid
                ↑ 收到消息并转发
      ┌─────────────────┐
      │  Redis Pub/Sub  │
      └─────────────────┘
                ↑ publish(B_userid, msg)
Server-1 收到 ONE_CHAT_MSG，发现 B 不在本节点
用户 A（连接到 Server-1）
```

**关键 API**：

```cpp
class Redis {
    bool connect();                                    // 连接 Redis
    bool publish(int channel, string message);         // 发布消息到频道
    bool subscribe(int channel);                       // 订阅频道
    bool unsubscribe(int channel);                     // 取消订阅
    // 消息接收回调（运行在独立线程中）
    function<void(int, string)> _notify_handler;
};
```

**用户登录时**：`subscribe(userid)` — 订阅以用户 ID 为频道名的 Redis 频道  
**用户下线时**：`unsubscribe(userid)` — 取消订阅  
**跨节点转发时**：`publish(target_userid, json_msg)` — 发布消息到目标用户频道

---

### 5. Qt 客户端

**源文件**：`src/client/`

**架构**：

```
LoginDialog
    │ 登录/注册成功
    ▼
MainWindow
    │ 用户操作（发消息、加好友等）
    ▼
NetworkClient（Singleton）
    │ QTcpSocket
    ▼
ChatServer
```

**NetworkClient**：

- 单例封装 `QTcpSocket`，隐藏底层网络细节
- 提供信号（Qt signals）用于异步通知 UI：
  - `loginResult(bool success, ...)` — 登录结果
  - `registerResult(bool success, ...)` — 注册结果
  - `chatMessageReceived(...)` — 收到单聊消息
  - `groupMessageReceived(...)` — 收到群聊消息
  - `friendStateChanged(...)` — 好友状态变更

---

## 线程模型

```
主线程（EventLoop）
│
├── accept()：接受新连接
├── read()：接收客户端数据
├── 解析 JSON，获取 msgid
└── 将 (conn, js, time) 投递到线程池
        │
        ▼
工作线程池（ThreadPool）
├── 执行 ChatService::login()
├── 执行 ChatService::oneChat()
├── 执行 ChatService::groupChat()
└── ... 其他业务处理

Redis 订阅线程（独立）
└── 阻塞监听 Redis subscribe 回调
    └── 通知主线程转发消息
```

**线程安全保障**：
- `_userConnMap` 读写均持有 `_connMutex` 互斥锁
- Redis 订阅回调通过 Muduo 的 `runInLoop` 安全地回到 EventLoop 线程执行

---

## 关键设计模式

| 模式 | 应用场景 |
|---|---|
| **单例模式** | `ChatService`（全局唯一业务处理实例）、`NetworkClient`（客户端网络单例） |
| **Reactor 模式** | Muduo 事件驱动网络框架，I/O 多路复用 |
| **观察者模式** | Qt signals/slots，UI 层订阅网络事件 |
| **DAO 模式** | `UserModel`、`FriendModel` 等封装数据库操作 |
| **策略/分发模式** | `_msgHandlerMap` 映射 msgid → 处理函数，避免大量 if/switch |

---

## 扩展方向

| 方向 | 方案 |
|---|---|
| 水平扩展 | 增加 ChatServer 实例 + Nginx 负载均衡 + Redis Pub/Sub |
| 协议升级 | Protobuf 替代 JSON，减少序列化开销 |
| 连接池 | 为 MySQL 引入连接池，减少频繁建连开销 |
| 消息可靠性 | 引入消息队列（Kafka/RabbitMQ）保障消息不丢失 |
| 安全传输 | TLS/SSL 加密 TCP 连接 |
| 监控 | Prometheus + Grafana 采集连接数、消息量等指标 |
| 容器化 | Docker Compose 一键部署 Server + MySQL + Redis |
