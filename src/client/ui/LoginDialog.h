#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>

namespace Ui {
class LoginDialog;
}

// 登录对话框类，负责用户登录界面
class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

    QString getUserId() const;
    QString getPassword() const;
    
    // 获取登录用户信息（登录成功后）
    int getLoggedUserId() const { return m_loggedUserId; }
    QString getLoggedUserName() const { return m_loggedUserName; }

private slots:
    void on_loginButton_clicked();
    
    // 网络回调
    void onConnected();
    void onDisconnected();
    void onLoginResult(bool success, const QString& message, int userId, const QString& userName);
    void onNetworkError(const QString& error);

private:
    void connectToServer();
    void setUIEnabled(bool enabled);

private:
    Ui::LoginDialog *ui;
    int m_loggedUserId;
    QString m_loggedUserName;
};

#endif // LOGINDIALOG_H
