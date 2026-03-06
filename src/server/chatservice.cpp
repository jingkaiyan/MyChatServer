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
#include <cstdlib>
#include <sstream>
#include <cstring>
#include <deque>
#include <algorithm>
#include <cctype>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace muduo;
using namespace std;

namespace
{
int readEnvInt(const char *key, int defaultValue, int minValue, int maxValue)
{
    const char *raw = getenv(key);
    if (raw == nullptr || *raw == '\0')
    {
        return defaultValue;
    }
    char *end = nullptr;
    long parsed = strtol(raw, &end, 10);
    if (end == raw || *end != '\0')
    {
        return defaultValue;
    }
    if (parsed < minValue)
    {
        return minValue;
    }
    if (parsed > maxValue)
    {
        return maxValue;
    }
    return static_cast<int>(parsed);
}

string toLowerCopy(string value)
{
    transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(tolower(ch));
    });
    return value;
}

string trim(const string &value)
{
    size_t start = 0;
    while (start < value.size() && isspace(static_cast<unsigned char>(value[start])))
    {
        ++start;
    }
    size_t end = value.size();
    while (end > start && isspace(static_cast<unsigned char>(value[end - 1])))
    {
        --end;
    }
    return value.substr(start, end - start);
}

bool decodeChunkedBody(const string &body, string &decoded, string &errmsg)
{
    decoded.clear();
    size_t pos = 0;
    while (pos < body.size())
    {
        size_t lineEnd = body.find("\r\n", pos);
        if (lineEnd == string::npos)
        {
            errmsg = "invalid chunked body: missing chunk size line";
            return false;
        }

        string sizeLine = body.substr(pos, lineEnd - pos);
        size_t semiPos = sizeLine.find(';');
        if (semiPos != string::npos)
        {
            sizeLine = sizeLine.substr(0, semiPos);
        }

        char *end = nullptr;
        unsigned long chunkSize = strtoul(sizeLine.c_str(), &end, 16);
        if (end == sizeLine.c_str() || *end != '\0')
        {
            errmsg = "invalid chunked body: bad chunk size";
            return false;
        }

        pos = lineEnd + 2;
        if (chunkSize == 0)
        {
            return true;
        }

        if (pos + chunkSize > body.size())
        {
            errmsg = "invalid chunked body: truncated chunk data";
            return false;
        }
        decoded.append(body, pos, chunkSize);
        pos += chunkSize;

        if (pos + 2 > body.size() || body.compare(pos, 2, "\r\n") != 0)
        {
            errmsg = "invalid chunked body: missing chunk terminator";
            return false;
        }
        pos += 2;
    }
    return true;
}

void sendJson(const TcpConnectionPtr &conn, const json &payload)
{
    conn->send(payload.dump() + "\n");
}

vector<string> buildGroupPayload(GroupModel &groupModel, int userid)
{
    vector<string> vec;
    vector<Group> groupVec = groupModel.queryGroups(userid);
    for (Group &group : groupVec)
    {
        json grpjson;
        grpjson["id"] = group.getId();
        grpjson["groupname"] = group.getName();
        grpjson["groupdesc"] = group.getDesc();

        vector<string> userV;
        for (GroupUser &user : group.getUsers())
        {
            json js;
            js["id"] = user.getId();
            js["name"] = user.getName();
            js["state"] = user.getState();
            js["role"] = user.getRole();
            userV.push_back(js.dump());
        }
        grpjson["users"] = userV;
        vec.push_back(grpjson.dump());
    }
    return vec;
}

