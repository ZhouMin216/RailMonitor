// LoginDialog.cpp
#include "LoginDialog.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QMessageBox>
#include <QPainter>
#include <QApplication>
#include <QScreen>
#include <QKeyEvent>
#include <QGuiApplication>
#include <QIcon>
#include <QLabel>
#include <QDate>
#include "utils.h"

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("石家庄车辆段铁鞋管理系统 - 登录");
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setModal(true);
    setFocusPolicy(Qt::StrongFocus);

    // 全屏：覆盖整个主屏幕
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        setGeometry(screen->geometry());
    }

    // === 创建 UI 元素 ===

    // 铁路 SVG 图标
    QLabel *railIcon = new QLabel;
    railIcon->setPixmap(coloredSvg(":/icon/logo.svg", QColor("#38BDF8"), 48,48));
    railIcon->setAttribute(Qt::WA_TranslucentBackground); // ← 透明背景
    // railIcon->setAutoFillBackground(false);


    // 标题
    QLabel *titleLabel = new QLabel("石家庄车辆段铁鞋管理系统");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(R"(
        font-size: 24px;
        font-weight: bold;
        color: white;
        border: none;               /* ← 显式声明无边框 */
        background: transparent;    /* ← 确保背景透明 */
        margin-bottom: 4px;
    )");

    QLabel *subTitleLabel = new QLabel("Shijiazhuang Vehicle Depot Iron Shoe Management System");
    subTitleLabel->setAlignment(Qt::AlignCenter);
    subTitleLabel->setStyleSheet(R"(
        font-size: 12px;
        color: #94a3b8;
        margin-bottom: 24px;
        border: none;
        background: transparent;
    )");

    // === 用户名输入区（带左侧 SVG 图标）===
    QWidget *userContainer = new QWidget;
    userContainer->setStyleSheet("background: #0f172a; border: 1px solid #334155; border-radius: 8px;");

    QLabel *userIcon = new QLabel;
    userIcon->setPixmap(coloredSvg(":/icon/user.svg", QColor("#94a3b8"), 20,20));
    userIcon->setAttribute(Qt::WA_TranslucentBackground); // ← 透明背景
    userIcon->setAutoFillBackground(false);

    usernameEdit = new QLineEdit;
    usernameEdit->setPlaceholderText("请输入管理员账号");
    usernameEdit->setStyleSheet("border: none; background: transparent; color: white; padding: 12px 0;");
    usernameEdit->setAttribute(Qt::WA_MacShowFocusRect, false);

    QHBoxLayout *userLayout = new QHBoxLayout(userContainer);
    userLayout->setContentsMargins(8, 0, 8, 0);
    userLayout->setSpacing(8);
    userLayout->addWidget(userIcon);
    userLayout->addWidget(usernameEdit);

    // === 密码输入区（带左侧 SVG 图标）===
    QWidget *pwdContainer = new QWidget;
    pwdContainer->setStyleSheet("background: #0f172a; border: 1px solid #334155; border-radius: 8px;");

    QLabel *lockIcon = new QLabel;
    lockIcon->setPixmap(coloredSvg(":/icon/password.svg", QColor("#94a3b8"), 20, 20));
    lockIcon->setAttribute(Qt::WA_TranslucentBackground);
    lockIcon->setAutoFillBackground(false);

    passwordEdit = new QLineEdit;
    passwordEdit->setPlaceholderText("请输入访问密码");
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setStyleSheet("border: none; background: transparent; color: white; padding: 12px 0;");
    passwordEdit->setAttribute(Qt::WA_MacShowFocusRect, false);

    QHBoxLayout *pwdLayout = new QHBoxLayout(pwdContainer);
    pwdLayout->setContentsMargins(8, 0, 8, 0);
    pwdLayout->setSpacing(8);
    pwdLayout->addWidget(lockIcon);
    pwdLayout->addWidget(passwordEdit);

    // 登录按钮
    loginButton = new QPushButton("进入管理中心");
    loginButton->setStyleSheet(R"(
        QPushButton {
            background: #0ea5e9;
            color: white;
            font-weight: bold;
            border: none;
            border-radius: 8px;
            padding: 12px;
            margin-top: 8px;
        }
        QPushButton:hover {
            background: #0284c7;
        }
        QPushButton:pressed {
            background: #036496;
        }
    )");
    connect(loginButton, &QPushButton::clicked, this, &LoginDialog::onAccepted);

    // 表单区域
    QVBoxLayout *formLayout = new QVBoxLayout;
    formLayout->setSpacing(16);
    formLayout->addWidget(userContainer);   // ← 使用带图标的容器
    formLayout->addWidget(pwdContainer);    // ← 使用带图标的容器
    formLayout->addWidget(loginButton);

    // 版权信息
    QString currentDate = QDate::currentDate().toString("yyyy-MM-dd");
    QLabel *copyright = new QLabel(QString("System Date: %1").arg(currentDate));
    copyright->setAlignment(Qt::AlignCenter);
    copyright->setStyleSheet(R"(font-size: 10px; color: #64748b; margin-top: 24px; border: none; background: transparent;)");

    // === 登录卡片（毛玻璃效果）===
    QFrame *card = new QFrame;
    card->setStyleSheet(R"(
        QFrame {
            background: rgba(15, 23, 42, 0.85);
            border: 1px solid rgba(56, 189, 248, 0.25);
            border-radius: 16px;
        }
    )");

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(32, 32, 32, 24);
    cardLayout->setSpacing(16);
    cardLayout->addWidget(railIcon, 0, Qt::AlignCenter);
    cardLayout->addWidget(titleLabel);
    cardLayout->addWidget(subTitleLabel);
    cardLayout->addLayout(formLayout);
    cardLayout->addWidget(copyright);

    // === 主布局：居中卡片 ===
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(card, 0, Qt::AlignCenter);

    // 聚焦到用户名
    usernameEdit->setFocus();
}

void LoginDialog::onAccepted()
{
    if (usernameEdit->text().trimmed().isEmpty() || passwordEdit->text().isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入用户名和密码。");
        return;
    }
    // 验证逻辑（示例：简单判断）
    if (username() != "admin" || password() != "123456") {
        QMessageBox::warning(this, "登录失败", "用户名或密码错误！");
        return ;
    }
    emit loginSuccess();
    // accept();
}

QString LoginDialog::username() const { return usernameEdit->text().trimmed(); }
QString LoginDialog::password() const { return passwordEdit->text(); }

// === 绘制全屏背景 ===
void LoginDialog::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 获取窗口中心点
    QRect rect = this->rect();
    QPoint center = rect.center();

    // 创建径向渐变：从中心开始，向外扩散
    QRadialGradient gradient(center, rect.width() / 2.0); // 半径为窗口一半
    gradient.setColorAt(0.0, QColor(15, 23, 42));         // 中心：深蓝 #0f172a
    gradient.setColorAt(0.3, QColor(15, 23, 42));         // 过渡区
    gradient.setColorAt(0.6, QColor(18, 30, 60));         // 略亮一点（#121e3c）
    gradient.setColorAt(0.9, QColor(2, 6, 23));           // 边缘：接近黑色 #020617
    gradient.setColorAt(1.0, QColor(2, 6, 23));

    // 填充整个窗口
    painter.fillRect(rect, gradient);
    QDialog::paintEvent(event);
}

// === 支持 ESC 退出 ===
void LoginDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        // reject();
        QApplication::quit();
    } else {
        QDialog::keyPressEvent(event);
    }
}
