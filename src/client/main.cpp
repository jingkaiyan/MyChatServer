#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "model/group.hpp"
#include "model/user.hpp"
#include "public.hpp"

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;

// 控制主菜单页面程序
bool isMainMenuRunning = false;

// 用于读写线程之间的通信
sem_t rwsem;
// 记录登录状态
atomic_bool g_isLoginSuccess{false}; // 使用原子布尔类型确保多线程安全

// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();
// 日志前缀
string logPrefix();
// 主聊天页面程序
void mainMenu(int);
// 显示当前登录成功用户的基本信息
void showCurrentUserData();

// 聊天客户端程序实现，main线程用作发送线程，子线程用作接收线程
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        /// 命令行参数不够，使用标准错误输出提示用户
        cerr << logPrefix() << "[错误] 参数无效，示例: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << logPrefix() << "[错误] socket 创建失败" << endl;
        exit(-1);
    }

    // 填写client需要连接的server信息ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client和server进行连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << logPrefix() << "[错误] 连接服务器失败" << endl;
        close(clientfd);
        exit(-1);
    }

    // 初始化读写线程通信用的信号量
    sem_init(&rwsem, 0, 0); // 保证主线程和子线程之间的同步

    // 连接服务器成功，启动接收子线程
    thread readTask(readTaskHandler, clientfd); // pthread_create
    readTask.detach();                          // pthread_detach

    // main线程用于接收用户输入，负责发送数据
    for (;;)
    {
        // 显示首页面菜单 登录、注册、退出
        cout << "\n================= 主菜单 =================" << endl;
        cout << "  1. 登录" << endl;
        cout << "  2. 注册" << endl;
        cout << "  3. 退出" << endl;
        cout << "==========================================" << endl;
        cout << "请选择: ";
        int choice = 0;
        cin >> choice;
        cin.get(); // 读掉缓冲区残留的回车

        switch (choice)
        {
        case 1: // login业务
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "用户ID: ";
            cin >> id;
            cin.get(); // 读掉缓冲区残留的回车
            cout << "密  码: ";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            g_isLoginSuccess = false; // 重置登录状态
            // 向服务器发送登录请求
            int len = send(clientfd, request.c_str(), request.size() + 1, 0);
            if (len == -1)
            {
                cerr << logPrefix() << "[错误] 发送登录请求失败" << endl;
            }
            sem_wait(&rwsem);     // 等待信号量，子线程处理完信息后，通知这里
            if (g_isLoginSuccess) // 登录成功
            {
                // 进入主菜单页面
                isMainMenuRunning = true;
                mainMenu(clientfd);
            }
        }
        break;
        case 2: // register业务
        {
            // 定义数组存放用户名和密码
            char name[50] = {0};
            char pwd[50] = {0};

            cout << "用户名: ";
            cin.getline(name, 50);
            cout << "密  码: ";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), request.size() + 1, 0);
            if (len == -1)
            {
                cerr << logPrefix() << "[错误] 发送注册请求失败" << endl;
            }
            sem_wait(&rwsem); // 等待信号量，子线程处理完信息后，通知这里
        }
        break;
        case 3: // quit业务
        {
            close(clientfd);
            sem_destroy(&rwsem);
            exit(0);
        }
        default:
            cerr << logPrefix() << "[错误] 无效输入" << endl;
            break;
        }
    }
    return 0;
}