bool requestLocalOllama(const json &messages, string &reply, string &errmsg)
{
    const char *hostEnv = getenv("MYCHAT_AI_HOST");
    const char *portEnv = getenv("MYCHAT_AI_PORT");
    const char *modelEnv = getenv("MYCHAT_AI_MODEL");

    string host = hostEnv ? hostEnv : "127.0.0.1";
    int port = portEnv ? atoi(portEnv) : 11434;
    string model = modelEnv ? modelEnv : "qwen2.5:0.5b";

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        errmsg = "create socket failed";
        return false;
    }

    int timeoutSec = readEnvInt("MYCHAT_AI_TIMEOUT_SEC", 20, 5, 180);
    timeval timeout;
    timeout.tv_sec = timeoutSec;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, host.c_str(), &servAddr.sin_addr) <= 0)
    {
        close(sockfd);
        errmsg = "invalid MYCHAT_AI_HOST";
        return false;
    }

    if (connect(sockfd, reinterpret_cast<sockaddr *>(&servAddr), sizeof(servAddr)) < 0)
    {
        close(sockfd);
        errmsg = "connect local ollama failed";
        return false;
    }

    json req;
    req["model"] = model;
    req["messages"] = messages;
    req["stream"] = false;
    string body = req.dump();

    ostringstream reqStream;
    reqStream << "POST /api/chat HTTP/1.1\r\n"
              << "Host: " << host << ":" << port << "\r\n"
              << "Content-Type: application/json\r\n"
              << "Connection: close\r\n"
              << "Content-Length: " << body.size() << "\r\n\r\n"
              << body;
    string rawReq = reqStream.str();

    size_t sent = 0;
    while (sent < rawReq.size())
    {
        ssize_t n = send(sockfd, rawReq.data() + sent, rawReq.size() - sent, 0);
        if (n <= 0)
        {
            close(sockfd);
            errmsg = "send request to ollama failed";
            return false;
        }
        sent += static_cast<size_t>(n);
    }

    string rawResp;
    char buf[4096];
    while (true)
    {
        ssize_t n = recv(sockfd, buf, sizeof(buf), 0);
        if (n == 0)
        {
            break;
        }
        if (n < 0)
        {
            close(sockfd);
            errmsg = "recv response from ollama failed";
            return false;
        }
        rawResp.append(buf, static_cast<size_t>(n));
    }
    close(sockfd);

    size_t splitPos = rawResp.find("\r\n\r\n");
    if (splitPos == string::npos)
    {
        errmsg = "invalid http response from ollama";
        return false;
    }

    string headers = rawResp.substr(0, splitPos);
    string rawBody = rawResp.substr(splitPos + 4);

    bool isChunked = false;
    size_t contentLength = 0;
    bool hasContentLength = false;

    istringstream headerStream(headers);
    string headerLine;
    while (getline(headerStream, headerLine))
    {
        if (!headerLine.empty() && headerLine.back() == '\r')
        {
            headerLine.pop_back();
        }

        size_t colon = headerLine.find(':');
        if (colon == string::npos)
        {
            continue;
        }

        string key = toLowerCopy(trim(headerLine.substr(0, colon)));
        string value = toLowerCopy(trim(headerLine.substr(colon + 1)));

        if (key == "transfer-encoding" && value.find("chunked") != string::npos)
        {
            isChunked = true;
        }
        else if (key == "content-length")
        {
            char *end = nullptr;
            unsigned long parsed = strtoul(value.c_str(), &end, 10);
            if (end != value.c_str() && *end == '\0')
            {
                contentLength = static_cast<size_t>(parsed);
                hasContentLength = true;
            }
        }
    }

    string jsonBody;
    if (isChunked)
    {
        if (!decodeChunkedBody(rawBody, jsonBody, errmsg))
        {
            return false;
        }
    }
    else if (hasContentLength)
    {
        if (rawBody.size() < contentLength)
        {
            errmsg = "invalid http response body length";
            return false;
        }
        jsonBody = rawBody.substr(0, contentLength);
    }
    else
    {
        jsonBody = rawBody;
    }

    jsonBody = trim(jsonBody);
    try
    {
        json resp = json::parse(jsonBody);
        if (!resp.contains("message") || !resp["message"].is_object())
        {
            errmsg = "ollama response missing message";
            return false;
        }
        if (!resp["message"].contains("content"))
        {
            errmsg = "ollama response missing content";
            return false;
        }
        reply = resp["message"]["content"].get<string>();
        return true;
    }
    catch (const json::exception &e)
    {
        errmsg = string("parse ollama response failed: ") + e.what();
        return false;
    }
}
}

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
    // 将删除好友消息插入到哈希表中
    _msgHandlerMap.insert({DELETE_FRIEND_MSG, std::bind(&ChatService::deleteFriend, this, _1, _2, _3)});
    // 将创建群组消息插入到哈希表中
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    // 将加入群组消息插入到哈希表中
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    // 将退出群组消息插入到哈希表中
    _msgHandlerMap.insert({QUIT_GROUP_MSG, std::bind(&ChatService::quitGroup, this, _1, _2, _3)});
    // 将群聊消息插入到哈希表中
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    // AI 对话
    _msgHandlerMap.insert({AI_CHAT_MSG, std::bind(&ChatService::aiChat, this, _1, _2, _3)});
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
    if (user.getId() == id && _userModel.checkPassword(id, pwd))
    {
        if (user.getState() == "online")
        {
            // 用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOG_MSG_ACK;
            response["error"] = 2;
            response["errmsg"] = "this account is using,input another!";
            sendJson(conn, response);
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
            sendJson(conn, response);
        }
    }
    else
    {
        // 用户不存在，登录失败
        json response;
        response["msgid"] = LOG_MSG_ACK;
        response["error"] = 1;
        response["errmsg"] = "id or password is invalid!";
        sendJson(conn, response);
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
        sendJson(conn, response);
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["error"] = 1;
        sendJson(conn, response);
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
    int senderId = js.value("id", js.value("from", -1));
    if (senderId < 0)
    {
        LOG_ERROR << "[服务] oneChat 缺少发送者ID";
        return;
    }
    js["id"] = senderId;
    js["from"] = senderId;
    if (!js.contains("name"))
    {
        User sender = _userModel.query(senderId);
        js["name"] = sender.getName();
    }
    if (!js.contains("time"))
    {
        js["time"] = time.toFormattedString();
    }

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
            sendJson(it->second, js);
            // 显示个人聊天消息
              cout << js["time"].get<string>() << "[" << senderId << "]"
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
                sendJson(it->second, msg);
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
    json response;
    response["msgid"] = ADD_FRIEND_MSG_ACK;

    if (userid == friendid)
    {
        response["error"] = 1;
        response["errmsg"] = "cannot add yourself";
        sendJson(conn, response);
        return;
    }

    User target = _userModel.query(friendid);
    if (target.getId() != friendid)
    {
        response["error"] = 1;
        response["errmsg"] = "user not found";
        sendJson(conn, response);
        return;
    }

    vector<User> currentFriends = _friendModel.query(userid);
    for (User &u : currentFriends)
    {
        if (u.getId() == friendid)
        {
            response["error"] = 1;
            response["errmsg"] = "already friends";
            sendJson(conn, response);
            return;
        }
    }

    bool inserted = _friendModel.insert(userid, friendid);
    if (!inserted)
    {
        response["error"] = 1;
        response["errmsg"] = "add friend failed";
        sendJson(conn, response);
        return;
    }

    // 返回添加好友响应并携带最新好友列表
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

    sendJson(conn, response);

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

        sendJson(friendConn, friendResp);
    }
}

