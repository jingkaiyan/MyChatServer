#ifndef JOINGROUPDIALOG_H
#define JOINGROUPDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class JoinGroupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit JoinGroupDialog(QWidget *parent = nullptr);
    ~JoinGroupDialog();
    
    int getGroupId() const;

private slots:
    void onJoinClicked();

private:
    QLineEdit *m_groupIdEdit;
    QPushButton *m_joinButton;
    QPushButton *m_cancelButton;
    int m_groupId;
};

#endif // JOINGROUPDIALOG_H
