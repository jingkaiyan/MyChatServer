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
#include <atomic>
#include <chrono>
#include <nlohmann/json.hpp>
#include <muduo/base/Logging.h>
#include "chatservice.hpp"

using namespace std;
using namespace placeholders; // 占位符
using json = nlohmann::json;

namespace
{
atomic<unsigned long long> g_msgCounter{0};
atomic<unsigned long long> g_parseErrorCounter{0};

string buildTraceId()
{
    static atomic<unsigned long long> seq{0};
    auto now = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
    auto n = ++seq;
    return string("tr_") + to_string(now) + "_" + to_string(n);
}
}

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
    if (conn->connected())
    {
        LOG_INFO << "[connection] event=connected peer=" << conn->peerAddress().toIpPort();
        return;
    }

    //客户端断开连接
    if(!conn -> connected())
    {
        LOG_INFO << "[connection] event=disconnected peer=" << conn->peerAddress().toIpPort();
        ChatService::instance() -> clientCloseException(conn);  //客户端异常退出
        conn -> shutdown();
    }
}

// 读写事件监听
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buf,
                           Timestamp time)
{
    const char *eol = nullptr;
    while ((eol = buf->findEOL()) != nullptr)
    {
        string line(buf->peek(), eol);
        buf->retrieveUntil(eol + 1);

        if (line.empty())
        {
            continue;
        }

        try
        {
            json js = json::parse(line);
            if (!js.contains("msgid"))
            {
                LOG_WARN << "[服务] 收到缺少msgid的消息: " << line;
                continue;
            }

            if (!js.contains("trace_id"))
            {
                js["trace_id"] = buildTraceId();
            }

            auto msgid = js["msgid"].get<int>();
            auto traceId = js["trace_id"].get<string>();
            auto count = ++g_msgCounter;
            LOG_INFO << "[ingress] trace_id=" << traceId
                     << " msgid=" << msgid
                     << " peer=" << conn->peerAddress().toIpPort()
                     << " bytes=" << line.size()
                     << " total_msgs=" << count;

            auto msgHandler = ChatService::instance()->getHandler(msgid);
            msgHandler(conn, js, time);
        }
        catch (const json::exception &e)
        {
            auto errCount = ++g_parseErrorCounter;
            LOG_WARN << "[服务] JSON解析失败: " << e.what() << " raw=" << line;
            LOG_WARN << "[ingress] parse_error_total=" << errCount
                     << " peer=" << conn->peerAddress().toIpPort();
        }
    }

    const size_t kMaxBufferedBytes = 1024 * 1024;
    if (buf->readableBytes() > kMaxBufferedBytes)
    {
        LOG_WARN << "[服务] 缓冲区超限，丢弃未完成数据 bytes=" << buf->readableBytes();
        buf->retrieveAll();
    }
}
