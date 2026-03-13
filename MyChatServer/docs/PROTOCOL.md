# MyChat 通信协议文档

本文档描述客户端与服务端之间的通信协议——基于 **JSON over TCP** 的消息格式规范。

---

## 协议概述

- **传输层**：TCP 长连接
- **序列化格式**：JSON（UTF-8 编码）
- **消息标识**：每条消息通过 `msgid` 字段标识类型
- **协议定义**：见 `include/public.hpp`

---

## 消息类型枚举

| 枚举名 | ID | 方向 | 说明 |
|---|---|---|---|
| `LOGIN_MSG` | 1 | 客户端 → 服务端 | 登录请求 |
| `REG_MSG` | 2 | 客户端 → 服务端 | 注册请求 |
| `REG_MSG_ACK` | 3 | 服务端 → 客户端 | 注册响应 |
| `LOG_MSG_ACK` | 4 | 服务端 → 客户端 | 登录响应 |
| `LOGINOUT_MSG` | 5 | 客户端 → 服务端 | 登出请求 |
| `ONE_CHAT_MSG` | 6 | 双向 | 单聊消息 |
| `ADD_FRIEND_MSG` | 7 | 客户端 → 服务端 | 添加好友请求 |
| `ADD_FRIEND_MSG_ACK` | 8 | 服务端 → 客户端 | 添加好友响应 |
| `DELETE_FRIEND_MSG` | 9 | 客户端 → 服务端 | 删除好友请求 |
| `DELETE_FRIEND_MSG_ACK` | 10 | 服务端 → 客户端 | 删除好友响应 |
| `CREATE_GROUP_MSG` | 11 | 客户端 → 服务端 | 创建群组请求 |
| `CREATE_GROUP_MSG_ACK` | 12 | 服务端 → 客户端 | 创建群组响应 |
| `ADD_GROUP_MSG` | 13 | 客户端 → 服务端 | 加入群组请求 |
| `ADD_GROUP_MSG_ACK` | 14 | 服务端 → 客户端 | 加入群组响应 |
| `QUIT_GROUP_MSG` | 15 | 客户端 → 服务端 | 退出群组请求 |
| `QUIT_GROUP_MSG_ACK` | 16 | 服务端 → 客户端 | 退出群组响应 |
| `GROUP_CHAT_MSG` | 17 | 双向 | 群聊消息 |
| `FRIEND_STATE_MSG` | 18 | 服务端 → 客户端 | 好友在线状态变更推送 |

---

## 消息格式详解

### 1. 登录请求 `LOGIN_MSG (1)`

**方向**：客户端 → 服务端

```json
{
  "msgid": 1,
  "id": 123,
  "password": "123456"
}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `msgid` | int | 消息类型，固定为 `1` |
| `id` | int | 用户 ID |
| `password` | string | 用户密码 |

---

### 2. 注册请求 `REG_MSG (2)`

**方向**：客户端 → 服务端

```json
{
  "msgid": 2,
  "name": "john_doe",
  "password": "123456"
}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `msgid` | int | 消息类型，固定为 `2` |
| `name` | string | 用户名 |
| `password` | string | 用户密码 |

---

### 3. 注册响应 `REG_MSG_ACK (3)`

**方向**：服务端 → 客户端

```json
{
  "msgid": 3,
  "errno": 0,
  "id": 123
}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `msgid` | int | 消息类型，固定为 `3` |
| `errno` | int | 错误码，`0` 表示成功，非 `0` 表示失败 |
| `id` | int | 注册成功时返回的用户 ID |
| `errmsg` | string | （可选）失败时的错误描述 |

---

### 4. 登录响应 `LOG_MSG_ACK (4)`

**方向**：服务端 → 客户端

```json
{
  "msgid": 4,
  "errno": 0,
  "id": 123,
  "name": "john_doe",
  "friends": [
    {"id": 2, "name": "alice", "state": "online"},
    {"id": 3, "name": "bob", "state": "offline"}
  ],
  "groups": [
    {
      "id": 1,
      "groupname": "C++ 学习群",
      "groupdesc": "C++ 技术交流",
      "users": [
        {"id": 2, "name": "alice", "state": "online", "role": "creator"},
        {"id": 3, "name": "bob", "state": "offline", "role": "normal"}
      ]
    }
  ],
  "offlinemsg": [
    {"from": 2, "from_name": "alice", "msg": "hello", "time": "2025-01-10 14:30:00"},
    {"msgid": 17, "groupid": 1, "from": 3, "from_name": "bob", "msg": "群消息", "time": "2025-01-10 14:31:00"}
  ]
}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `msgid` | int | 消息类型，固定为 `4` |
| `errno` | int | 错误码，`0` 表示成功 |
| `id` | int | 用户 ID |
| `name` | string | 用户名 |
| `friends` | array | 好友列表（含在线状态） |
| `groups` | array | 群组列表（含成员信息） |
| `offlinemsg` | array | 离线消息列表 |
| `errmsg` | string | （可选）失败时的错误描述 |

---

### 5. 登出请求 `LOGINOUT_MSG (5)`

**方向**：客户端 → 服务端

```json
{
  "msgid": 5,
  "id": 123
}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `msgid` | int | 消息类型，固定为 `5` |
| `id` | int | 用户 ID |

---

### 6. 单聊消息 `ONE_CHAT_MSG (6)`

**方向**：双向（客户端发送 / 服务端转发）

```json
{
  "msgid": 6,
  "id": 123,
  "name": "john_doe",
  "to": 456,
  "msg": "你好，这是一条消息",
  "time": "2025-01-10 14:35:20"
}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `msgid` | int | 消息类型，固定为 `6` |
| `id` | int | 发送方用户 ID |
| `name` | string | 发送方用户名 |
| `to` | int | 接收方用户 ID |
| `msg` | string | 消息内容 |
| `time` | string | 发送时间（`YYYY-MM-DD HH:MM:SS` 格式） |

