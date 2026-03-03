#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "../network/NetworkClient.h"
#include <QMessageBox>
#include <QDateTime>
#include <QListWidgetItem>
#include <QDebug>

// 构造函数，初始化UI
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , currentChatUserId(-1)
    , currentChatGroupId(-1)
    , isChatWithGroup(false)
{
    ui->setupUi(this);
    ui->messageEdit->setPlaceholderText("输入消息，按 Enter 可快速发送...");
    connect(ui->messageEdit, &QLineEdit::returnPressed, ui->sendButton, &QPushButton::click);
    connect(ui->friendListWidget, &QListWidget::itemClicked, this, &MainWindow::on_friendItem_clicked);
    connect(ui->groupListWidget, &QListWidget::itemClicked, this, &MainWindow::on_groupItem_clicked);
    connect(ui->contactSearchEdit, &QLineEdit::textChanged, this, &MainWindow::on_contactSearch_textChanged);

    ui->chatBrowser->setOpenExternalLinks(true);
    
    // 连接网络信号
    connectNetworkSignals();
    
    appendSystemTip("欢迎使用 MyChat，正在加载好友和群组列表 ✨");
    
    // 显示当前登录用户信息
    NetworkClient& client = NetworkClient::instance();
    QString userName = client.getCurrentUserName();
    int userId = client.getCurrentUserId();
    setWindowTitle(QString("MyChat - %1 (ID: %2)").arg(userName).arg(userId));
}

// 析构函数，释放UI资源
MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connectNetworkSignals()
{
    NetworkClient& client = NetworkClient::instance();
    connect(&client, &NetworkClient::friendListReceived, this, &MainWindow::onFriendListReceived);
    connect(&client, &NetworkClient::groupListReceived, this, &MainWindow::onGroupListReceived);
    connect(&client, &NetworkClient::offlineMessagesReceived, this, &MainWindow::onOfflineMessagesReceived);
    connect(&client, &NetworkClient::chatMessageReceived, this, &MainWindow::onChatMessageReceived);
    connect(&client, &NetworkClient::groupMessageReceived, this, &MainWindow::onGroupMessageReceived);
    connect(&client, &NetworkClient::friendStateChanged, this, &MainWindow::onFriendStateChanged);
    connect(&client, &NetworkClient::disconnected, this, &MainWindow::onNetworkDisconnected);
}

// 发送按钮点击事件处理
void MainWindow::on_sendButton_clicked()
{
    QString msg = ui->messageEdit->text().trimmed();
    if(msg.isEmpty()) {
        QMessageBox::warning(this, "提示", "消息不能为空");
        return;
    }

    if (currentChatTarget.isEmpty()) {
        QMessageBox::information(this, "提示", "请先从左侧选择一个好友或群组");
        return;
    }

    NetworkClient& client = NetworkClient::instance();
    
    if (isChatWithGroup) {
        // 发送群聊消息
        if (currentChatGroupId > 0) {
            client.sendGroupMessage(currentChatGroupId, msg);
            appendChatMessage("我", msg, true);
            ui->messageEdit->clear();
        }
    } else {
        // 发送单聊消息
        if (currentChatUserId > 0) {
            client.sendChatMessage(currentChatUserId, msg);
            appendChatMessage("我", msg, true);
            ui->messageEdit->clear();
        }
    }
}

void MainWindow::on_friendItem_clicked(QListWidgetItem *item)
{
    if (item == nullptr) {
        return;
    }
    
    QString friendName = item->text();
    // 移除状态后缀（如 "(在线)"）
    int pos = friendName.indexOf(" (");
    if (pos > 0) {
        friendName = friendName.left(pos);
    }
    
    currentChatTarget = friendName;
    currentChatUserId = m_friendNameToId.value(friendName, -1);
    currentChatGroupId = -1;
    isChatWithGroup = false;
    
    ui->currentChatLabel->setText(QString("当前会话：好友 %1").arg(friendName));
    appendSystemTip(QString("已切换到好友会话：%1 (ID: %2)").arg(friendName).arg(currentChatUserId));
}