// 删除好友业务 msgid id friendid
void ChatService::deleteFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();
    
    // 从数据库删除好友关系
    bool success = _friendModel.remove(userid, friendid);
    
    // 返回删除好友响应并携带最新好友列表
    json response;
    response["msgid"] = DELETE_FRIEND_MSG_ACK;
    response["error"] = success ? 0 : 1;

    if (success)
    {
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
    }

    sendJson(conn, response);

    // 若对方在线，推送对方的好友列表更新
    if (success)
    {
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
            friendResp["msgid"] = DELETE_FRIEND_MSG_ACK;
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

            sendJson(friendConn, friendResp);
        }
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
        response["groups"] = buildGroupPayload(_groupModel, userid);
    }
    else
    {
        response["error"] = 1;
        response["errmsg"] = "create group failed";
    }
    sendJson(conn, response);
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
        response["groups"] = buildGroupPayload(_groupModel, userid);
    }
    else
    {
        response["error"] = 1;
        response["groupid"] = groupid;
        response["errmsg"] = "join group failed";
    }
    sendJson(conn, response);
}

// 退出群组业务
void ChatService::quitGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    
    // 从数据库删除用户群组关系
    bool success = _groupModel.quitGroup(userid, groupid);
    
    json response;
    response["msgid"] = QUIT_GROUP_MSG_ACK;
    response["error"] = success ? 0 : 1;
    response["groupid"] = groupid;
    
    if (success)
    {
        response["groups"] = buildGroupPayload(_groupModel, userid);
    }
    else
    {
        response["errmsg"] = "quit group failed";
    }
    
    sendJson(conn, response);
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js.value("id", js.value("from", -1));
    if (userid < 0)
    {
        LOG_ERROR << "[服务] groupChat 缺少发送者ID";
        return;
    }
    js["id"] = userid;
    js["from"] = userid;
    if (!js.contains("name"))
    {
        User sender = _userModel.query(userid);
        js["name"] = sender.getName();
    }
    if (!js.contains("time"))
    {
        js["time"] = time.toFormattedString();
    }

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
            sendJson(it->second, js);
            // 显示群消息
            cout << "群消息[" << groupid << "]" << js.value("time", "")
                  << "[" << userid << "]" << js.value("name", "")
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
        it->second->send(msg + "\n");
        return;
    }
    //存储用户的离线消息
    _offlineMsgModel.insert(userid, msg);
}

