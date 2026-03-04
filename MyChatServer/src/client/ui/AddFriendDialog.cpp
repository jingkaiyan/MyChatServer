#include "AddFriendDialog.h"
#include <QMessageBox>
#include <QFont>

AddFriendDialog::AddFriendDialog(QWidget *parent)
    : QDialog(parent)
    , m_friendId(-1)
{
    setWindowTitle("添加好友");
    setFixedSize(420, 220);
    
    // 创建控件
    QLabel *tipLabel = new QLabel("请输入好友的用户ID：", this);
    m_friendIdEdit = new QLineEdit(this);
    m_friendIdEdit->setPlaceholderText("输入用户ID");
    
    m_addButton = new QPushButton("添加", this);
    m_cancelButton = new QPushButton("取消", this);

    QFont titleFont;
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    tipLabel->setFont(titleFont);

    QFont inputFont;
    inputFont.setPointSize(16);
    inputFont.setBold(true);
    m_friendIdEdit->setFont(inputFont);
    m_friendIdEdit->setStyleSheet("QLineEdit { color: #111111; background: #ffffff; border: 1px solid #9ca3af; padding: 6px; }");
    m_friendIdEdit->setMinimumHeight(46);

    QFont buttonFont;
    buttonFont.setPointSize(12);
    buttonFont.setBold(true);
    m_addButton->setFont(buttonFont);
    m_cancelButton->setFont(buttonFont);
    m_addButton->setMinimumHeight(40);
    m_cancelButton->setMinimumHeight(40);
    
    // 布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tipLabel);
    mainLayout->addWidget(m_friendIdEdit);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // 连接信号
    connect(m_addButton, &QPushButton::clicked, this, &AddFriendDialog::onAddClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

AddFriendDialog::~AddFriendDialog()
{
}

int AddFriendDialog::getFriendId() const
{
    return m_friendId;
}

void AddFriendDialog::onAddClicked()
{
    QString idStr = m_friendIdEdit->text().trimmed();
    if (idStr.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请输入好友ID");
        return;
    }
    
    bool ok;
    m_friendId = idStr.toInt(&ok);
    if (!ok || m_friendId <= 0)
    {
        QMessageBox::warning(this, "提示", "请输入有效的用户ID（正整数）");
        return;
    }
    
    accept();
}
