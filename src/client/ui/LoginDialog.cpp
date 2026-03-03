#include "LoginDialog.h"
#include "ui_LoginDialog.h"
#include <QMessageBox>

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    ui->userIdEdit->setPlaceholderText("请输入账号 / 手机号");
    ui->passwordEdit->setPlaceholderText("请输入登录密码");
    ui->loginButton->setDefault(true);
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
    accept(); // 关闭对话框并返回成功
}
