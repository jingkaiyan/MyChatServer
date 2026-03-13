#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>

namespace Ui {
class RegisterDialog;
}

// 注册对话框类，负责新用户注册界面
class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr);
    ~RegisterDialog();

private slots:
    void on_registerButton_clicked();
    void on_cancelButton_clicked();

    // 网络回调
    void onConnected();
    void onRegisterResult(bool success, const QString& message);
    void onNetworkError(const QString& error);

private:
    void connectAndRegister();
    void setUIEnabled(bool enabled);

private:
    Ui::RegisterDialog *ui;
    QString m_pendingName;
    QString m_pendingPassword;
};

#endif // REGISTERDIALOG_H
