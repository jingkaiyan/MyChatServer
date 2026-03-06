#include "CreateGroupDialog.h"
#include <QMessageBox>
#include <QFont>

CreateGroupDialog::CreateGroupDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("创建群组");
    setFixedSize(460, 320);
    
    // 创建控件
    QLabel *nameLabel = new QLabel("群组名称：", this);
    m_groupNameEdit = new QLineEdit(this);
    m_groupNameEdit->setPlaceholderText("输入群组名称");
    
    QLabel *descLabel = new QLabel("群组描述：", this);
    m_groupDescEdit = new QTextEdit(this);
    m_groupDescEdit->setPlaceholderText("输入群组描述（可选）");
    m_groupDescEdit->setMaximumHeight(80);
    
    m_createButton = new QPushButton("创建", this);
    m_cancelButton = new QPushButton("取消", this);

    QFont labelFont;
    labelFont.setPointSize(12);
    labelFont.setBold(true);
    nameLabel->setFont(labelFont);
    descLabel->setFont(labelFont);

    QFont inputFont;
    inputFont.setPointSize(14);
    inputFont.setBold(true);
    m_groupNameEdit->setFont(inputFont);
    m_groupNameEdit->setStyleSheet("QLineEdit { color: #111111; background: #ffffff; border: 1px solid #9ca3af; padding: 6px; }");
    m_groupNameEdit->setMinimumHeight(44);

    QFont textFont;
    textFont.setPointSize(12);
    m_groupDescEdit->setFont(textFont);
    m_groupDescEdit->setStyleSheet("QTextEdit { color: #111111; background: #ffffff; border: 1px solid #9ca3af; padding: 6px; }");
    m_groupDescEdit->setMinimumHeight(110);

    QFont buttonFont;
    buttonFont.setPointSize(12);
    buttonFont.setBold(true);
    m_createButton->setFont(buttonFont);
    m_cancelButton->setFont(buttonFont);
    m_createButton->setMinimumHeight(40);
    m_cancelButton->setMinimumHeight(40);
    
    // 布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(nameLabel);
    mainLayout->addWidget(m_groupNameEdit);
    mainLayout->addWidget(descLabel);
    mainLayout->addWidget(m_groupDescEdit);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_createButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // 连接信号
    connect(m_createButton, &QPushButton::clicked, this, &CreateGroupDialog::onCreateClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

CreateGroupDialog::~CreateGroupDialog()
{
}

QString CreateGroupDialog::getGroupName() const
{
    return m_groupName;
}

QString CreateGroupDialog::getGroupDesc() const
{
    return m_groupDesc;
}

void CreateGroupDialog::onCreateClicked()
{
    m_groupName = m_groupNameEdit->text().trimmed();
    if (m_groupName.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请输入群组名称");
        return;
    }
    
    m_groupDesc = m_groupDescEdit->toPlainText().trimmed();
    if (m_groupDesc.isEmpty())
    {
        m_groupDesc = "暂无描述";
    }
    
    accept();
}
