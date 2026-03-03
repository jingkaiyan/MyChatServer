#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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
    // 发送按钮点击槽函数
    void on_sendButton_clicked();
    void on_friendItem_clicked(QListWidgetItem *item);
    void on_groupItem_clicked(QListWidgetItem *item);
    void on_contactSearch_textChanged(const QString &text);

private:
    void initializeContactLists();
    void appendSystemTip(const QString &text);
    void updateContactStats();

    Ui::MainWindow *ui; // UI指针
    QString currentChatTarget;
};

#endif // MAINWINDOW_H
