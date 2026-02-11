/*
 * 聊天业务服务实现：
 * - 单例模式，集中管理聊天相关业务（如登录、注册）。
 * - 启动时将消息ID与对应业务处理函数绑定，供网络层分发调用。
 * - 提供 getHandler(msgid) 供外部按消息类型获取回调。
 */
#include <iostream>
#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include "db/db.h"
#include <mutex>
#include <vector>
#include <map>

using namespace muduo;
using namespace std;

// 获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}
// 注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    // 多实例部署时不要在此处重置在线状态，避免互相覆盖
    // 将登录消息插入到哈希表中
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    // 将注册消息插入到哈希表中
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    // 将一对一聊天消息插入到哈希表中
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    // 将添加好友消息插入到哈希表中
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    // 将创建群组消息插入到哈希表中
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    // 将加入群组消息插入到哈希表中
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    // 将群聊消息插入到哈希表中
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    // 登出
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});

    // 连接redis服务器
    if (_redis.connect())
    {
        // 设置上报消息的回调函数
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}
// 获取对应的消息处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        // 返回空的处理器（调用方应检查）
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR << "[服务] 未找到消息处理器 msgid=" << msgid;
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// 处理登录业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.query(id);
    if (user.getId() == id && user.getPassword() == pwd)
    {
        if (user.getState() == "online")
        {
            // 用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOG_MSG_ACK;
            response["error"] = 2;
            response["errmsg"] = "this account is using,input another!";
            conn->send(response.dump());
        }
        else
        {
            // 登陆成功，记录用户连接信息
            {
                // 由于登录操作可能被多个连接同时触发，加一个锁，线程安全考虑
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }
            // 登陆成功，向redis订阅channel(id)
            _redis.subscribe(id);

            // 登陆成功，更新用户状态信息offline -> online
            user.setState("online");
            _userModel.updateState(user);

            // 通知好友上线状态
            notifyFriendsState(id, "online");

            json response;
            response["msgid"] = LOG_MSG_ACK;
            response["error"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            // 查询用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                // 读取该用户的离线消息后，删除掉用户的离线消息
                _offlineMsgModel.remove(id);
            }
            // 查询好友的登录信息，并返回
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                vector<string> vec2;
                // 由于User没有转化成json的方法，所以需要手动转换成json，区别于之前的处理离线消息
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }
            // 查询用户的群组信息，并返回
            vector<Group> groupVec = _groupModel.queryGroups(id);
            if (!groupVec.empty())
            {
                vector<string> vec3;
                for (Group &group : groupVec)
                {
                    json grpjs;
                    grpjs["id"] = group.getId();
                    grpjs["groupname"] = group.getName();
                    grpjs["groupdesc"] = group.getDesc();

                    vector<string> userVec;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userVec.push_back(js.dump());
                    }
                    grpjs["users"] = userVec;
                    vec3.push_back(grpjs.dump());
                }
                response["groups"] = vec3;
            }
            conn->send(response.dump());
        }
    }
    else
    {
        // 用户不存在，登录失败
        json response;
        response["msgid"] = LOG_MSG_ACK;
        response["error"] = 1;
        response["errmsg"] = "id or password is invalid!";
        conn->send(response.dump());
    }
}

// 处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPassword(pwd);
    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["error"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["error"] = 1;
        conn->send(response.dump());
    }
}
// 处理注销
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }
    // 用户下线后，在redis中取消订阅
    _redis.unsubscribe(userid);

    // 更新用户的状态信息
    User user;
    user.setId(userid);
    user.setState("offline");
    _userModel.updateState(user);

    // 通知好友下线状态
    notifyFriendsState(userid, "offline");
}
// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        // 加锁，保护_userConnMap的线程安全，防止与其他线程的插入/查找并发
        lock_guard<mutex> lock(_connMutex);
        // 遍历用户连接，找到对应的要删除的连接
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); it++)
        {
            if (it->second == conn)
            {
                // 从map表删除用户的连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    //异常退出，取消用户的订阅消息
    _redis.unsubscribe(user.getId());
    // 更新用户状态
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
        // 通知好友下线状态
        notifyFriendsState(user.getId(), "offline");
    }
}
// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 获取对方id
    int toid = js["to"].get<int>();
    // 查找toid用户是否在线
    {
        // 上锁，避免在访问_userConnMap时出现并发读写竞态，登录、下线、单聊发送等问题
        lock_guard<mutex> lock(_connMutex);
        // 查找目标用户是否在线
        auto it = _userConnMap.find(toid);
        // 目标用户在线，转发消息
        if (it != _userConnMap.end())
        {
            // toid在线，转发消息    服务器主动推送消息给toid用户
            it->second->send(js.dump());
            // 显示个人聊天消息
            cout << js["time"].get<string>() << "[" << js["id"].get<int>() << "]"
                 << js["name"].get<string>() << " said " << js["msg"].get<string>() << endl;
            return;
        }
    }
    // toid在线，但是在不同服务器
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }
    // toid不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}

