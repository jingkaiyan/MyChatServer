/*
 * 聊天服务器实现：
 * - 封装 Muduo TcpServer 的初始化、回调注册、线程数配置。
 * - onConnection 处理连接建立/断开，断开时关闭连接。
 * - onMessage 读取客户端数据，反序列化 JSON，并后续交给业务层按 msgid 分发。
 */
#include "chatserver.hpp"
#include <functional> // 参数对象绑定器
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include "chatservice.hpp"

using namespace std;
using namespace placeholders; // 占位符
using json = nlohmann::json;
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : server_(loop, listenAddr, nameArg), _loop(loop)
{
    // 注册链接回调函数
    server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
    // 注册读写事件相关回调函数
    server_.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    server_.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    server_.start();
}

// 连接监听
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    //客户端断开连接
    if(!conn -> connected())
    {
        ChatService::instance() -> clientCloseException(conn);  //客户端异常退出
        conn -> shutdown();
    }
}

// 读写事件监听
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buf,
                           Timestamp time)
{
   string buffer = buf -> retrieveAllAsString();
   //数据的反序列化
   json js = json::parse(buffer);
   //达到的目的：完全解耦网络模块的代码和业务模块的代码
   //通过js["msgid"]获取业务handler -> conn js time
   auto msgHandler = ChatService::instance() -> getHandler(js["msgid"].get<int>());
   //调用消息对应的业务处理方法
   msgHandler(conn,js,time);
}
