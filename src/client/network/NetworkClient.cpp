#include "NetworkClient.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

NetworkClient& NetworkClient::instance()
{
    static NetworkClient instance;
    return instance;
}

NetworkClient::NetworkClient(QObject* parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_currentUserId(-1)
{
    connect(m_socket, &QTcpSocket::connected, this, &NetworkClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &NetworkClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &NetworkClient::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &NetworkClient::onSocketError);
}

NetworkClient::~NetworkClient()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
}

void NetworkClient::connectToServer(const QString& ip, quint16 port)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        qDebug() << "[Network] 已经连接到服务器";
        return;
    }
    
    qDebug() << "[Network] 正在连接服务器" << ip << ":" << port;
    m_socket->connectToHost(ip, port);
}

void NetworkClient::disconnectFromServer()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
}

bool NetworkClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void NetworkClient::login(int userId, const QString& password)
{
    json loginMsg;
    loginMsg["msgid"] = LOGIN_MSG;
    loginMsg["id"] = userId;
    loginMsg["password"] = password.toStdString();
    
    sendMessage(loginMsg);
    qDebug() << "[Network] 发送登录请求: userId=" << userId;
}

void NetworkClient::registerUser(const QString& name, const QString& password)
{
    json regMsg;
    regMsg["msgid"] = REG_MSG;
    regMsg["name"] = name.toStdString();
    regMsg["password"] = password.toStdString();
    
    sendMessage(regMsg);
    qDebug() << "[Network] 发送注册请求: name=" << name;
}

void NetworkClient::sendChatMessage(int toUserId, const QString& message)
{
    json chatMsg;
    chatMsg["msgid"] = ONE_CHAT_MSG;
    chatMsg["from"] = m_currentUserId;
    chatMsg["to"] = toUserId;
    chatMsg["msg"] = message.toStdString();
    
    sendMessage(chatMsg);
    qDebug() << "[Network] 发送单聊消息: to=" << toUserId;
}

void NetworkClient::sendGroupMessage(int groupId, const QString& message)
{
    json groupMsg;
    groupMsg["msgid"] = GROUP_CHAT_MSG;
    groupMsg["from"] = m_currentUserId;
    groupMsg["groupid"] = groupId;
    groupMsg["msg"] = message.toStdString();
    
    sendMessage(groupMsg);
    qDebug() << "[Network] 发送群聊消息: groupId=" << groupId;
}

void NetworkClient::addFriend(int friendId)
{
    json addMsg;
    addMsg["msgid"] = ADD_FRIEND_MSG;
    addMsg["id"] = m_currentUserId;
    addMsg["friendid"] = friendId;
    
    sendMessage(addMsg);
}

void NetworkClient::createGroup(const QString& groupName, const QString& groupDesc)
{
    json createMsg;
    createMsg["msgid"] = CREATE_GROUP_MSG;
    createMsg["id"] = m_currentUserId;
    createMsg["groupname"] = groupName.toStdString();
    createMsg["groupdesc"] = groupDesc.toStdString();
    
    sendMessage(createMsg);
}

void NetworkClient::joinGroup(int groupId)
{
    json joinMsg;
    joinMsg["msgid"] = ADD_GROUP_MSG;
    joinMsg["id"] = m_currentUserId;
    joinMsg["groupid"] = groupId;
    
    sendMessage(joinMsg);
}

void NetworkClient::sendMessage(const json& jsonMsg)
{
    if (!isConnected()) {
        qWarning() << "[Network] 未连接到服务器，无法发送消息";
        emit errorOccurred("未连接到服务器");
        return;
    }
    
    std::string msgStr = jsonMsg.dump();
    msgStr += '\n';  // 消息分隔符（与服务端约定）
    
    m_socket->write(msgStr.c_str(), msgStr.size());
    m_socket->flush();
}

void NetworkClient::onConnected()
{
    qDebug() << "[Network] 成功连接到服务器";
    m_readBuffer.clear();
    emit connected();
}

void NetworkClient::onDisconnected()
{
    qDebug() << "[Network] 与服务器断开连接";
    m_currentUserId = -1;
    m_currentUserName.clear();
    emit disconnected();
}

