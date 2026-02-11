#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>

using namespace std;

class Redis
{ 
public:
    Redis();    // 构造函数
    ~Redis();   // 析构函数

    //连接redis服务器
    bool connect();

    //向redis指定的通道channel发布消息
    bool publish(int channel,string message);

    //向redis指定的通道subscribe订阅消息
    bool subscribe(int channel);

    //向redis指定的通道unsubscribe取消订阅消息
    bool unsubscribe(int channel);

    //在独立线程中接受订阅通道消息
    /*
    * 由于Redis的订阅是阻塞式的，如果在主线程中接收消息，会影响主线程处理其他业务逻辑
    * 所以需要在单独的线程中接收订阅的消息，然后通过回调函数上报给业务层
    */
    void observer_channel_message();

    //初始化向上层业务上报通道消息的回调函数
    void init_notify_handler(function<void(int,string)> fn);
private:
    //hiredis同步上下文对象，负责publish消息
    redisContext* _publish_context;
    //hiredis同步上下文对象，负责subscribe消息
    redisContext* _subscribe_context;

    //回调函数，受到订阅消息，给sercvice层上报
    function<void(int,string)> _notify_message_handler;
};
#endif // !REDIS_H