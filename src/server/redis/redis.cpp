#include "server/redis/redis.hpp"
#include <iostream>
#include <cstdlib>


using namespace std;

Redis::Redis() : _publish_context(nullptr), _subscribe_context(nullptr){}
Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }
    if (_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context);
    }
}

// 连接redis服务器
bool Redis::connect()
{
    const char *redisHost = std::getenv("REDIS_HOST");
    const char *redisPort = std::getenv("REDIS_PORT");
    string host = redisHost ? redisHost : "127.0.0.1";
    int port = redisPort ? atoi(redisPort) : 6379;

    _publish_context = redisConnect(host.c_str(), port);
    if (_publish_context == nullptr)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }
    // 负责subscribe订阅消息的上下文连接
    _subscribe_context = redisConnect(host.c_str(), port);
    if (_subscribe_context == nullptr)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }
    // 在单独的线程中，监听通道上的事件，有消息给业务层进行上报
    thread t([&]()
             { observer_channel_message(); }); // 使用lambda表达式创建线程,目的是避免线程创建后自动结束
    t.detach();                                // 分离线程
    cout << "connect redis-server success!" << endl;
    return true;
}

// 向redis指定的通道channel发布消息
bool Redis::publish(int channel, string message) //(非阻塞)
{
    // 向channel通道发布消息
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (reply == nullptr)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel) //(阻塞)
{
    // SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅，不接收通道消息
    if (redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel) == REDIS_ERR)
    {
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite用于将命令发送给redis服务器
    int done = 0;
    while (!done)
    {
        if (redisBufferWrite(this->_subscribe_context, &done) == REDIS_ERR)
        {
            cerr << "buffer write failed!" << endl;
            return false;
        }
    }
    return true;
}

// 向redis指定的通道unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if (redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel) == REDIS_ERR)
    {
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite用于将命令发送给redis服务器
    int done = 0;
    while (!done)
    {
        if (redisBufferWrite(this->_subscribe_context, &done) == REDIS_ERR)
        {
            cerr << "buffer write failed!" << endl;
            return false;
        }
    }
    return true;
}

// 在独立线程中接受订阅通道消息
/*
 * 由于Redis的订阅是阻塞式的，如果在主线程中接收消息，会影响主线程处理其他业务逻辑
 * 所以需要在单独的线程中接收订阅的消息，然后通过回调函数上报给业务层
 */
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    // 循环获取信息
    while (redisGetReply(this->_subscribe_context, (void **)&reply) == REDIS_OK)
    {
        // 订阅收到的消息是一个带三个元素的数组
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 给业务层上报通道上发生的消息
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr << ">>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<" << endl;
}

// 初始化向上层业务上报通道消息的回调函数
void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler = fn;
}
