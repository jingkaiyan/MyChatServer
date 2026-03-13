#include "LoginDialog.h"
#include "ui_LoginDialog.h"
#include "RegisterDialog.h"
#include "../network/NetworkClient.h"
#include <QMessageBox>
#include <QDebug>

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog),
    m_loggedUserId(-1)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    ui->userIdEdit->setPlaceholderText("请输入账号 ID（数字）");
    ui->passwordEdit->setPlaceholderText("请输入登录密码");
    ui->loginButton->setDefault(true);
    
    // 连接网络信号
    NetworkClient& client = NetworkClient::instance();
    connect(&client, &NetworkClient::connected, this, &LoginDialog::onConnected);
    connect(&client, &NetworkClient::disconnected, this, &LoginDialog::onDisconnected);
    connect(&client, &NetworkClient::loginResult, this, &LoginDialog::onLoginResult);
    connect(&client, &NetworkClient::errorOccurred, this, &LoginDialog::onNetworkError);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

QString LoginDialog::getUserId() const
{
    return ui->userIdEdit->text();
}

QString LoginDialog::getPassword() const
{
    return ui->passwordEdit->text();
}

void LoginDialog::on_loginButton_clicked()
{
    if(getUserId().isEmpty() || getPassword().isEmpty()) {
        QMessageBox::warning(this, "提示", "账号和密码不能为空");
        return;
    }
    
    // 验证用户 ID 是否为数字
    bool ok = false;
    int userId = getUserId().toInt(&ok);
    if (!ok || userId <= 0) {
        QMessageBox::warning(this, "提示", "账号必须是正整数");
        return;
    }
    
    // 连接服务器
    connectToServer();
}

void LoginDialog::on_registerButton_clicked()
{
    RegisterDialog regDlg(this);
    regDlg.exec();
}

void LoginDialog::connectToServer()
{
    NetworkClient& client = NetworkClient::instance();
    
    if (client.isConnected()) {
        // 已连接，直接发送登录请求
        bool ok = false;
        int userId = getUserId().toInt(&ok);
        if (ok) {
            ui->loginButton->setText("登录中...");
            setUIEnabled(false);
            client.login(userId, getPassword());
        }
    } else {
        // 未连接，先连接服务器
        ui->loginButton->setText("连接中...");
        setUIEnabled(false);
        // 默认连接本地服务器，可以改成配置
        client.connectToServer("127.0.0.1", 8000);
    }
}

void LoginDialog::onConnected()
{
    qDebug() << "[LoginDialog] 服务器连接成功，发送登录请求";
    ui->loginButton->setText("登录中...");
    
    bool ok = false;
    int userId = getUserId().toInt(&ok);
    if (ok) {
        NetworkClient::instance().login(userId, getPassword());
    }
}

void LoginDialog::onDisconnected()
{
    qDebug() << "[LoginDialog] 与服务器断开连接";
    ui->loginButton->setText("登录");
    setUIEnabled(true);
}

void LoginDialog::onLoginResult(bool success, const QString& message, int userId, const QString& userName)
{
    ui->loginButton->setText("登录");
    setUIEnabled(true);
    
    if (success) {
        m_loggedUserId = userId;
        m_loggedUserName = userName;
        QMessageBox::information(this, "登录成功", QString("欢迎您，%1！").arg(userName));
        accept(); // 关闭对话框并返回成功
    } else {
        QMessageBox::warning(this, "登录失败", message);
    }
}

void LoginDialog::onNetworkError(const QString& error)
{
    ui->loginButton->setText("登录");
    setUIEnabled(true);
    QMessageBox::critical(this, "网络错误", QString("连接服务器失败：%1").arg(error));
}

void LoginDialog::setUIEnabled(bool enabled)
{
    ui->userIdEdit->setEnabled(enabled);
    ui->passwordEdit->setEnabled(enabled);
    ui->loginButton->setEnabled(enabled);
    ui->registerButton->setEnabled(enabled);
}
