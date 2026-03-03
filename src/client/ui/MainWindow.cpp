#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QMessageBox>
#include <QDateTime>
#include <QListWidgetItem>

// 构造函数，初始化UI
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->messageEdit->setPlaceholderText("输入消息，按 Enter 可快速发送...");
    connect(ui->messageEdit, &QLineEdit::returnPressed, ui->sendButton, &QPushButton::click);
    connect(ui->friendListWidget, &QListWidget::itemClicked, this, &MainWindow::on_friendItem_clicked);
    connect(ui->groupListWidget, &QListWidget::itemClicked, this, &MainWindow::on_groupItem_clicked);
    connect(ui->contactSearchEdit, &QLineEdit::textChanged, this, &MainWindow::on_contactSearch_textChanged);

    initializeContactLists();

    ui->chatBrowser->setOpenExternalLinks(true);
    appendSystemTip("欢迎使用 MyChat，已加载好友和群组列表 ✨");
}

// 析构函数，释放UI资源
MainWindow::~MainWindow()
{
    delete ui;
}

// 发送按钮点击事件处理
void MainWindow::on_sendButton_clicked()
{
    QString msg = ui->messageEdit->text();
    if(msg.isEmpty()) {
        QMessageBox::warning(this, "提示", "消息不能为空");
        return;
    }

    if (currentChatTarget.isEmpty()) {
        QMessageBox::information(this, "提示", "请先从左侧选择一个好友或群组");
        return;
    }

    // TODO: 调用底层发送逻辑
    QString now = QDateTime::currentDateTime().toString("hh:mm:ss");
    ui->chatBrowser->append(QString("<b style='color:#60a5fa;'>我</b> → <span style='color:#fbbf24;'>%1</span> <span style='color:#94a3b8;'>[%2]</span>: %3")
                           .arg(currentChatTarget, now, msg.toHtmlEscaped()));
    ui->messageEdit->clear();
}

void MainWindow::on_friendItem_clicked(QListWidgetItem *item)
{
    if (item == nullptr) {
        return;
    }
    currentChatTarget = item->text();
    ui->currentChatLabel->setText(QString("当前会话：好友 %1").arg(currentChatTarget));
    appendSystemTip(QString("已切换到好友会话：%1").arg(currentChatTarget));
}

void MainWindow::on_groupItem_clicked(QListWidgetItem *item)
{
    if (item == nullptr) {
        return;
    }
    currentChatTarget = item->text();
    ui->currentChatLabel->setText(QString("当前会话：群组 %1").arg(currentChatTarget));
    appendSystemTip(QString("已切换到群组会话：%1").arg(currentChatTarget));
}

void MainWindow::initializeContactLists()
{
    const QStringList friendNames = {
        "Alice (在线)",
        "Bob (离线)",
        "Charlie (在线)",
        "Diana (忙碌)"
    };

    const QStringList groupNames = {
        "C++ 学习群 (23)",
        "项目讨论组 (8)",
        "MyChat 测试群 (15)"
    };

    ui->friendListWidget->addItems(friendNames);
    ui->groupListWidget->addItems(groupNames);

    updateContactStats();

    if (ui->friendListWidget->count() > 0) {
        ui->friendListWidget->setCurrentRow(0);
        on_friendItem_clicked(ui->friendListWidget->item(0));
    }
}

void MainWindow::appendSystemTip(const QString &text)
{
    QString now = QDateTime::currentDateTime().toString("hh:mm:ss");
    ui->chatBrowser->append(QString("<span style='color:#38bdf8;'>系统</span> <span style='color:#94a3b8;'>[%1]</span>：%2")
                           .arg(now, text.toHtmlEscaped()));
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
