#ifndef ADDFRIENDDIALOG_H
#define ADDFRIENDDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class AddFriendDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddFriendDialog(QWidget *parent = nullptr);
    ~AddFriendDialog();
    
    int getFriendId() const;

private slots:
    void onAddClicked();

private:
    QLineEdit *m_friendIdEdit;
    QPushButton *m_addButton;
    QPushButton *m_cancelButton;
    int m_friendId;
};

#endif // ADDFRIENDDIALOG_H
