# MyChat 项目简历说明（全项目覆盖版）

## 1) 项目定位（一句话）

MyChat 是一个覆盖服务端、客户端、可观测、压测和 AI 扩展的 C++ 即时通讯系统，具备从开发到部署再到监控和评估的完整工程闭环。

---

## 2) 项目整体组成（可直接口述）

- **服务端（src/server）**：Muduo Reactor 网络模型，负责登录注册、单聊群聊、好友群组、离线消息、跨节点转发。
- **客户端（src/client）**：Qt Widgets 桌面端，负责登录、会话、联系人/群组管理、AI 助手交互。
- **数据与路由**：MySQL 负责持久化，Redis Pub/Sub 负责多实例消息同步。
- **运维与观测（scripts + monitoring）**：提供启停脚本、健康检查、指标导出、Prometheus/Grafana 本地演示。
- **实验与报告（scripts + exports）**：支持压测数据生成、检查、对比分析与可视化输出。
- **AI 扩展（ai + scripts）**：接入 Ollama 本地模型，支持客户端 AI 会话。

---

## 3) 项目结构（简历中可引用）

```text
MyChatServer/
├── src/server/                 # ChatServer: 网络+业务+模型
├── src/client/                 # ChatClient: Qt UI + Network
├── scripts/                    # 启停、健康检查、压测、导出、AI
├── monitoring/                 # Prometheus/Grafana 配置
├── exports/                    # 运行指标与压测报告产物
├── ai/                         # AI 模型演示编排
├── include/                    # 公共协议与服务端头文件
└── thirdparty/                 # 第三方头文件
```

---

## 4) 技术栈（中英文可直接粘贴）

- Linux / C++11
- Muduo (Reactor) / TCP-IP / JSON over TCP
- MySQL / Redis Pub-Sub
- Qt Widgets / QTcpSocket
- CMake / Bash Scripting
- Prometheus / Grafana
- Ollama (optional)

---

## 5) 个人贡献（全链路版本）

1. **核心 IM 能力建设**：实现登录注册、单聊群聊、好友与群组管理、离线消息存储与拉取。
2. **分布式消息路由**：基于 Redis Pub/Sub 打通多实例消息同步。
3. **协议与稳定性优化**：完善消息分发与容错，增强字段兼容，提升链路稳定性。
4. **数据安全与一致性优化**：改进好友/群组相关业务流程，增强数据库安全性与可维护性。
5. **工程化交付**：构建一键编译、服务启停、健康检查、指标导出与排障脚本体系。
6. **可观测与量化评估**：落地 Prometheus/Grafana 本地演示与压测对比报告流程。
7. **AI 能力拓展**：接入本地模型推理服务，实现客户端 AI 助手会话。

---

## 6) 简历项目描述（中文，直接可贴）

**MyChat 分布式即时通讯系统（C++/Muduo/MySQL/Redis/Qt）**  
构建 Linux 下 C++ IM 系统，服务端基于 Muduo Reactor 处理长连接与业务分发，MySQL 负责用户/好友/群组/离线消息持久化，Redis Pub/Sub 支撑多实例消息转发。客户端使用 Qt 实现会话、联系人与群组交互。项目配套脚本化构建部署、健康检查、Prometheus/Grafana 可观测能力与压测对比报告流程，并扩展本地 AI 助手会话功能。

---

## 7) Resume Bullet（英文，直接可贴）

- Built a Linux-first IM system in C++ with Muduo (Reactor), supporting login, one-to-one/group chat, friend/group management, offline message storage, and multi-instance message routing via Redis Pub/Sub.
- Developed a Qt desktop client with TCP/JSON protocol handling, real-time chat interactions, contact/group management, and AI-assistant conversation integration.
- Delivered full engineering workflow including CMake-based build, script-driven deployment and health checks, metrics export, Prometheus/Grafana observability demo, and benchmark comparison reports.

---

## 8) 面试 1 分钟讲解模板

我做的是一个完整链路的 C++ 即时通讯项目，不只是聊天功能本身。后端基于 Muduo Reactor，支持登录、单聊群聊、好友群组和离线消息；MySQL 做持久化，Redis 做多实例消息同步。客户端用 Qt 做桌面会话交互。除了业务能力，我还做了脚本化部署、健康检查、Prometheus/Grafana 可观测和压测对比报告，另外还接了本地 AI 助手会话。整体上，这个项目覆盖了开发、部署、观测和优化的全流程。

---

## 9) 适配岗位建议

- Linux C++ 后端开发
- 即时通讯/长连接服务开发
- 分布式系统与消息中间件方向
- 基础架构与可观测平台方向



