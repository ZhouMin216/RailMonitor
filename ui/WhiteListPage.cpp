// WhiteListPage.cpp

#include "WhiteListPage.h"
#include "utils.h"
#include <QGroupBox>

WhiteListPage::WhiteListPage(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    refreshTable(); // 初始化空表
}

void WhiteListPage::setupUI()
{
    setStyleSheet("background-color: #0f172a; color: #cbd5e1;");

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // ===== 标题 =====
    auto title = new QLabel("员工准入白名单配置");
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: white;");
    auto subtitle = new QLabel("管理有权开启智能鞋柜授权人员名单");
    subtitle->setStyleSheet("font-size: 12px; color: #94a3b8;");
    mainLayout->addWidget(title);
    mainLayout->addWidget(subtitle);

    // ===== 主内容区：左右分栏 =====
    auto contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(20);

    // --- 左侧：表格容器 ---
    auto tableContainer = new QWidget();
    tableContainer->setObjectName("tableContainer");
    tableContainer->setStyleSheet("#tableContainer { background: rgba(15, 23, 42, 0.7); border: 1px solid rgba(56, 189, 248, 0.1); border-radius: 12px; }");
    auto tableLayout = new QVBoxLayout(tableContainer);
    tableLayout->setContentsMargins(0, 0, 0, 0);

    // 表头
    auto headerWidget = new QWidget();
    headerWidget->setStyleSheet("background: #1e293b;  border-top-left-radius: 12px; border-top-right-radius: 12px;");
    auto headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(16, 12, 16, 12);

    QLabel *logo = new QLabel;
    logo->setPixmap(coloredSvg(":/icon/whitelist_icon.svg", QColor("#38BDF8"), 36, 36));
    logo->setAttribute(Qt::WA_TranslucentBackground); // ← 透明背景
    // railIcon->setAutoFillBackground(false);
    headerLayout->addWidget(logo);

    auto hTitle = new QLabel("生效名单管理");
    hTitle->setStyleSheet("font-weight: bold; color: white; font-size: 16px;");
    headerLayout->addWidget(hTitle);
    headerLayout->addStretch();

    auto searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("快速查找姓名/授权ID");
    searchEdit->setFixedWidth(200);
    searchEdit->setStyleSheet(
        "background: #1e293b; border: 1px solid #334155; border-radius: 6px; "
        "padding: 6px 10px; color: white;"
        );
    headerLayout->addWidget(searchEdit);
    tableLayout->addWidget(headerWidget);

    // 表格
    m_table = new QTableWidget();
    m_table->setColumnCount(Columns::index(Column::Count));
    QStringList headers;
    for (int i = 0; i < Columns::index(Column::Count); ++i) {
        headers << Columns::header(static_cast<Column>(i));
    }
    m_table->setHorizontalHeaderLabels(headers);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "   background: #1e293b; color: #94a3b8; padding: 10px 12px;"
        "   border: none; border-bottom: 1px solid #334155;"
        "}"
        );
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // m_table->setStyleSheet(
    //     "QTableWidget { background: transparent; gridline-color: #1e293b; }"
    //     "QTableWidget::item { padding: 10px 12px; }"
    //     "QTableWidget::item:hover { background-color: rgba(56, 189, 248, 0.1); }"
    //     );
    m_table->setStyleSheet(
        "QTableWidget { background: transparent; gridline-color: #1e293b; }"
        // 移除 QTableWidget::item { padding: ... }
        "QTableWidget::item:hover { background-color: rgba(56, 189, 248, 0.1); }"
        );
    m_table->verticalHeader()->setDefaultSectionSize(40);
    tableLayout->addWidget(m_table);

    // 页脚
    auto footer = new QLabel("共 0 名授权人员处于生效状态");
    footer->setObjectName("footerLabel");
    footer->setStyleSheet("#footerLabel { color: #64748b; font-size: 12px; margin-top: 8px; text-align: right; }");
    footer->setAlignment(Qt::AlignRight);
    tableLayout->addWidget(footer);

    contentLayout->addWidget(tableContainer, 1);

    // --- 右侧：表单容器（与左侧等高）---
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
    add_user->setAttribute(Qt::WA_TranslucentBackground); // ← 透明背景
    titleLayout->addWidget(add_user);

    auto formTitle = new QLabel("添加白名单人员");
    formTitle->setStyleSheet("font-weight: bold; font-size: 16px; color: white;");
    titleLayout->addWidget(formTitle);
    titleLayout->addStretch();
    formLayout->addLayout(titleLayout);

    // 姓名
    const int LABEL_WIDTH = 80; // 根据您的UI调整这个值

    // --- 姓名 ---
    auto nameLayout = new QHBoxLayout();
    auto nameLabel = new QLabel("员工姓名");
    nameLabel->setFixedWidth(LABEL_WIDTH); // ← 设置固定宽度
    nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter); // ← 文本右对齐
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

    // --- 授权ID ---
    auto uidLayout = new QHBoxLayout();
    auto uidLabel = new QLabel("授权ID");
    uidLabel->setFixedWidth(LABEL_WIDTH); // ← 同样的固定宽度
    uidLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter); // ← 同样的对齐方式
    uidLayout->addWidget(uidLabel);

    m_lineUid = new QLineEdit();
    m_lineUid->setPlaceholderText("输入数字ID");
    // m_lineUid->setValidator(new QIntValidator(0, 4294967295, this));
    m_lineUid->setInputMethodHints(Qt::ImhDigitsOnly);
    m_lineUid->setStyleSheet(
        "background: #1e293b; border: 1px solid #334155; border-radius: 6px; "
        "padding: 8px 10px; color: white;"
        );
    uidLayout->addWidget(m_lineUid);
    uidLayout->addStretch();
    formLayout->addLayout(uidLayout);

    // --- 所属部门 ---
    auto deptLayout = new QHBoxLayout();
    auto deptLabel = new QLabel("所属部门");
    deptLabel->setFixedWidth(LABEL_WIDTH); // ← 同样的固定宽度
    deptLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter); // ← 同样的对齐方式
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
    // m_btnAdd->setIcon(QIcon(":/icons/delete.svg"));
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
        m_lineName->clear();
        m_lineUid->clear();
        m_lineDept->clear();
        m_editingUidStr.clear();
        m_btnAdd->setText("提交并生效");
    });

    formLayout->addWidget(m_btnAdd);
    formLayout->addWidget(m_btnReset);
    formLayout->addStretch(); // 推动按钮靠上，使表单与表格等高

    contentLayout->addWidget(formContainer);
    mainLayout->addLayout(contentLayout);

    // ===== 全局配置 =====
    auto globalBox = new QGroupBox("系统全局同步配置");
    globalBox->setStyleSheet(
        "QGroupBox {"
        "   border: 1px solid #334155; border-radius: 12px; padding: 16px;"
        "   background: rgba(15, 23, 42, 0.7);"
        "}"
        "QGroupBox::title {"
        "   subcontrol-origin: margin; left: 10px; padding: 0 4px;"
        "   color: #60a5fa; font-weight: bold;"
        "}"
        );
    auto globalLayout = new QVBoxLayout(globalBox);

    auto grid = new QGridLayout();
    grid->setVerticalSpacing(16);
    grid->setHorizontalSpacing(24);

    m_cbOfflineLock = new QCheckBox("离线自动锁定");
    m_cbOfflineLock->setChecked(true);
    m_cbOfflineLock->setStyleSheet("color: white;");
    grid->addWidget(m_cbOfflineLock, 0, 0);
    auto lbl1 = new QLabel("当鞋柜网络中断时，是否启动紧急物理锁定");
    lbl1->setStyleSheet("color: #64748b; font-size: 12px;");
    grid->addWidget(lbl1, 1, 0);

    m_cbAlertPush = new QCheckBox("告警外部触控推送");
    m_cbAlertPush->setChecked(true);
    m_cbAlertPush->setStyleSheet("color: white;");
    grid->addWidget(m_cbAlertPush, 0, 1);
    auto lbl2 = new QLabel("异常情况即时推送至责任人手机钉钉/企业微信");
    lbl2->setStyleSheet("color: #64748b; font-size: 12px;");
    grid->addWidget(lbl2, 1, 1);

    auto syncLabel = new QLabel("白名单全网同步频率");
    syncLabel->setStyleSheet("color: white; font-weight: medium;");
    grid->addWidget(syncLabel, 2, 0);
    auto freqLabel = new QLabel("每 10 分钟");
    freqLabel->setStyleSheet(
        "color: #38bdf8; background: rgba(56, 189, 248, 0.1); "
        "border: 1px solid rgba(56, 189, 248, 0.3); border-radius: 4px; "
        "padding: 2px 8px; font-size: 12px;"
        );
    grid->addWidget(freqLabel, 2, 1);

    m_cbAnonymize = new QCheckBox("数据存储脱敏");
    m_cbAnonymize->setChecked(false);
    m_cbAnonymize->setStyleSheet("color: white;");
    grid->addWidget(m_cbAnonymize, 3, 0);
    auto lbl3 = new QLabel("针对日志中的人员姓名进行部分掩码处理");
    lbl3->setStyleSheet("color: #64748b; font-size: 12px;");
    grid->addWidget(lbl3, 4, 0);

    globalLayout->addLayout(grid);

    auto btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    m_btnSaveGlobal = new QPushButton("保存全局配置");
    m_btnSaveGlobal->setStyleSheet(
        "QPushButton {"
        "   background-color: #10b981; color: white; border: none; border-radius: 6px;"
        "   padding: 8px 24px; font-weight: bold; min-width: 120px;"
        "}"
        "QPushButton:hover { background-color: #34d399; }"
        );
    connect(m_btnSaveGlobal, &QPushButton::clicked, this, &WhiteListPage::onSaveGlobalConfig);
    btnLayout->addWidget(m_btnSaveGlobal);
    globalLayout->addLayout(btnLayout);

    mainLayout->addWidget(globalBox);
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
        itemName->setFont(QFont("", -1, QFont::Bold));
        itemName->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_table->setItem(row, Columns::index(Column::Name), itemName);

        // 部门
        auto itemDept = new QTableWidgetItem(e.department);
        itemDept->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_table->setItem(row, Columns::index(Column::Department), itemDept);

        // 操作按钮
        auto actionsWidget = new QWidget();
        actionsWidget->setStyleSheet("background: transparent;"); // ⭐ 透明背景
        actionsWidget->setMinimumSize(64, 32);
        actionsWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        auto hlayout = new QHBoxLayout(actionsWidget);
        hlayout->setContentsMargins(4, 0, 4, 0);
        hlayout->setSpacing(8);
        hlayout->setAlignment(Qt::AlignCenter); // 居中

        auto btnEdit = new QPushButton("E");
        btnEdit->setFixedSize(24, 24);
        btnEdit->setStyleSheet(
            "QPushButton { background: transparent; border: none; color: #38bdf8; }"
            "QPushButton:hover { color: #60a5fa; }"
            );
        connect(btnEdit, &QPushButton::clicked, [this, row]() { onEditClicked(row); });

        auto btnDel = new QPushButton("D");
        btnDel->setFixedSize(24, 24);
        btnDel->setStyleSheet(
            "QPushButton { background: transparent; border: none; color: #f87171; }"
            "QPushButton:hover { color: #fca5a5; }"
            );
        connect(btnDel, &QPushButton::clicked, [this, row]() { onDeleteClicked(row); });

        hlayout->addWidget(btnEdit);
        hlayout->addWidget(btnDel);
        actionsWidget->adjustSize();
        m_table->setCellWidget(row, Columns::index(Column::Actions), actionsWidget);

        ++row;
    }

    // 更新页脚计数
    auto footer = findChild<QLabel*>("footerLabel");
    if (footer) {
        footer->setText(QString("共 %1 名授权人员处于生效状态").arg(entries.size()));
    }
}

