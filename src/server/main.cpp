/*
 * 程序入口：
 * - 创建事件循环与监听地址。
 * - 构建 ChatServer 对象并启动服务。
 * - 进入事件循环，开始处理网络事件。
 */
#include "chatserver.hpp"
#include "chatservice.hpp"
#include<iostream>
#include<string>
#include<signal.h>

using namespace std;

//处理服务器cltr+c结束后，重置user的状态信息
void resetHandler(int)
{
    ChatService::instance() -> reset();
    exit(0);
}
int main(int argc, char **argv)
{
    signal(SIGINT,resetHandler);

    string ip = "127.0.0.1";
    uint16_t port = 6000;

    if (argc >= 3)
    {
        ip = argv[1];
        port = static_cast<uint16_t>(atoi(argv[2]));
    }
    else
    {
        cout << "请输入服务器IP(默认 127.0.0.1): ";
        string inputIp;
        getline(cin, inputIp);
        if (!inputIp.empty())
        {
            ip = inputIp;
        }

        cout << "请输入端口(默认 6000): ";
        string inputPort;
        getline(cin, inputPort);
        if (!inputPort.empty())
        {
            port = static_cast<uint16_t>(atoi(inputPort.c_str()));
        }
    }

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop,addr,"ChatServer");

    server.start();
    loop.loop();

    return 0;
}