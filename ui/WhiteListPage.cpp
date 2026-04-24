// WhiteListPage.cpp

#include "WhiteListPage.h"
#include "utils.h"
#include <QGroupBox>
#include <QFormLayout>
#include <QFileDialog>

WhiteListPage::WhiteListPage(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    // refreshTable(); // 初始化空表
}

void WhiteListPage::getAllWhitelist() {
    emit getWhitelist(0);
}

void WhiteListPage::setupUI()
{
    setStyleSheet("background-color: #0f172a; color: #cbd5e1;");

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // ===== 标题 =====
    auto title = new QLabel("白名单配置");
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: white;");
    auto subtitle = new QLabel("管理有权开启智能鞋柜授权人员名单");
    subtitle->setStyleSheet("font-size: 12px; color: #94a3b8;");
    mainLayout->addWidget(title);
    mainLayout->addWidget(subtitle);

    // ===== 主内容区：左右分栏 =====
    auto contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(20);


    // --- 左侧：表格容器（不变）---
    auto tableContainer = new QWidget();
    tableContainer->setObjectName("tableContainer");
    tableContainer->setStyleSheet("#tableContainer { background: rgba(15, 23, 42, 0.7); border: 1px solid rgba(56, 189, 248, 0.1); border-radius: 12px; }");
    auto tableLayout = new QVBoxLayout(tableContainer);
    tableLayout->setContentsMargins(0, 0, 0, 0);

    // 表头（保持不变）
    auto headerWidget = new QWidget();
    headerWidget->setStyleSheet("background: #1e293b;  border-top-left-radius: 12px; border-top-right-radius: 12px;");
    auto headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(16, 12, 16, 12);

    QLabel *logo = new QLabel;
    logo->setPixmap(coloredSvg(":/icon/whitelist_icon.svg", QColor("#38BDF8"), 36, 36));
    logo->setAttribute(Qt::WA_TranslucentBackground);
    headerLayout->addWidget(logo);

    auto hTitle = new QLabel("生效名单管理");
    hTitle->setStyleSheet("font-weight: bold; color: white; font-size: 16px;");
    headerLayout->addWidget(hTitle);
    headerLayout->addStretch();

    // auto searchEdit = new QLineEdit();
    // searchEdit->setPlaceholderText("快速查找姓名/授权ID");
    // searchEdit->setFixedWidth(200);
    // searchEdit->setStyleSheet(
    //     "background: #1e293b; border: 1px solid #334155; border-radius: 6px; "
    //     "padding: 6px 10px; color: white;"
    //     );
    // headerLayout->addWidget(searchEdit);
    tableLayout->addWidget(headerWidget);

    // 表格（保持不变）
    m_table = new QTableWidget();
    m_table->setColumnCount(Columns::index(Column::Count));
    QStringList headers;
    for (int i = 0; i < Columns::index(Column::Count); ++i) {
        headers << Columns::header(static_cast<Column>(i));
    }
    m_table->setHorizontalHeaderLabels(headers);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setDefaultSectionSize(40);
    tableLayout->addWidget(m_table);

    // 页脚（保持不变）
    auto footer = new QLabel("共 0 名授权人员处于生效状态");
    footer->setObjectName("footerLabel");
    footer->setStyleSheet("#footerLabel { color: #64748b; font-size: 12px; margin-top: 8px; text-align: right; }");
    footer->setAlignment(Qt::AlignRight);
    tableLayout->addWidget(footer);

    contentLayout->addWidget(tableContainer, 1); // 左侧占主要空间

    // --- 右侧：垂直面板（原 formContainer + globalBox）---
    auto rightPanel = new QWidget();
    rightPanel->setObjectName("rightPanel");
    rightPanel->setStyleSheet("#rightPanel { background: transparent; }"); // 透明，不额外加背景
    auto rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(20);

    // >>> 原 formContainer（白名单添加表单）<<<
    auto formContainer = new QWidget();
    formContainer->setObjectName("formContainer");
    formContainer->setStyleSheet("#formContainer { background: rgba(15, 23, 42, 0.7); border: 1px solid rgba(56, 189, 248, 0.1); border-radius: 12px; }");
    formContainer->setMinimumWidth(320);
    formContainer->setMaximumWidth(360);
    auto formLayout = new QVBoxLayout(formContainer);
    formLayout->setContentsMargins(20, 20, 20, 20);
    formLayout->setSpacing(16);

    auto titleLayout = new QHBoxLayout();
    QLabel *add_user = new QLabel;
    add_user->setPixmap(coloredSvg(":/icon/add_user.svg", QColor("#38BDF8"), 36, 36));
    add_user->setAttribute(Qt::WA_TranslucentBackground);
    titleLayout->addWidget(add_user);

    auto formTitle = new QLabel("添加白名单人员");
    formTitle->setStyleSheet("font-weight: bold; font-size: 16px; color: white;");
    titleLayout->addWidget(formTitle);
    titleLayout->addStretch();
    formLayout->addLayout(titleLayout);

    const int LABEL_WIDTH = 80;
    // 姓名
    auto nameLayout = new QHBoxLayout();
    auto nameLabel = new QLabel("员工姓名");
    nameLabel->setFixedWidth(LABEL_WIDTH);
    nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    nameLayout->addWidget(nameLabel);
    m_lineName = new QLineEdit();
    m_lineName->setPlaceholderText("输入姓名");
    m_lineName->setStyleSheet(
        "background: #1e293b; border: 1px solid #334155; border-radius: 6px; "
        "padding: 8px 10px; color: white;"
        );
    nameLayout->addWidget(m_lineName);
    nameLayout->addStretch();
    formLayout->addLayout(nameLayout);

    // 授权ID
    auto uidLayout = new QHBoxLayout();
    auto uidLabel = new QLabel("授权ID");
    uidLabel->setFixedWidth(LABEL_WIDTH);
    uidLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    uidLayout->addWidget(uidLabel);
    m_lineUid = new QLineEdit();
    m_lineUid->setPlaceholderText("输入数字ID");
    m_lineUid->setInputMethodHints(Qt::ImhDigitsOnly);
    m_lineUid->setStyleSheet(
        "QLineEdit {"
        "background: #1e293b; "
        "border: 1px solid #334155; "
        "border-radius: 6px; "
        "padding: 8px 10px; "
        "color: white;"
        "}"
        "QLineEdit:disabled {"
        "background: #334155; "          // 更浅/更暗的灰色，表示禁用
        "border: 1px solid #475569; "    // 边框也变淡
        "color: #94a3b8; "               // 文字变灰
        "opacity: 0.8;"                  // 可选：降低不透明度
        "}"
        );
    uidLayout->addWidget(m_lineUid);
    uidLayout->addStretch();
    formLayout->addLayout(uidLayout);

    // 所属部门
    auto deptLayout = new QHBoxLayout();
    auto deptLabel = new QLabel("所属部门");
    deptLabel->setFixedWidth(LABEL_WIDTH);
    deptLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    deptLayout->addWidget(deptLabel);
    m_lineDept = new QLineEdit();
    m_lineDept->setPlaceholderText("输入部门");
    m_lineDept->setStyleSheet(
        "background: #1e293b; border: 1px solid #334155; border-radius: 6px; "
        "padding: 8px 10px; color: white;"
        );
    deptLayout->addWidget(m_lineDept);
    deptLayout->addStretch();
    formLayout->addLayout(deptLayout);

    // 按钮
    m_btnAdd = new QPushButton("提交并生效");
    m_btnAdd->setStyleSheet(
        "QPushButton {"
        "   background-color: #0ea5e9; color: white; border: none; border-radius: 6px;"
        "   padding: 8px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #38bdf8; }"
        );
    connect(m_btnAdd, &QPushButton::clicked, this, &WhiteListPage::onAddClicked);

    m_btnReset = new QPushButton("重置输入");
    m_btnReset->setStyleSheet(
        "QPushButton {"
        "   background: transparent; color: #94a3b8; border: 1px solid #334155;"
        "   border-radius: 6px; padding: 6px; font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: rgba(56, 189, 248, 0.1); color: #cbd5e1; }"
        );
    connect(m_btnReset, &QPushButton::clicked, [this]() {
        m_lineUid->setEnabled(true);
        m_lineName->clear();
        m_lineUid->clear();
        m_lineDept->clear();
        m_editingUidStr.clear();
        m_btnAdd->setText("提交并生效");
    });

    formLayout->addWidget(m_btnAdd);
    formLayout->addWidget(m_btnReset);
    // formLayout->addStretch();

    rightLayout->addWidget(formContainer);
    rightLayout->addStretch();

    // 将右侧面板加入主内容区
    contentLayout->addWidget(rightPanel, 0); // 右侧不伸展，按内容大小

    // 添加主内容区到主布局
    mainLayout->addLayout(contentLayout);
}

