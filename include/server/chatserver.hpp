#ifndef CHARTSERVER_H
#define CHARTSERVER_H
/*
 * 聊天服务器对外接口声明：
 * 1) 封装 muduo::TcpServer，负责监听端口、接受连接。
 * 2) 设置连接/读写回调，把客户端 TCP 事件交给业务层处理。
 * 3) 对外提供 start() 启动服务，构造时注入事件循环与监听地址。
 */
#include <string>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/Timestamp.h>

using namespace muduo;
using namespace muduo::net;

class ChatServer
{
public:
    //初始化聊天服务器对象
        ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const std::string& nameArg);
    //启动服务
    void start();
private:
    //上报连接相关信息的回调函数
    void onConnection(const TcpConnectionPtr &conn);
    //上报读写事件相关的回调函数
    void onMessage(const TcpConnectionPtr &conn,
                   Buffer *buf,
                   Timestamp time);

    TcpServer server_;  //组合muduo库，实现服务器功能
    EventLoop *_loop;   //实现事件循环对象的指针
};

#endif // !CHARTSERVER_H