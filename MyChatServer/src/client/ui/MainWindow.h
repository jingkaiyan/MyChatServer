#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include "../../thirdparty/json.hpp"

using json = nlohmann::json;

class QListWidgetItem;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// 主聊天窗口类，负责显示聊天内容和发送消息
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // UI 事件槽函数
    void on_sendButton_clicked();
    void on_friendListWidget_itemClicked(QListWidgetItem *item);
    void on_groupListWidget_itemClicked(QListWidgetItem *item);
    void on_contactSearchEdit_textChanged(const QString &text);
    void on_addFriendButton_clicked();
    void on_deleteFriendButton_clicked();
    void on_createGroupButton_clicked();
    void on_joinGroupButton_clicked();
    void on_quitGroupButton_clicked();
    
    // 网络回调槽函数
    void onFriendListReceived(const json& friendList);
    void onGroupListReceived(const json& groupList);
    void onOfflineMessagesReceived(const json& messages);
    void onChatMessageReceived(int fromUserId, const QString& userName, const QString& message);
    void onGroupMessageReceived(int groupId, int fromUserId, const QString& userName, const QString& message);
    void onFriendStateChanged(int friendId, const QString& state);
    void onFriendOperationResult(bool success, const QString& message);
    void onNetworkDisconnected();
    void onNetworkError(const QString& error);

private:
    void connectNetworkSignals();
    void appendSystemTip(const QString &text);
    void updateContactStats();
    void appendChatMessage(const QString& sender, const QString& message, bool isSent = false);

    Ui::MainWindow *ui;
    QString currentChatTarget; // 当前聊天目标（显示名）
    int currentChatUserId;     // 当前聊天用户 ID
    int currentChatGroupId;    // 当前聊天群组 ID
    bool isChatWithGroup;      // 是否是群聊
    
    // ID 与显示名的映射
    QMap<int, QString> m_friendIdToName;  // 好友 ID -> 名称
    QMap<QString, int> m_friendNameToId;  // 好友名称 -> ID
    QMap<int, QString> m_groupIdToName;   // 群组 ID -> 名称
    QMap<QString, int> m_groupNameToId;   // 群组名称 -> ID
};

#endif // MAINWINDOW_H