void ChatService::aiChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js.value("id", -1);
    string prompt = js.value("msg", "");
    string action = js.value("action", "");
    int maxRounds = readEnvInt("MYCHAT_AI_MAX_ROUNDS", 8, 2, 30);
    size_t maxMessages = static_cast<size_t>(maxRounds * 2);

    json response;
    response["msgid"] = AI_CHAT_MSG_ACK;
    response["from"] = 0;
    response["id"] = 0;
    response["to"] = userid;
    response["name"] = "AI助手";
    response["time"] = time.toFormattedString();
    if (js.contains("trace_id"))
    {
        response["trace_id"] = js["trace_id"];
    }

    if (action == "reset" || prompt == "/reset")
    {
        lock_guard<mutex> lock(_aiContextMutex);
        _aiContexts.erase(userid);
        response["error"] = 0;
        response["msg"] = "已清空当前 AI 会话记忆";
        sendJson(conn, response);
        return;
    }

    if (prompt.empty())
    {
        response["error"] = 1;
        response["errmsg"] = "empty ai prompt";
        sendJson(conn, response);
        return;
    }

    json userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = prompt;

    json requestMessages = json::array();
    {
        lock_guard<mutex> lock(_aiContextMutex);
        deque<json> &history = _aiContexts[userid];
        history.push_back(userMessage);
        while (history.size() > maxMessages)
        {
            history.pop_front();
        }
        for (const auto &message : history)
        {
            requestMessages.push_back(message);
        }
    }

    string aiReply;
    string errmsg;
    if (!requestLocalOllama(requestMessages, aiReply, errmsg))
    {
        lock_guard<mutex> lock(_aiContextMutex);
        auto it = _aiContexts.find(userid);
        if (it != _aiContexts.end() && !it->second.empty())
        {
            it->second.pop_back();
            if (it->second.empty())
            {
                _aiContexts.erase(it);
            }
        }

        response["error"] = 1;
        response["errmsg"] = "ai service unavailable: " + errmsg;
        sendJson(conn, response);
        LOG_WARN << "[AI] request failed userid=" << userid << " errmsg=" << errmsg;
        return;
    }

    {
        lock_guard<mutex> lock(_aiContextMutex);
        deque<json> &history = _aiContexts[userid];
        json assistantMessage;
        assistantMessage["role"] = "assistant";
        assistantMessage["content"] = aiReply;
        history.push_back(assistantMessage);
        while (history.size() > maxMessages)
        {
            history.pop_front();
        }
    }

    response["error"] = 0;
    response["msg"] = aiReply;
    sendJson(conn, response);
}