void WhiteListPage::populateTable(const WhitelistMap &entries)
{
    m_table->setRowCount(static_cast<int>(entries.size()));
    int row = 0;
    for (auto it = entries.cbegin(); it != entries.cend(); ++it) {
        quint32 uid = it.key();
        const WhitelistEntry &e = it.value();

        // 授权ID
        auto itemUid = new QTableWidgetItem(QString::number(uid));
        itemUid->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_table->setItem(row, Columns::index(Column::Uid), itemUid);

        // 姓名
        auto itemName = new QTableWidgetItem(e.name);
        // itemName->setFont(QFont("", -1, QFont::Bold));
        itemName->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_table->setItem(row, Columns::index(Column::Name), itemName);

        // 部门
        auto itemDept = new QTableWidgetItem(e.department);
        itemDept->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_table->setItem(row, Columns::index(Column::Department), itemDept);

        // 操作按钮
        // 创建一个容器（必须，因为要放两个按钮）
        auto container = new QWidget();
        container->setMinimumSize(52, 28); // ⭐ 关键：宽度=24+24+4(spacing)+4(margins)
        container->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed); // 高度固定

        auto hlayout = new QHBoxLayout(container);
        hlayout->setContentsMargins(2, 0, 2, 0);
        hlayout->setSpacing(4);
        hlayout->setAlignment(Qt::AlignCenter);

        // 替换文字按钮为图标按钮（更符合 UI 美观和空间限制）
        auto btnEdit = new QPushButton();
        btnEdit->setFixedSize(24, 24);
        btnEdit->setIcon(QIcon(coloredSvg(":/icon/edit.svg", QColor("#38BDF8"), 16, 16))); // 假设有 edit 图标
        btnEdit->setIconSize(QSize(16, 16));
        btnEdit->setStyleSheet(
            "QPushButton {"
            "   background-color: rgba(56, 189, 248, 0.2);"
            "   border: 1px solid rgba(56, 189, 248, 0.5);"
            "   border-radius: 4px;"
            "}"
            "QPushButton:hover {"
            "   background-color: rgba(56, 189, 248, 0.4);"
            "}"
            );

        auto btnDel = new QPushButton();
        btnDel->setFixedSize(24, 24);
        btnDel->setIcon(QIcon(coloredSvg(":/icon/delete.svg", QColor("#ef4444"), 16, 16)));
        btnDel->setIconSize(QSize(16, 16));
        btnDel->setStyleSheet(
            "QPushButton {"
            "   background-color: rgba(239, 68, 68, 0.2);"
            "   border: 1px solid rgba(239, 68, 68, 0.5);"
            "   border-radius: 4px;"
            "}"
            "QPushButton:hover {"
            "   background-color: rgba(239, 68, 68, 0.4);"
            "}"
            );

        connect(btnEdit, &QPushButton::clicked, [this, row]() { onEditClicked(row); });
        connect(btnDel, &QPushButton::clicked, [this, row]() { onDeleteClicked(row); });

        hlayout->addWidget(btnEdit);
        hlayout->addWidget(btnDel);

        m_table->setCellWidget(row, Columns::index(Column::Actions), container);

        qDebug() << "Row" << row << "Container size:" << container->size()
                 << "Layout size hint:" << hlayout->sizeHint();
        ++row;
    }
    m_table->viewport()->update();      // 立即重绘
    m_table->repaint();                 // 备用

    // 更新页脚计数
    auto footer = findChild<QLabel*>("footerLabel");
    if (footer) {
        footer->setText(QString("共 %1 名授权人员处于生效状态").arg(entries.size()));
    }
}

