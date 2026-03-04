#ifndef CREATEGROUPDIALOG_H
#define CREATEGROUPDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class CreateGroupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateGroupDialog(QWidget *parent = nullptr);
    ~CreateGroupDialog();
    
    QString getGroupName() const;
    QString getGroupDesc() const;

private slots:
    void onCreateClicked();

private:
    QLineEdit *m_groupNameEdit;
    QTextEdit *m_groupDescEdit;
    QPushButton *m_createButton;
    QPushButton *m_cancelButton;
    QString m_groupName;
    QString m_groupDesc;
};

#endif // CREATEGROUPDIALOG_H
