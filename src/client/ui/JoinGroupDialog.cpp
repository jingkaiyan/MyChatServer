#include "JoinGroupDialog.h"
#include <QMessageBox>
#include <QFont>

JoinGroupDialog::JoinGroupDialog(QWidget *parent)
    : QDialog(parent)
    , m_groupId(-1)
{
    setWindowTitle("加入群组");
    setFixedSize(420, 220);
    
    // 创建控件
    QLabel *tipLabel = new QLabel("请输入群组ID：", this);
    m_groupIdEdit = new QLineEdit(this);
    m_groupIdEdit->setPlaceholderText("输入群组ID");
    
    m_joinButton = new QPushButton("加入", this);
    m_cancelButton = new QPushButton("取消", this);

    QFont titleFont;
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    tipLabel->setFont(titleFont);

    QFont inputFont;
    inputFont.setPointSize(16);
    inputFont.setBold(true);
    m_groupIdEdit->setFont(inputFont);
    m_groupIdEdit->setStyleSheet("QLineEdit { color: #111111; background: #ffffff; border: 1px solid #9ca3af; padding: 6px; }");
    m_groupIdEdit->setMinimumHeight(46);

    QFont buttonFont;
    buttonFont.setPointSize(12);
    buttonFont.setBold(true);
    m_joinButton->setFont(buttonFont);
    m_cancelButton->setFont(buttonFont);
    m_joinButton->setMinimumHeight(40);
    m_cancelButton->setMinimumHeight(40);
    
    // 布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tipLabel);
    mainLayout->addWidget(m_groupIdEdit);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_joinButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // 连接信号
    connect(m_joinButton, &QPushButton::clicked, this, &JoinGroupDialog::onJoinClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

JoinGroupDialog::~JoinGroupDialog()
{
}

int JoinGroupDialog::getGroupId() const
{
    return m_groupId;
}

void JoinGroupDialog::onJoinClicked()
{
    QString idStr = m_groupIdEdit->text().trimmed();
    if (idStr.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请输入群组ID");
        return;
    }
    
    bool ok;
    m_groupId = idStr.toInt(&ok);
    if (!ok || m_groupId <= 0)
    {
        QMessageBox::warning(this, "提示", "请输入有效的群组ID（正整数）");
        return;
    }
    
    accept();
}