void MainWindow::on_groupItem_clicked(QListWidgetItem *item)
{
    if (item == nullptr) {
        return;
    }
    
    QString groupName = item->text();
    // 移除成员数后缀（如 "(23)"）
    int pos = groupName.indexOf(" (");
    if (pos > 0) {
        groupName = groupName.left(pos);
    }
    
    currentChatTarget = groupName;
    currentChatGroupId = m_groupNameToId.value(groupName, -1);
    currentChatUserId = -1;
    isChatWithGroup = true;
    
    ui->currentChatLabel->setText(QString("当前会话：群组 %1").arg(groupName));
    appendSystemTip(QString("已切换到群组会话：%1 (ID: %2)").arg(groupName).arg(currentChatGroupId));
}

void MainWindow::onFriendListReceived(const json& friendList)
{
    qDebug() << "[MainWindow] 收到好友列表";
    
    ui->friendListWidget->clear();
    m_friendIdToName.clear();
    m_friendNameToId.clear();
    
    for (const auto& friendJson : friendList) {
        int id = friendJson["id"].get<int>();
        std::string name = friendJson["name"].get<std::string>();
        std::string state = friendJson["state"].get<std::string>();
        
        QString qName = QString::fromStdString(name);
        QString qState = QString::fromStdString(state);
        
        m_friendIdToName[id] = qName;
        m_friendNameToId[qName] = id;
        
        QString displayName = QString("%1 (%2)").arg(qName).arg(qState);
        ui->friendListWidget->addItem(displayName);
    }
    
    updateContactStats();
    appendSystemTip(QString("好友列表已更新，共 %1 位好友").arg(friendList.size()));
    
    // 默认选中第一个好友
    if (ui->friendListWidget->count() > 0) {
        ui->friendListWidget->setCurrentRow(0);
        on_friendItem_clicked(ui->friendListWidget->item(0));
    }
}

void MainWindow::onGroupListReceived(const json& groupList)
{
    qDebug() << "[MainWindow] 收到群组列表";
    
    ui->groupListWidget->clear();
    m_groupIdToName.clear();
    m_groupNameToId.clear();
    
    for (const auto& groupJson : groupList) {
        int id = groupJson["id"].get<int>();
        std::string name = groupJson["groupname"].get<std::string>();
        
        QString qName = QString::fromStdString(name);
        
        m_groupIdToName[id] = qName;
        m_groupNameToId[qName] = id;
        
        // 计算群成员数
        int memberCount = 0;
        if (groupJson.contains("users")) {
            memberCount = groupJson["users"].size();
        }
        
        QString displayName = QString("%1 (%2)").arg(qName).arg(memberCount);
        ui->groupListWidget->addItem(displayName);
    }
    
    updateContactStats();
    appendSystemTip(QString("群组列表已更新，共 %1 个群组").arg(groupList.size()));
}

void MainWindow::onOfflineMessagesReceived(const json& messages)
{
    qDebug() << "[MainWindow] 收到离线消息";
    
    if (messages.empty()) {
        return;
    }
    
    appendSystemTip(QString("您有 %1 条离线消息").arg(messages.size()));
    
    for (const auto& msgJson : messages) {
        try {
            int fromId = msgJson["from"].get<int>();
            std::string fromName = msgJson.value("name", "未知");
            std::string content = msgJson["msg"].get<std::string>();
            std::string timeStr = msgJson.value("time", "");
            
            QString sender = QString::fromStdString(fromName);
            QString message = QString::fromStdString(content);
            QString time = QString::fromStdString(timeStr);
            
            QString now = time.isEmpty() ? QDateTime::currentDateTime().toString("hh:mm:ss") : time;
            ui->chatBrowser->append(QString("<b style='color:#f59e0b;'>离线</b> <b style='color:#60a5fa;'>%1</b> <span style='color:#94a3b8;'>[%2]</span>: %3")
                                   .arg(sender, now, message.toHtmlEscaped()));
        } catch (const json::exception& e) {
            qWarning() << "[MainWindow] 解析离线消息失败:" << e.what();
        }
    }
}

