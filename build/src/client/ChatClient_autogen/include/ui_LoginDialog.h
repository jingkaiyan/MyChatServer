/********************************************************************************
** Form generated from reading UI file 'LoginDialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOGINDIALOG_H
#define UI_LOGINDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_LoginDialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *titleLabel;
    QFormLayout *formLayout;
    QLabel *labelUserId;
    QLineEdit *userIdEdit;
    QLabel *labelPassword;
    QLineEdit *passwordEdit;
    QPushButton *loginButton;

    void setupUi(QDialog *LoginDialog)
    {
        if (LoginDialog->objectName().isEmpty())
            LoginDialog->setObjectName(QString::fromUtf8("LoginDialog"));
        LoginDialog->resize(420, 280);
        LoginDialog->setStyleSheet(QString::fromUtf8("QDialog {\n"
"    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,\n"
"        stop:0 #1f2937, stop:1 #111827);\n"
"}\n"
"QLabel {\n"
"    color: #e5e7eb;\n"
"    font-size: 14px;\n"
"}\n"
"QLabel#titleLabel {\n"
"    color: #f9fafb;\n"
"    font-size: 22px;\n"
"    font-weight: 700;\n"
"}\n"
"QLineEdit {\n"
"    color: #f9fafb;\n"
"    background: rgba(255,255,255,0.08);\n"
"    border: 1px solid rgba(255,255,255,0.25);\n"
"    border-radius: 10px;\n"
"    padding: 8px 10px;\n"
"}\n"
"QLineEdit:focus {\n"
"    border: 1px solid #60a5fa;\n"
"}\n"
"QPushButton#loginButton {\n"
"    color: #ffffff;\n"
"    background: #2563eb;\n"
"    border: none;\n"
"    border-radius: 12px;\n"
"    font-size: 15px;\n"
"    font-weight: 600;\n"
"    min-height: 40px;\n"
"}\n"
"QPushButton#loginButton:hover { background: #3b82f6; }\n"
"QPushButton#loginButton:pressed { background: #1d4ed8; }"));
        verticalLayout = new QVBoxLayout(LoginDialog);
        verticalLayout->setSpacing(16);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(28, 22, 28, 22);
        titleLabel = new QLabel(LoginDialog);
        titleLabel->setObjectName(QString::fromUtf8("titleLabel"));
        titleLabel->setAlignment(Qt::AlignCenter);

        verticalLayout->addWidget(titleLabel);

        formLayout = new QFormLayout();
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        formLayout->setHorizontalSpacing(12);
        formLayout->setVerticalSpacing(12);
        labelUserId = new QLabel(LoginDialog);
        labelUserId->setObjectName(QString::fromUtf8("labelUserId"));

        formLayout->setWidget(0, QFormLayout::LabelRole, labelUserId);

        userIdEdit = new QLineEdit(LoginDialog);
        userIdEdit->setObjectName(QString::fromUtf8("userIdEdit"));

        formLayout->setWidget(0, QFormLayout::FieldRole, userIdEdit);

        labelPassword = new QLabel(LoginDialog);
        labelPassword->setObjectName(QString::fromUtf8("labelPassword"));

        formLayout->setWidget(1, QFormLayout::LabelRole, labelPassword);

        passwordEdit = new QLineEdit(LoginDialog);
        passwordEdit->setObjectName(QString::fromUtf8("passwordEdit"));
        passwordEdit->setEchoMode(QLineEdit::Password);

        formLayout->setWidget(1, QFormLayout::FieldRole, passwordEdit);


        verticalLayout->addLayout(formLayout);

        loginButton = new QPushButton(LoginDialog);
        loginButton->setObjectName(QString::fromUtf8("loginButton"));

        verticalLayout->addWidget(loginButton);


        retranslateUi(LoginDialog);

        QMetaObject::connectSlotsByName(LoginDialog);
    } // setupUi

    void retranslateUi(QDialog *LoginDialog)
    {
        LoginDialog->setWindowTitle(QApplication::translate("LoginDialog", "\347\231\273\345\275\225 - MyChat", nullptr));
        titleLabel->setText(QApplication::translate("LoginDialog", "\346\254\242\350\277\216\345\233\236\346\235\245", nullptr));
        labelUserId->setText(QApplication::translate("LoginDialog", "\350\264\246\345\217\267", nullptr));
        labelPassword->setText(QApplication::translate("LoginDialog", "\345\257\206\347\240\201", nullptr));
        loginButton->setText(QApplication::translate("LoginDialog", "\350\277\233\345\205\245\350\201\212\345\244\251", nullptr));
    } // retranslateUi

};

namespace Ui {
    class LoginDialog: public Ui_LoginDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOGINDIALOG_H