// 重置用户状态信息
void ChatService::reset()
{
    // 把所有用户的状态设置成offline
    _userModel.resetState();
}

// 通知好友状态变化
void ChatService::notifyFriendsState(int userid, const string &state)
{
    vector<User> friends = _friendModel.query(userid);
    if (friends.empty())
    {
        return;
    }

    json msg;
    msg["msgid"] = FRIEND_STATE_MSG;
    msg["id"] = userid;
    msg["state"] = state;

    for (User &f : friends)
    {
        int friendid = f.getId();
        // 先查本机连接
        {
            lock_guard<mutex> lock(_connMutex);
            auto it = _userConnMap.find(friendid);
            if (it != _userConnMap.end())
            {
                it->second->send(msg.dump());
                continue;
            }
        }

        // 不在本机，查询是否在线，在线则通过Redis投递
        User u = _userModel.query(friendid);
        if (u.getState() == "online")
        {
            _redis.publish(friendid, msg.dump());
        }
    }
}

// 添加好友业务 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();
    // 存储好友信息
    _friendModel.insert(userid, friendid);

    // 返回添加好友响应并携带最新好友列表
    json response;
    response["msgid"] = ADD_FRIEND_MSG_ACK;
    response["error"] = 0;

    vector<User> userVec = _friendModel.query(userid);
    if (!userVec.empty())
    {
        vector<string> vec;
        for (User &user : userVec)
        {
            json js;
            js["id"] = user.getId();
            js["name"] = user.getName();
            js["state"] = user.getState();
            vec.push_back(js.dump());
        }
        response["friends"] = vec;
    }

    conn->send(response.dump());

    // 若对方在线，推送对方的好友列表更新
    TcpConnectionPtr friendConn;
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(friendid);
        if (it != _userConnMap.end())
        {
            friendConn = it->second;
        }
    }
    if (friendConn)
    {
        json friendResp;
        friendResp["msgid"] = ADD_FRIEND_MSG_ACK;
        friendResp["error"] = 0;

        vector<User> friendVec = _friendModel.query(friendid);
        if (!friendVec.empty())
        {
            vector<string> vec;
            for (User &user : friendVec)
            {
                json js;
                js["id"] = user.getId();
                js["name"] = user.getName();
                js["state"] = user.getState();
                vec.push_back(js.dump());
            }
            friendResp["friends"] = vec;
        }

        friendConn->send(friendResp.dump());
    }
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];
    // 存储新创建的群组信息
    Group group(-1, name, desc);
    json response;
    response["msgid"] = CREATE_GROUP_MSG_ACK;
    if (_groupModel.createGroup(group))
    {
        // 存储群组创建人的信息
        _groupModel.addGroup(userid, group.getId(), "creator");
        response["error"] = 0;
        response["groupid"] = group.getId();
    }
    else
    {
        response["error"] = 1;
    }
    conn->send(response.dump());
}

// 加入群聊业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    // 存储用户的群组信息
    json response;
    response["msgid"] = ADD_GROUP_MSG_ACK;
    if (_groupModel.addGroup(userid, groupid, "normal"))
    {
        response["error"] = 0;
        response["groupid"] = groupid;
    }
    else
    {
        response["error"] = 1;
        response["groupid"] = groupid;
    }
    conn->send(response.dump());
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupusers(userid, groupid);
    // 添加互斥锁，保护后面群聊详细相关操作的线程安全
    lock_guard<mutex> lock(_connMutex);
    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id); // 在用户连接表中查找群成员的连接信息
        if (it != _userConnMap.end())
        {
            // 转发群消息
            it->second->send(js.dump());
            // 显示群消息
            cout << "群消息[" << groupid << "]" << js.value("time", "")
                 << "[" << js["id"].get<int>() << "]" << js.value("name", "")
                 << " said " << js.value("msg", "") << endl;
        }
        else
        {
            // 查询toid是否在线
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                // 存储离线群消息
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}

// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }
    //存储用户的离线消息
    _offlineMsgModel.insert(userid, msg);
}