void MainWindow::onChatMessageReceived(int fromUserId, const QString& userName, const QString& message)
{
    qDebug() << "[MainWindow] 收到单聊消息: from=" << fromUserId << userName;
    
    // 如果当前正在和这个人聊天，直接显示
    if (!isChatWithGroup && currentChatUserId == fromUserId) {
        appendChatMessage(userName, message);
    } else {
        // 否则显示系统提示
        appendSystemTip(QString("收到 %1 的新消息: %2").arg(userName, message));
    }
}

void MainWindow::onGroupMessageReceived(int groupId, int fromUserId, const QString& userName, const QString& message)
{
    qDebug() << "[MainWindow] 收到群聊消息: groupId=" << groupId << "from=" << fromUserId << userName;
    
    // 如果当前正在这个群聊，直接显示
    if (isChatWithGroup && currentChatGroupId == groupId) {
        appendChatMessage(userName, message);
    } else {
        QString groupName = m_groupIdToName.value(groupId, QString::number(groupId));
        appendSystemTip(QString("收到群 %1 中 %2 的新消息: %3").arg(groupName, userName, message));
    }
}

void MainWindow::onFriendStateChanged(int friendId, const QString& state)
{
    qDebug() << "[MainWindow] 好友状态变更: friendId=" << friendId << "state=" << state;
    
    QString friendName = m_friendIdToName.value(friendId);
    if (friendName.isEmpty()) {
        return;
    }
    
    // 更新好友列表中的状态显示
    for (int i = 0; i < ui->friendListWidget->count(); ++i) {
        QListWidgetItem* item = ui->friendListWidget->item(i);
        QString itemText = item->text();
        if (itemText.startsWith(friendName + " (")) {
            item->setText(QString("%1 (%2)").arg(friendName, state));
            break;
        }
    }
    
    appendSystemTip(QString("好友 %1 状态变更为：%2").arg(friendName, state));
}

void MainWindow::onNetworkDisconnected()
{
    QMessageBox::warning(this, "连接断开", "与服务器的连接已断开，请重新登录");
    close();
}

void MainWindow::appendSystemTip(const QString &text)
{
    QString now = QDateTime::currentDateTime().toString("hh:mm:ss");
    ui->chatBrowser->append(QString("<span style='color:#38bdf8;'>系统</span> <span style='color:#94a3b8;'>[%1]</span>：%2")
                           .arg(now, text.toHtmlEscaped()));
}

void MainWindow::appendChatMessage(const QString& sender, const QString& message, bool isSent)
{
    QString now = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString color = isSent ? "#60a5fa" : "#10b981";
    QString arrow = isSent ? "→" : "";
    QString target = isSent ? QString(" <span style='color:#fbbf24;'>%1</span>").arg(currentChatTarget) : "";
    
    ui->chatBrowser->append(QString("<b style='color:%1;'>%2</b>%3%4 <span style='color:#94a3b8;'>[%5]</span>: %6")
                           .arg(color, sender, arrow, target, now, message.toHtmlEscaped()));
}

void MainWindow::on_contactSearch_textChanged(const QString &text)
{
    const QString keyword = text.trimmed();

    for (int i = 0; i < ui->friendListWidget->count(); ++i) {
        QListWidgetItem *item = ui->friendListWidget->item(i);
        item->setHidden(!keyword.isEmpty() && !item->text().contains(keyword, Qt::CaseInsensitive));
    }

    for (int i = 0; i < ui->groupListWidget->count(); ++i) {
        QListWidgetItem *item = ui->groupListWidget->item(i);
        item->setHidden(!keyword.isEmpty() && !item->text().contains(keyword, Qt::CaseInsensitive));
    }

    updateContactStats();
}

void MainWindow::updateContactStats()
{
    int visibleFriends = 0;
    int visibleGroups = 0;

    for (int i = 0; i < ui->friendListWidget->count(); ++i) {
        if (!ui->friendListWidget->item(i)->isHidden()) {
            ++visibleFriends;
        }
    }

    for (int i = 0; i < ui->groupListWidget->count(); ++i) {
        if (!ui->groupListWidget->item(i)->isHidden()) {
            ++visibleGroups;
        }
    }

    ui->contactStatsLabel->setText(QString("可见好友 %1 · 可见群组 %2").arg(visibleFriends).arg(visibleGroups));
}