**服务端行为**：
- 若接收方在线：直接转发给目标连接
- 若接收方离线：存储为离线消息，待对方登录时下发

---

### 7. 添加好友请求 `ADD_FRIEND_MSG (7)`

**方向**：客户端 → 服务端

```json
{
  "msgid": 7,
  "id": 123,
  "friendid": 456
}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `msgid` | int | 消息类型，固定为 `7` |
| `id` | int | 发起方用户 ID |
| `friendid` | int | 目标好友用户 ID |

---

### 8. 添加好友响应 `ADD_FRIEND_MSG_ACK (8)`

**方向**：服务端 → 客户端

```json
{
  "msgid": 8,
  "errno": 0
}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `msgid` | int | 消息类型，固定为 `8` |
| `errno` | int | 错误码，`0` 表示成功 |
| `errmsg` | string | （可选）失败时的错误描述 |

---

### 9. 删除好友请求 `DELETE_FRIEND_MSG (9)`

**方向**：客户端 → 服务端

```json
{
  "msgid": 9,
  "id": 123,
  "friendid": 456
}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `msgid` | int | 消息类型，固定为 `9` |
| `id` | int | 发起方用户 ID |
| `friendid` | int | 目标好友用户 ID |

---

### 10. 删除好友响应 `DELETE_FRIEND_MSG_ACK (10)`

**方向**：服务端 → 客户端

```json
{
  "msgid": 10,
  "errno": 0
}
```

---

### 11. 创建群组请求 `CREATE_GROUP_MSG (11)`

**方向**：客户端 → 服务端

```json
{
  "msgid": 11,
  "id": 123,
  "groupname": "C++ 学习群",
  "groupdesc": "C++ 技术交流群"
}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `msgid` | int | 消息类型，固定为 `11` |
| `id` | int | 创建者用户 ID |
| `groupname` | string | 群组名称 |
| `groupdesc` | string | 群组描述 |

---

### 12. 创建群组响应 `CREATE_GROUP_MSG_ACK (12)`

**方向**：服务端 → 客户端

```json
{
  "msgid": 12,
  "errno": 0,
  "groupid": 1
}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `msgid` | int | 消息类型，固定为 `12` |
| `errno` | int | 错误码，`0` 表示成功 |
| `groupid` | int | 成功时返回的群组 ID |

---

### 13. 加入群组请求 `ADD_GROUP_MSG (13)`

**方向**：客户端 → 服务端

```json
{
  "msgid": 13,
  "id": 123,
  "groupid": 1
}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `msgid` | int | 消息类型，固定为 `13` |
| `id` | int | 用户 ID |
| `groupid` | int | 目标群组 ID |

---

### 14. 加入群组响应 `ADD_GROUP_MSG_ACK (14)`

**方向**：服务端 → 客户端

```json
{
  "msgid": 14,
  "errno": 0
}
```

---

### 15. 退出群组请求 `QUIT_GROUP_MSG (15)`

**方向**：客户端 → 服务端

```json
{
  "msgid": 15,
  "id": 123,
  "groupid": 1
}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `msgid` | int | 消息类型，固定为 `15` |
| `id` | int | 用户 ID |
| `groupid` | int | 要退出的群组 ID |

---

### 16. 退出群组响应 `QUIT_GROUP_MSG_ACK (16)`

**方向**：服务端 → 客户端

```json
{
  "msgid": 16,
  "errno": 0
}
```

---

### 17. 群聊消息 `GROUP_CHAT_MSG (17)`

**方向**：双向（客户端发送 / 服务端广播）

```json
{
  "msgid": 17,
  "id": 123,
  "name": "john_doe",
  "groupid": 1,
  "msg": "大家好！",
  "time": "2025-01-10 15:00:00"
}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `msgid` | int | 消息类型，固定为 `17` |
| `id` | int | 发送方用户 ID |
| `name` | string | 发送方用户名 |
| `groupid` | int | 目标群组 ID |
| `msg` | string | 消息内容 |
| `time` | string | 发送时间 |

**服务端行为**：
- 查询群组所有成员
- 对在线成员：直接推送消息
- 对离线成员：存入离线消息，待登录时下发

---

### 18. 好友状态变更推送 `FRIEND_STATE_MSG (18)`

**方向**：服务端 → 客户端（服务端主动推送）

```json
{
  "msgid": 18,
  "id": 456,
  "name": "alice",
  "state": "online"
}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `msgid` | int | 消息类型，固定为 `18` |
| `id` | int | 状态变更的好友用户 ID |
| `name` | string | 好友用户名 |
| `state` | string | 新的在线状态：`"online"` 或 `"offline"` |

**触发时机**：
- 用户登录成功时，通知其所有在线好友
- 用户登出或连接断开时，通知其所有在线好友

---

## 错误码约定

| `errno` | 含义 |
|---|---|
| `0` | 操作成功 |
| `1` | 操作失败（通用错误，详见 `errmsg`） |

---

## 协议扩展说明

1. **离线消息格式**：单聊和群聊的离线消息均存储为 JSON 字符串，随登录响应（`LOG_MSG_ACK`）一起返回
2. **时间格式**：统一使用 `YYYY-MM-DD HH:MM:SS` 格式
3. **连接管理**：客户端异常断开时，服务端会自动清理连接并通知其好友下线
4. **分布式场景**：跨节点消息通过 Redis Pub/Sub 转发，对客户端透明
