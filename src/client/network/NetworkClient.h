#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QString>
#include <QHostAddress>
#include "../../thirdparty/json.hpp"
#include "../../include/public.hpp"

using json = nlohmann::json;

/**
 * @brief 网络客户端类 - 单例模式
 * 负责与 ChatServer 的 TCP 通信、JSON 协议封装与解析
 */
class NetworkClient : public QObject
{
    Q_OBJECT

public:
    static NetworkClient& instance();
    ~NetworkClient();

    // 连接管理
    void connectToServer(const QString& ip, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    // 业务接口
    void login(int userId, const QString& password);
    void registerUser(const QString& name, const QString& password);
    void sendChatMessage(int toUserId, const QString& message);
    void sendGroupMessage(int groupId, const QString& message);
    void addFriend(int friendId);
    void createGroup(const QString& groupName, const QString& groupDesc);
    void joinGroup(int groupId);

    // 当前登录用户信息
    int getCurrentUserId() const { return m_currentUserId; }
    QString getCurrentUserName() const { return m_currentUserName; }

signals:
    // 连接状态信号
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);

    // 业务消息信号
    void loginResult(bool success, const QString& message, int userId, const QString& userName);
    void registerResult(bool success, const QString& message);
    void friendListReceived(const json& friendList);
    void groupListReceived(const json& groupList);
    void offlineMessagesReceived(const json& messages);
    void chatMessageReceived(int fromUserId, const QString& userName, const QString& message);
    void groupMessageReceived(int groupId, int fromUserId, const QString& userName, const QString& message);
    void friendStateChanged(int friendId, const QString& state);

private:
    NetworkClient(QObject* parent = nullptr);
    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    void sendMessage(const json& jsonMsg);
    void handleMessage(const json& jsonMsg);
    
    // 具体消息处理
    void handleLoginResponse(const json& jsonMsg);
    void handleRegisterResponse(const json& jsonMsg);
    void handleChatMessage(const json& jsonMsg);
    void handleGroupMessage(const json& jsonMsg);
    void handleFriendStateChange(const json& jsonMsg);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket* m_socket;
    QString m_readBuffer;  // 处理粘包
    
    // 当前用户信息
    int m_currentUserId;
    QString m_currentUserName;
};

#endif // NETWORKCLIENT_H