void WhiteListPage::refreshTable()
{
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
        emit entryAdded(uid, name);
        QMessageBox::information(this, "成功", "人员已添加！");
    } else {
        // 编辑
        bool wasFound = false;
        for (auto it = m_whitelist.begin(); it != m_whitelist.end(); ++it) {
            if (QString::number(it.key()) == m_editingUidStr) {
                it.value().name = name;
                it.value().department = dept;
                emit entryUpdated(it.key(), name);
                wasFound = true;
                break;
            }
        }
        if (!wasFound) {
            QMessageBox::critical(this, "错误", "编辑失败：原始记录不存在");
            return;
        }
        QMessageBox::information(this, "成功", "信息已更新！");
        m_editingUidStr.clear();
        m_btnAdd->setText("提交并生效");
    }

    refreshTable();
    m_lineName->clear();
    m_lineUid->clear();
    m_lineDept->clear();
}

void WhiteListPage::onEditClicked(int row)
{
    if (row < 0 || row >= m_table->rowCount()) return;

    QString uidStr = m_table->item(row, Columns::index(Column::Uid))->text();
    QString name = m_table->item(row, Columns::index(Column::Name))->text();
    QString dept = m_table->item(row, Columns::index(Column::Department))->text();

    m_lineUid->setText(uidStr);
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
    refreshTable();
    QMessageBox::information(this, "成功", "已删除。");
}

void WhiteListPage::onSaveGlobalConfig()
{
    // 预留接口：此处可调用数据库保存
    QMessageBox::information(this, "提示", "全局配置保存成功（内存模拟）");
}