// 处理注册的响应逻辑
void doRegResponse(json &responsejs)
{
    if (responsejs["error"].get<int>() != 0) // 注册失败
    {
        cerr << logPrefix() << "[失败] 注册失败：用户名已存在" << endl;
    }
    else // 注册成功
    {
        cout << logPrefix() << "[成功] 注册成功，您的用户ID为: " << responsejs["id"] << "（请牢记）" << endl;
    }
}
// 处理登录的响应逻辑
void doLoginResponse(json &responsejs)
{
    if (responsejs["error"].get<int>() != 0) // 登录失败
    {
        cerr << logPrefix() << "[失败] 登录失败：" << responsejs["errmsg"] << endl;
        g_isLoginSuccess = false;
    }
    else // 登录成功
    {
        // 记录当前用户的id和name
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);
        // 记录当前用户的好友信息列表
        if (responsejs.contains("friends"))
        {
            // 初始化好友列表
            g_currentUserFriendList.clear();
            // 创建一个容器存储好友列表信息
            vector<string> vec = responsejs["friends"];
            for (string &str : vec)
            {
                json js = json::parse(str); // 反序列化
                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }
        // 记录当前用户的群组信息列表
        if (responsejs.contains("groups"))
        {
            // 初始化群组信息列表
            g_currentUserGroupList.clear();
            // 创建一个容器存储群组列表信息
            vector<string> vec1 = responsejs["groups"];
            for (string &groupstr : vec1)
            {
                json grpjs = json::parse(groupstr); // 反序列化
                Group group;
                group.setId(grpjs["id"].get<int>());
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);
                // 创建一个容器存储群组中各个用户的信息
                vector<string> vec2 = grpjs["users"];
                for (string &userstr : vec2)
                {
                    json js = json::parse(userstr); // 反序列化
                    GroupUser user;
                    user.setId(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);
                    group.getUsers().push_back(user);
                }
                g_currentUserGroupList.push_back(group);
            }
        }
        // 显示登录用户的基本信息
        showCurrentUserData();
        // 显示当前用户的离线消息 个人聊天消息或者群组消息
        if (responsejs.contains("offlinemsg"))
        {
            vector<string> vec = responsejs["offlinemsg"];
            for (string &str : vec)
            {
                json js = json::parse(str);
                // 个人聊天消息 / 群消息
                if (ONE_CHAT_MSG == js["msgid"])
                {
                    cout << "\n" << logPrefix() << "[私聊] " << js["time"].get<string>()
                         << "  用户ID: " << js["id"].get<int>()
                         << "  姓名: " << js["name"].get<string>()
                         << "  内容: " << js["msg"].get<string>() << endl;
                }
                else // 群消息
                {
                    cout << "\n" << logPrefix() << "[群聊] 群ID: " << js["groupid"]
                         << "  时间: " << js["time"].get<string>()
                         << "  用户ID: " << js["id"].get<int>()
                         << "  姓名: " << js["name"].get<string>()
                         << "  内容: " << js["msg"].get<string>() << endl;
                }
            }
        }
        cout << logPrefix() << "[成功] 登录成功，欢迎回来，" << g_currentUser.getName() << "！" << endl;
        g_isLoginSuccess = true;
    }
}

