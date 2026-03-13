#include "RegisterDialog.h"
#include "ui_RegisterDialog.h"
#include "../network/NetworkClient.h"
#include <QMessageBox>
#include <QDebug>

RegisterDialog::RegisterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegisterDialog)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    ui->nameEdit->setPlaceholderText("请输入您的昵称");
    ui->passwordEdit->setPlaceholderText("请设置登录密码");
    ui->confirmPasswordEdit->setPlaceholderText("再次输入密码");
    ui->registerButton->setDefault(true);

    // 连接网络信号
    NetworkClient& client = NetworkClient::instance();
    connect(&client, &NetworkClient::connected, this, &RegisterDialog::onConnected);
    connect(&client, &NetworkClient::registerResult, this, &RegisterDialog::onRegisterResult);
    connect(&client, &NetworkClient::errorOccurred, this, &RegisterDialog::onNetworkError);
}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}

void RegisterDialog::on_registerButton_clicked()
{
    QString name = ui->nameEdit->text().trimmed();
    QString password = ui->passwordEdit->text();
    QString confirm = ui->confirmPasswordEdit->text();

    if (name.isEmpty() || password.isEmpty() || confirm.isEmpty()) {
        QMessageBox::warning(this, "提示", "昵称和密码不能为空");
        return;
    }

    if (password != confirm) {
        QMessageBox::warning(this, "提示", "两次输入的密码不一致");
        return;
    }

    if (password.length() < 6) {
        QMessageBox::warning(this, "提示", "密码长度不能少于 6 位");
        return;
    }

    m_pendingName = name;
    m_pendingPassword = password;
    connectAndRegister();
}

void RegisterDialog::on_cancelButton_clicked()
{
    reject();
}

void RegisterDialog::connectAndRegister()
{
    NetworkClient& client = NetworkClient::instance();

    if (client.isConnected()) {
        ui->registerButton->setText("注册中...");
        setUIEnabled(false);
        client.registerUser(m_pendingName, m_pendingPassword);
    } else {
        ui->registerButton->setText("连接中...");
        setUIEnabled(false);
        client.connectToServer("127.0.0.1", 8000);
    }
}

void RegisterDialog::onConnected()
{
    qDebug() << "[RegisterDialog] 服务器连接成功，发送注册请求";
    ui->registerButton->setText("注册中...");
    NetworkClient::instance().registerUser(m_pendingName, m_pendingPassword);
}

void RegisterDialog::onRegisterResult(bool success, const QString& message)
{
    ui->registerButton->setText("注册");
    setUIEnabled(true);

    if (success) {
        QMessageBox::information(this, "注册成功", message);
        accept();
    } else {
        QMessageBox::warning(this, "注册失败", message);
    }
}

void RegisterDialog::onNetworkError(const QString& error)
{
    ui->registerButton->setText("注册");
    setUIEnabled(true);
    QMessageBox::critical(this, "网络错误", QString("连接服务器失败：%1").arg(error));
}

void RegisterDialog::setUIEnabled(bool enabled)
{
    ui->nameEdit->setEnabled(enabled);
    ui->passwordEdit->setEnabled(enabled);
    ui->confirmPasswordEdit->setEnabled(enabled);
    ui->registerButton->setEnabled(enabled);
    ui->cancelButton->setEnabled(enabled);
}
