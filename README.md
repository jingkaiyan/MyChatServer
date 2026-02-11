# MyChatServer
这是一个基于 Linux 的即时通讯项目，包含 ChatServer 与 ChatClient 两端：  服务端基于 Muduo 网络库，使用 MySQL 存储用户、好友、群组与离线消息； 客户端通过 TCP 与服务端交互，支持注册、登录、好友/群组、单聊/群聊等功能； 服务端通过 Redis 发布/订阅实现多服务器之间的消息转发与在线状态同步； Nginx stream 模块用于 TCP 负载均衡，支持多实例部署与扩展。