// 处理添加好友的响应逻辑
void doAddFriendResponse(json &responsejs)
{
    if (responsejs["error"].get<int>() != 0) // 添加好友失败
    {
        cerr << logPrefix() << "[失败] 添加好友失败：" << responsejs["errmsg"] << endl;
    }
    else // 添加好友成功
    {
        cout << logPrefix() << "[成功] 添加好友成功" << endl;
        if (responsejs.contains("friends"))
        {
            g_currentUserFriendList.clear();
            vector<string> vec = responsejs["friends"];
            for (string &str : vec)
            {
                json js = json::parse(str);
                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
            showCurrentUserData();
        }
    }
}

// 处理创建群组的响应逻辑
void doCreateGroupResponse(json &responsejs)
{
    if (responsejs["error"].get<int>() != 0)
    {
        cerr << logPrefix() << "[失败] 创建群组失败" << endl;
    }
    else
    {
        cout << logPrefix() << "[成功] 创建群组成功，群ID: " << responsejs["groupid"] << endl;
    }
}

// 处理加入群组的响应逻辑
void doAddGroupResponse(json &responsejs)
{
    if (responsejs["error"].get<int>() != 0)
    {
        cerr << logPrefix() << "[失败] 加入群组失败，群ID: " << responsejs["groupid"] << endl;
    }
    else
    {
        cout << logPrefix() << "[成功] 已加入群组，群ID: " << responsejs["groupid"] << endl;
    }
}

// 子线程，接收消息
void readTaskHandler(int clientfd)
{
    for (;;)
    {
        // 定义一个数组存储接收的数据
        char buffer[1024] = {0};
        // 接收数据
        int len = recv(clientfd, buffer, 1024, 0);
        // 如果没有接收到数据，退出线程
        if (len == -1 || len == 0)
        {
            close(clientfd);
            exit(-1);
        }
        // 接收到ChatServer发送的数据后，进行反序列化
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>(); // 获取消息类型
        if (msgtype == ONE_CHAT_MSG)
        {
            cout << "\n"
                 << logPrefix() << "[私聊] " << js["time"].get<string>()
                 << "  用户ID: " << js["id"].get<int>()
                 << "  姓名: " << js["name"].get<string>()
                 << "  内容: " << js["msg"].get<string>() << endl;
        }
        if (msgtype == GROUP_CHAT_MSG) // 群聊消息
        {
            cout << "\n"
                 << logPrefix() << "[群聊] 群ID: " << js["groupid"]
                 << "  时间: " << js["time"].get<string>()
                 << "  用户ID: " << js["id"].get<int>()
                 << "  姓名: " << js["name"].get<string>()
                 << "  内容: " << js["msg"].get<string>() << endl;
        }
        if (msgtype == LOG_MSG_ACK) // 登录响应消息
        {
            doLoginResponse(js); // 处理登录响应业务逻辑
            sem_post(&rwsem);    // 通知主线程，登录响应处理完毕
            continue;
        }
        if (msgtype == REG_MSG_ACK) // 注册响应消息
        {
            doRegResponse(js); // 处理注册响应业务逻辑
            sem_post(&rwsem);  // 通知主线程，注册响应处理完毕
            continue;
        }
        if (msgtype == ADD_FRIEND_MSG_ACK) // 添加好友响应消息
        {
            doAddFriendResponse(js); // 处理添加好友响应业务逻辑
            continue;
        }
        if (msgtype == CREATE_GROUP_MSG_ACK) // 创建群组响应消息
        {
            doCreateGroupResponse(js);
            continue;
        }
        if (msgtype == ADD_GROUP_MSG_ACK) // 加入群组响应消息
        {
            doAddGroupResponse(js);
            continue;
        }
        if (msgtype == FRIEND_STATE_MSG) // 好友状态变更
        {
            int friendid = js["id"].get<int>();
            string state = js["state"].get<string>();
            for (User &user : g_currentUserFriendList)
            {
                if (user.getId() == friendid)
                {
                    user.setState(state);
                    break;
                }
            }
            cout << "\n" << logPrefix() << "[提示] 好友 " << friendid << " 状态变更为 " << state << endl;
            showCurrentUserData();
            continue;
        }
    }
}

// 显示当前成功登录用户的基本信息
void showCurrentUserData()
{
    cout << "\n================ 当前登录用户 ================" << endl;
    cout << "ID: " << g_currentUser.getId() << "   姓名: " << g_currentUser.getName() << endl;
    cout << "----------------- 好友列表 ------------------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << "ID: " << user.getId() << "   姓名: " << user.getName() << "   状态: " << user.getState() << endl;
        }
    }
    cout << "----------------- 群组列表 ------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << "群ID: " << group.getId() << "   名称: " << group.getName() << "   描述: " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << "  - 成员ID: " << user.getId() << "   姓名: " << user.getName()
                     << "   状态: " << user.getState() << "   角色: " << user.getRole() << endl;
            }
        }
    }
    cout << "==========================================" << endl;
}

// ”help" command handler
void help(int fd = 0, string str = "");
// 处理创建群组的响应逻辑
void doCreateGroupResponse(json &responsejs);
// 处理加入群组的响应逻辑
void doAddGroupResponse(json &responsejs);
// “chat" command handler
void chat(int, string);
// ”addfriend" command handler
void addfriend(int, string);
// “creategroup" command handler
void creategroup(int, string);
//"addgroup" command handler
void addgroup(int, string);
//"groupchat" command handler
void groupchat(int, string);
//"loginout" command handler
void loginout(int, string);

