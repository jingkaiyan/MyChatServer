

// Qt桌面应用入口，仅保留UI相关内容，适用于本地桌面环境
#include <QApplication>
#include "ui/MainWindow.h"
#include "ui/LoginDialog.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 先弹出登录对话框
    LoginDialog loginDlg;
    if (loginDlg.exec() == QDialog::Accepted) {
        // 登录成功后进入主界面
        MainWindow w;
        w.show();
        return app.exec();
    }
    // 登录取消或失败直接退出
    return 0;
}