void WhiteListPage::refreshTable(const WhitelistMap &entries)
{
    m_whitelist = entries;
    populateTable(m_whitelist);
}

void WhiteListPage::onAddClicked()
{
    QString uidStr = m_lineUid->text().trimmed();
    QString name = m_lineName->text().trimmed();
    QString dept = m_lineDept->text().trimmed();

    if (uidStr.isEmpty() || name.isEmpty() || dept.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请填写授权ID、姓名和部门！");
        return;
    }

    bool ok = false;
    quint64 val = uidStr.toULongLong(&ok);
    if (!ok || val > 4294967295ULL) {
        QMessageBox::warning(this, "输入错误", "授权ID必须是 0 ~ 4294967295 之间的整数！");
        return;
    }
    quint32 uid = static_cast<quint32>(val);

    if (m_editingUidStr.isEmpty()) {
        // 新增
        if (m_whitelist.contains(uid)) {
            QMessageBox::warning(this, "重复ID", "该授权ID已存在！");
            return;
        }
        WhitelistEntry entry{uid, name, dept};
        m_whitelist[uid] = entry;
        emit entryAdded(m_whitelist[uid]);
        // QMessageBox::information(this, "成功", "人员已添加！");
    } else {
        // 编辑
        bool wasFound = false;
        for (auto it = m_whitelist.begin(); it != m_whitelist.end(); ++it) {
            if (QString::number(it.key()) == m_editingUidStr) {
                it.value().name = name;
                it.value().department = dept;
                emit entryUpdated(it.value());
                wasFound = true;
                break;
            }
        }
        if (!wasFound) {
            QMessageBox::critical(this, "错误", "编辑失败：原始记录不存在");
            return;
        }
        // QMessageBox::information(this, "成功", "信息已更新！");
        m_editingUidStr.clear();
        m_btnAdd->setText("提交并生效");
    }

    // refreshTable(m_whitelist);
    m_lineName->clear();
    m_lineUid->clear();
    m_lineDept->clear();
    m_lineUid->setEnabled(true);
}

