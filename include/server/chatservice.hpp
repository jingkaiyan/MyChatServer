#ifndef CHATSERVICE_H
#define CHATSERVICE_H

/*
 * 聊天业务服务声明：
 * 1) 作为业务层单例，集中管理聊天相关的处理逻辑（登录、注册等）。
 * 2) 通过消息ID -> 处理函数的映射，统一分发客户端请求。
 * 3) 对外暴露具体业务入口（login、reg），供网络层回调调用。
 */
#include<muduo/net/TcpConnection.h>
#include<unordered_map>
#include<functional>
#include<nlohmann/json.hpp>
#include "model/usermodel.hpp"
#include<mutex>
#include"model/offlinemessagemodel.hpp"
#include"model/friendmodel.hpp"
#include"model/groupmodel.hpp"
#include"redis/redis.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;
using nlohmann::json;

//处理消息的事件回调方法类型
using MsgHandler = function<void(const TcpConnectionPtr &conn,json &js,Timestamp time)>;

//聊天服务器业务类
class ChatService
{
public:
    //获取单例对象的接口函数
    static ChatService* instance();
    //处理登录业务
    void login(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //处理注册业务
    void reg(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //添加好友业务
    void addFriend(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //群组业务
    void createGroup(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //加入群组业务
    void addGroup(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //群聊业务
    void groupChat(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    //处理注销业务的处理器
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 重置用户状态信息
    void reset();
    // 通知好友状态变化
    void notifyFriendsState(int userid, const string &state);
    //从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int userid,string msg);
private:
    ChatService();  //单例模式
    //存储消息id和其对应的业务处理方法
    unordered_map<int,MsgHandler> _msgHandlerMap;
    //存储在线用户的通信连接
    unordered_map<int,TcpConnectionPtr> _userConnMap;
    //存储好友信息
    FriendModel _friendModel;
    //数据操作类对象
    UserModel _userModel;
    // 互斥锁，保护_userConnMap的线程安全
    mutex _connMutex;
    // 离线消息操作对象
    OfflineMsgModel _offlineMsgModel;
    //群组业务操作对象
    GroupModel _groupModel;
    //redis对象
    Redis _redis;
};
#endif // !CHATSERVICE_H