// 系统支持的客户端命令列表
unordered_map<string, string> commandMap =
    {
        {"help", "显示所有支持的命令，格式help"},
        {"chat", "一对一聊天，格式chat:friendid:message"},
        {"addfriend", "添加好友，格式addfriend:friendid"},
        {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
        {"addgroup", "加入群组，格式addgroup:groupid"},
        {"groupchat", "群聊，格式groupchat:groupid:message"},
        {"loginout", "注销，格式loginout"}};

// 注册系统支持的客户端处理命令
unordered_map<string, function<void(int, string)>> commanHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};

// 主聊天页面程序
void mainMenu(int clientfd)
{
    help();
    char buffer[1024] = {0}; // 定义数组，存放用户的输入命令
    while (isMainMenuRunning)
    {
        cin.getline(buffer, 1024);         // 获取用户输入的一行命令
        string commandbuf(buffer);         // 转换成string类型，便于操作
        string command;                    // 存放命令
        size_t idx = commandbuf.find(":"); // 查找用户输入的命令中是否有:分隔符
        if (idx == -1)
        {
            command = commandbuf; // 没有:分隔符，说明用户输入的命令没有参数
        }
        else
        {
            command = commandbuf.substr(0, idx); // 获取命令
        }
        auto it = commanHandlerMap.find(command); // 查找命令
        if (it == commanHandlerMap.end())
        {
            cerr << logPrefix() << "[错误] 无效命令，请输入 help 查看" << endl;
            continue;
        }
        // 调用相应的事件处理回调
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx)); // 调用命令处理方法
    }
}

// ”help" command handler
void help(int clientfdfd, string str)
{
    cout << "\n"
         << logPrefix() << "可用命令列表:" << endl;
    for (auto &p : commandMap)
    {
        cout << "  " << p.first << "  ->  " << p.second << endl;
    }
    cout << endl;
}
//"addfriend" command handler
void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << logPrefix() << "[错误] 发送添加好友请求失败" << endl;
    }
    else
    {
        cout << logPrefix() << "[已发送] 添加好友请求" << endl;
    }
}
//"chat" command handler
void chat(int clientfd, string str)
{
    int idx = str.find(":"); // friendid:message
    if (idx == -1)
    {
        cerr << logPrefix() << "[错误] 私聊命令格式错误：chat:friendid:message" << endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);
    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["to"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << logPrefix() << "[错误] 发送私聊消息失败" << endl;
    }
    else
    {
        cout << logPrefix() << "[已发送] 私聊消息" << endl;
    }
}

//"creategroup" command handler
void creategroup(int clientfd, string str)
{
    int idx = str.find(":"); // groupname:groupdesc
    if (idx == -1)
    {
        cerr << logPrefix() << "[错误] 创建群组命令格式错误：creategroup:groupname:groupdesc" << endl;
        return;
    }

    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);
    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << logPrefix() << "[错误] 创建群组请求发送失败" << endl;
        return;
    }
    cout << logPrefix() << "[已发送] 创建群组请求" << endl;
}
//"addgroup" command handler
void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << logPrefix() << "[错误] 加入群组请求发送失败" << endl;
    }
    else
    {
        cout << logPrefix() << "[已发送] 加入群组请求" << endl;
    }
}
// "groupchat" command handler
void groupchat(int clientfd, string str)
{
    int idx = str.find(":"); // groupid:message
    if (idx == -1)
    {
        cerr << logPrefix() << "[错误] 群聊命令格式错误：groupchat:groupid:message" << endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);
    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << logPrefix() << "[错误] 发送群聊消息失败" << endl;
        return;
    }
    cout << logPrefix() << "[已发送] 群聊消息" << endl;
}
//"loginout" command handler
void loginout(int clientfd, string str)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << logPrefix() << "[错误] 发送注销请求失败" << endl;
    }
    else
    {
        isMainMenuRunning = false;                                         // 结束主菜单
        cout << logPrefix() << "[成功] 注销完成，返回登录界面..." << endl; // 提示用户注销成功
    }
}
// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return string(date);
}

string logPrefix()
{
    return "[" + getCurrentTime() + "] ";
}