void WhiteListPage::onEditClicked(int row)
{
    if (row < 0 || row >= m_table->rowCount()) return;

    QString uidStr = m_table->item(row, Columns::index(Column::Uid))->text();
    QString name = m_table->item(row, Columns::index(Column::Name))->text();
    QString dept = m_table->item(row, Columns::index(Column::Department))->text();

    m_lineUid->setText(uidStr);
    m_lineUid->setEnabled(false);
    m_lineName->setText(name);
    m_lineDept->setText(dept);

    m_editingUidStr = uidStr;
    m_btnAdd->setText("更新信息");
}

void WhiteListPage::onDeleteClicked(int row)
{
    if (row < 0 || row >= m_table->rowCount()) return;
    QString uidStr = m_table->item(row, Columns::index(Column::Uid))->text();
    quint32 uid = uidStr.toUInt();

    if (QMessageBox::question(this, "确认删除",
                              QString("确定要删除人员 %1 吗？").arg(uidStr)) != QMessageBox::Yes)
        return;

    m_whitelist.remove(uid);
    emit entryRemoved(uid);
    // refreshTable(m_whitelist);
    // QMessageBox::information(this, "成功", "已删除。");
}

void WhiteListPage::handleOperateResult(bool ok, const QString& msg){
    qDebug() << ok << " " << msg;
    QMessageBox::warning(this, ok ? "成功" : "失败", msg);

    emit getWhitelist(0);
}

void WhiteListPage::onSaveGlobalConfig()
{
    // 预留接口：此处可调用数据库保存
    QMessageBox::information(this, "提示", "全局配置保存成功（内存模拟）");
}