void NetworkClient::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    m_readBuffer.append(QString::fromUtf8(data));
    
    // 处理粘包：按行分割消息
    int pos = 0;
    while ((pos = m_readBuffer.indexOf('\n')) != -1) {
        QString line = m_readBuffer.left(pos).trimmed();
        m_readBuffer.remove(0, pos + 1);
        
        if (line.isEmpty()) {
            continue;
        }
        
        try {
            json jsonMsg = json::parse(line.toStdString());
            handleMessage(jsonMsg);
        } catch (const json::exception& e) {
            qWarning() << "[Network] JSON 解析失败:" << e.what() << "原始数据:" << line;
        }
    }
}

void NetworkClient::onSocketError(QAbstractSocket::SocketError socketError)
{
    QString errorMsg = m_socket->errorString();
    qWarning() << "[Network] Socket 错误:" << errorMsg << "错误码:" << socketError;
    emit errorOccurred(errorMsg);
}

void NetworkClient::handleMessage(const json& jsonMsg)
{
    if (!jsonMsg.contains("msgid")) {
        qWarning() << "[Network] 消息缺少 msgid 字段";
        return;
    }
    
    int msgId = jsonMsg["msgid"].get<int>();
    
    switch (msgId) {
    case LOG_MSG_ACK:
        handleLoginResponse(jsonMsg);
        break;
    case REG_MSG_ACK:
        handleRegisterResponse(jsonMsg);
        break;
    case ONE_CHAT_MSG:
        handleChatMessage(jsonMsg);
        break;
    case GROUP_CHAT_MSG:
        handleGroupMessage(jsonMsg);
        break;
    case FRIEND_STATE_MSG:
        handleFriendStateChange(jsonMsg);
        break;
    default:
        qDebug() << "[Network] 收到未处理的消息类型:" << msgId;
        break;
    }
}

void NetworkClient::handleLoginResponse(const json& jsonMsg)
{
    int errno_val = jsonMsg["errno"].get<int>();
    std::string errmsg = jsonMsg.value("errmsg", "");
    
    if (errno_val != 0) {
        // 登录失败
        qDebug() << "[Network] 登录失败:" << QString::fromStdString(errmsg);
        emit loginResult(false, QString::fromStdString(errmsg), -1, "");
        return;
    }
    
    // 登录成功
    m_currentUserId = jsonMsg["id"].get<int>();
    m_currentUserName = QString::fromStdString(jsonMsg["name"].get<std::string>());
    
    qDebug() << "[Network] 登录成功: userId=" << m_currentUserId << "userName=" << m_currentUserName;
    emit loginResult(true, "登录成功", m_currentUserId, m_currentUserName);
    
    // 发送好友列表
    if (jsonMsg.contains("friends")) {
        emit friendListReceived(jsonMsg["friends"]);
    }
    
    // 发送群组列表
    if (jsonMsg.contains("groups")) {
        emit groupListReceived(jsonMsg["groups"]);
    }
    
    // 发送离线消息
    if (jsonMsg.contains("offlinemsg")) {
        emit offlineMessagesReceived(jsonMsg["offlinemsg"]);
    }
}

void NetworkClient::handleRegisterResponse(const json& jsonMsg)
{
    int errno_val = jsonMsg["errno"].get<int>();
    std::string errmsg = jsonMsg.value("errmsg", "");
    
    if (errno_val != 0) {
        emit registerResult(false, QString::fromStdString(errmsg));
    } else {
        int userId = jsonMsg["id"].get<int>();
        emit registerResult(true, QString("注册成功，您的账号是: %1").arg(userId));
    }
}

void NetworkClient::handleChatMessage(const json& jsonMsg)
{
    int fromUserId = jsonMsg["from"].get<int>();
    std::string userName = jsonMsg["name"].get<std::string>();
    std::string message = jsonMsg["msg"].get<std::string>();
    
    emit chatMessageReceived(fromUserId, QString::fromStdString(userName), QString::fromStdString(message));
}

void NetworkClient::handleGroupMessage(const json& jsonMsg)
{
    int groupId = jsonMsg["groupid"].get<int>();
    int fromUserId = jsonMsg["from"].get<int>();
    std::string userName = jsonMsg["name"].get<std::string>();
    std::string message = jsonMsg["msg"].get<std::string>();
    
    emit groupMessageReceived(groupId, fromUserId, QString::fromStdString(userName), QString::fromStdString(message));
}

void NetworkClient::handleFriendStateChange(const json& jsonMsg)
{
    int friendId = jsonMsg["id"].get<int>();
    std::string state = jsonMsg["state"].get<std::string>();
    
    emit friendStateChanged(friendId, QString::fromStdString(state));
}
