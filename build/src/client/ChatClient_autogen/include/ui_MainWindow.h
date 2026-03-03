/********************************************************************************
** Form generated from reading UI file 'MainWindow.ui'
**
** Created by: Qt User Interface Compiler version 5.12.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QLabel *titleLabel;
    QLabel *subtitleLabel;
    QHBoxLayout *contentLayout;
    QFrame *sideCard;
    QVBoxLayout *verticalLayout_3;
    QLineEdit *contactSearchEdit;
    QLabel *contactStatsLabel;
    QTabWidget *contactTabs;
    QWidget *friendsTab;
    QVBoxLayout *verticalLayout_5;
    QListWidget *friendListWidget;
    QWidget *groupsTab;
    QVBoxLayout *verticalLayout_6;
    QListWidget *groupListWidget;
    QFrame *chatCard;
    QVBoxLayout *verticalLayout_2;
    QLabel *currentChatLabel;
    QTextBrowser *chatBrowser;
    QHBoxLayout *horizontalLayout;
    QLineEdit *messageEdit;
    QPushButton *sendButton;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(920, 640);
        MainWindow->setStyleSheet(QString::fromUtf8("QMainWindow {\n"
"    background: #0b1220;\n"
"}\n"
"QWidget#centralwidget {\n"
"    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,\n"
"        stop:0 #0f172a, stop:1 #111827);\n"
"}\n"
"QFrame#chatCard {\n"
"    background: rgba(17, 24, 39, 210);\n"
"    border: 1px solid rgba(148, 163, 184, 0.25);\n"
"    border-radius: 16px;\n"
"}\n"
"QFrame#sideCard {\n"
"    background: rgba(17, 24, 39, 190);\n"
"    border: 1px solid rgba(148, 163, 184, 0.25);\n"
"    border-radius: 16px;\n"
"}\n"
"QLabel#titleLabel {\n"
"    color: #f8fafc;\n"
"    font-size: 22px;\n"
"    font-weight: 700;\n"
"}\n"
"QLabel#subtitleLabel {\n"
"    color: #94a3b8;\n"
"    font-size: 13px;\n"
"}\n"
"QLabel#currentChatLabel {\n"
"    color: #cbd5e1;\n"
"    font-size: 13px;\n"
"    font-weight: 600;\n"
"}\n"
"QTextBrowser {\n"
"    color: #e2e8f0;\n"
"    background: rgba(2, 6, 23, 0.65);\n"
"    border: 1px solid rgba(148, 163, 184, 0.25);\n"
"    border-radius: 12px;\n"
"    padding: 10px;\n"
"    font-size: 13px;\n"
"}\n"
"QLineEdi"
                        "t {\n"
"    color: #f1f5f9;\n"
"    background: rgba(255, 255, 255, 0.08);\n"
"    border: 1px solid rgba(148, 163, 184, 0.35);\n"
"    border-radius: 11px;\n"
"    padding: 9px 12px;\n"
"    font-size: 13px;\n"
"}\n"
"QLineEdit:focus {\n"
"    border: 1px solid #60a5fa;\n"
"}\n"
"QTabWidget::pane {\n"
"    border: 1px solid rgba(148, 163, 184, 0.25);\n"
"    border-radius: 10px;\n"
"    background: rgba(2, 6, 23, 0.45);\n"
"}\n"
"QTabBar::tab {\n"
"    color: #94a3b8;\n"
"    padding: 7px 14px;\n"
"    border-top-left-radius: 8px;\n"
"    border-top-right-radius: 8px;\n"
"}\n"
"QTabBar::tab:selected {\n"
"    color: #e2e8f0;\n"
"    background: rgba(37, 99, 235, 0.25);\n"
"}\n"
"QListWidget {\n"
"    color: #e2e8f0;\n"
"    background: transparent;\n"
"    border: none;\n"
"    outline: none;\n"
"}\n"
"QListWidget::item {\n"
"    border-radius: 8px;\n"
"    padding: 8px;\n"
"    margin: 2px;\n"
"}\n"
"QListWidget::item:selected {\n"
"    background: rgba(37, 99, 235, 0.35);\n"
"}\n"
"QLabel#contactStatsLabel "
                        "{\n"
"    color: #94a3b8;\n"
"    font-size: 12px;\n"
"}\n"
"QPushButton#sendButton {\n"
"    color: #ffffff;\n"
"    background: #2563eb;\n"
"    border: none;\n"
"    border-radius: 11px;\n"
"    min-width: 110px;\n"
"    min-height: 40px;\n"
"    font-size: 14px;\n"
"    font-weight: 700;\n"
"}\n"
"QPushButton#sendButton:hover { background: #3b82f6; }\n"
"QPushButton#sendButton:pressed { background: #1d4ed8; }"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setSpacing(14);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(22, 18, 22, 18);
        titleLabel = new QLabel(centralwidget);
        titleLabel->setObjectName(QString::fromUtf8("titleLabel"));

        verticalLayout->addWidget(titleLabel);

        subtitleLabel = new QLabel(centralwidget);
        subtitleLabel->setObjectName(QString::fromUtf8("subtitleLabel"));

        verticalLayout->addWidget(subtitleLabel);

        contentLayout = new QHBoxLayout();
        contentLayout->setSpacing(12);
        contentLayout->setObjectName(QString::fromUtf8("contentLayout"));
        sideCard = new QFrame(centralwidget);
        sideCard->setObjectName(QString::fromUtf8("sideCard"));
        sideCard->setMinimumSize(QSize(260, 0));
        sideCard->setMaximumSize(QSize(320, 16777215));
        sideCard->setFrameShape(QFrame::StyledPanel);
        sideCard->setFrameShadow(QFrame::Raised);
        verticalLayout_3 = new QVBoxLayout(sideCard);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        verticalLayout_3->setContentsMargins(12, 12, 12, 12);
        contactSearchEdit = new QLineEdit(sideCard);
        contactSearchEdit->setObjectName(QString::fromUtf8("contactSearchEdit"));

        verticalLayout_3->addWidget(contactSearchEdit);

        contactStatsLabel = new QLabel(sideCard);
        contactStatsLabel->setObjectName(QString::fromUtf8("contactStatsLabel"));

        verticalLayout_3->addWidget(contactStatsLabel);

        contactTabs = new QTabWidget(sideCard);
        contactTabs->setObjectName(QString::fromUtf8("contactTabs"));
        friendsTab = new QWidget();
        friendsTab->setObjectName(QString::fromUtf8("friendsTab"));
        verticalLayout_5 = new QVBoxLayout(friendsTab);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        verticalLayout_5->setContentsMargins(8, 8, 8, 8);
        friendListWidget = new QListWidget(friendsTab);
        friendListWidget->setObjectName(QString::fromUtf8("friendListWidget"));

        verticalLayout_5->addWidget(friendListWidget);

        contactTabs->addTab(friendsTab, QString());
        groupsTab = new QWidget();
        groupsTab->setObjectName(QString::fromUtf8("groupsTab"));
        verticalLayout_6 = new QVBoxLayout(groupsTab);
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        verticalLayout_6->setContentsMargins(8, 8, 8, 8);
        groupListWidget = new QListWidget(groupsTab);
        groupListWidget->setObjectName(QString::fromUtf8("groupListWidget"));

        verticalLayout_6->addWidget(groupListWidget);

        contactTabs->addTab(groupsTab, QString());

        verticalLayout_3->addWidget(contactTabs);


        contentLayout->addWidget(sideCard);

        chatCard = new QFrame(centralwidget);
        chatCard->setObjectName(QString::fromUtf8("chatCard"));
        chatCard->setFrameShape(QFrame::StyledPanel);
        chatCard->setFrameShadow(QFrame::Raised);
        verticalLayout_2 = new QVBoxLayout(chatCard);
        verticalLayout_2->setSpacing(12);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(14, 14, 14, 14);
        currentChatLabel = new QLabel(chatCard);
        currentChatLabel->setObjectName(QString::fromUtf8("currentChatLabel"));

        verticalLayout_2->addWidget(currentChatLabel);

        chatBrowser = new QTextBrowser(chatCard);
        chatBrowser->setObjectName(QString::fromUtf8("chatBrowser"));

        verticalLayout_2->addWidget(chatBrowser);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(10);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        messageEdit = new QLineEdit(chatCard);
        messageEdit->setObjectName(QString::fromUtf8("messageEdit"));

        horizontalLayout->addWidget(messageEdit);

        sendButton = new QPushButton(chatCard);
        sendButton->setObjectName(QString::fromUtf8("sendButton"));

        horizontalLayout->addWidget(sendButton);


        verticalLayout_2->addLayout(horizontalLayout);


        contentLayout->addWidget(chatCard);


        verticalLayout->addLayout(contentLayout);

        MainWindow->setCentralWidget(centralwidget);

        retranslateUi(MainWindow);

        contactTabs->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MyChat Desktop", nullptr));
        titleLabel->setText(QApplication::translate("MainWindow", "MyChat \350\201\212\345\244\251\345\256\242\346\210\267\347\253\257", nullptr));
        subtitleLabel->setText(QApplication::translate("MainWindow", "\346\262\211\346\265\270\345\274\217\345\257\271\350\257\235\347\225\214\351\235\242 \302\267 \346\270\205\347\210\275\346\267\261\350\211\262\344\270\273\351\242\230", nullptr));
        contactSearchEdit->setPlaceholderText(QApplication::translate("MainWindow", "\346\220\234\347\264\242\345\245\275\345\217\213\346\210\226\347\276\244\347\273\204...", nullptr));
        contactStatsLabel->setText(QApplication::translate("MainWindow", "\345\217\257\350\247\201\345\245\275\345\217\213 0 \302\267 \345\217\257\350\247\201\347\276\244\347\273\204 0", nullptr));
        contactTabs->setTabText(contactTabs->indexOf(friendsTab), QApplication::translate("MainWindow", "\345\245\275\345\217\213", nullptr));
        contactTabs->setTabText(contactTabs->indexOf(groupsTab), QApplication::translate("MainWindow", "\347\276\244\347\273\204", nullptr));
        currentChatLabel->setText(QApplication::translate("MainWindow", "\345\275\223\345\211\215\344\274\232\350\257\235\357\274\232\346\234\252\351\200\211\346\213\251\350\201\224\347\263\273\344\272\272", nullptr));
        sendButton->setText(QApplication::translate("MainWindow", "\345\217\221\351\200\201", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
