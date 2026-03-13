#include "NetworkConfigPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QIntValidator>

NetworkConfigPage::NetworkConfigPage(QWidget *parent)
    : QWidget(parent) {

    ipEdit = new QLineEdit("127.0.0.1");
    portEdit = new QLineEdit("8888");
    portEdit->setValidator(new QIntValidator(1, 65535, this));
    connectBtn = new QPushButton("连接服务器");
    disconnectBtn = new QPushButton("断开连接");
    statusLabel = new QLabel("连接状态：未连接");

    // 设置输入框宽度一致（可选）
    ipEdit->setMinimumWidth(150);
    portEdit->setMinimumWidth(150);

    // 使用 QFormLayout 实现标签-字段对齐
    QFormLayout *formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow); // 输入框可拉伸
    formLayout->setFormAlignment(Qt::AlignCenter); // 整个表单居中（可选）
    formLayout->setLabelAlignment(Qt::AlignRight); // 标签右对齐（关键！）
    formLayout->addRow("IP：", ipEdit);
    formLayout->addRow("端口：", portEdit);

    // 按钮布局
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addWidget(connectBtn);
    btnLayout->addSpacing(20);
    btnLayout->addWidget(disconnectBtn);
    btnLayout->setAlignment(Qt::AlignCenter); // 按钮整体居中

    // 状态标签
    statusLabel->setAlignment(Qt::AlignCenter);

    // 主布局：垂直堆叠
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 60, 40, 40);
    mainLayout->addStretch();
    mainLayout->addLayout(formLayout);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(btnLayout);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(statusLabel);
    mainLayout->addStretch();

    // 居中整个内容（通过外层布局）
    QWidget *container = new QWidget;
    container->setLayout(mainLayout);
    container->setMaximumWidth(400);

    QVBoxLayout *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addStretch();
    outer->addWidget(container, 0, Qt::AlignCenter);
    outer->addStretch();

    // 连接信号
    connect(connectBtn, &QPushButton::clicked, this, [this]() {
        emit connectRequested(ipEdit->text(), portEdit->text().toInt());
    });
    connect(disconnectBtn, &QPushButton::clicked, this, &NetworkConfigPage::disconnectRequested);
    this->setStyleSheet(R"(
        NetworkConfigPage {
            background-color: transparent; /* 保持透明，让主窗口背景透出 */
        }
        QLabel {
            font-size: 18px;
            color: #333;
        }
        QLineEdit {
        font-size: 18px;
            padding: 6px 10px;
            border: 1px solid #aaa;
            border-radius: 4px;
            background: white;
            min-width: 120px;
        }
        QPushButton {
            padding: 6px 16px;
            border: none;
            border-radius: 4px;
            background: #5B9BD5;
            color: white;
            font-weight: bold;
        }
        QPushButton:hover {
            background: #3C7AB8;
        }
        QPushButton:pressed {
            background: #2A5F9E;
        }
    )");


}

QString NetworkConfigPage::getIP() const {
    return ipEdit->text();
}

int NetworkConfigPage::getPort() const {
    return portEdit->text().toInt();
}
