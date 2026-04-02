#include "TieShoePage.h"
#include "database/DatabaseManager.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QApplication> // 用于获取调色板

// ========================
// StatusItem 实现 (新)
// ========================
StatusItem::StatusItem(ShoeStatus status)
    : QTableWidgetItem(), m_status(status), m_statusText(EnumtoString(status)) {
    setFlags(flags() & ~Qt::ItemIsEditable);
    setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter); // 文字左对齐，垂直居中
}

QVariant StatusItem::data(int role) const {
    if (role == Qt::DisplayRole) {
        // 返回状态文字
        return m_statusText;
    } else if (role == Qt::DecorationRole) {
        // 返回状态图标
        QColor color;
        switch (m_status) {
        case ShoeStatus::Online:      color = QColor("#00ffcc"); break;
        case ShoeStatus::Offline:     color = QColor("#ffd166"); break;
        case ShoeStatus::Unregister:  color = QColor("#ff6b6b"); break;
        case ShoeStatus::InCabinet:   color = QColor("#4a6aff"); break;
        default:                      color = Qt::gray;
        }
        return createCirclePixmap(color);
    }
    // 对于其他角色，回退到父类实现
    return QTableWidgetItem::data(role);
}

QPixmap StatusItem::createCirclePixmap(const QColor& color) const {
    // 创建一个清晰、无光晕的圆点
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen); // 无边框
    painter.setBrush(color);
    // 绘制一个紧贴边界的圆，不留额外空白
    painter.drawEllipse(0, 0, 15, 15);
    return pixmap;
}

// ========================
// BatteryBarDelegate 实现 (修改)
// ========================
void BatteryBarDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    bool ok = false;
    int batteryValue = index.data(Qt::DisplayRole).toInt(&ok);
    if (!ok || batteryValue < 0) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    batteryValue = qBound(0, batteryValue, 100);

    // 根据电量确定进度条颜色
    QColor barColor;
    if (batteryValue > 60) {
        barColor = QColor("#00ffcc");
    } else if (batteryValue > 20) {
        barColor = QColor("#ffd166");
    } else {
        barColor = QColor("#ff6b6b");
    }

    // 1. 绘制背景
    QRect backgroundRect = option.rect;
    backgroundRect.adjust(4, 4, -4, -4);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor("#2d2d44"));
    painter->drawRoundedRect(backgroundRect, 3, 3);

    // 2. 计算并绘制进度条
    int barWidth = static_cast<int>((backgroundRect.width() * batteryValue) / 100.0);
    QRect barRect = backgroundRect;
    barRect.setWidth(barWidth);
    painter->setBrush(barColor);
    painter->drawRoundedRect(barRect, 3, 3);

    // 3. 绘制文字 - 关键修改：动态设置文字颜色
    QString text = QString("%1%").arg(batteryValue);
    painter->setFont(option.font);

    // 根据进度条颜色选择高对比度的文字颜色
    QColor textColor;
    if (barColor == QColor("#ff6b6b")) { // 低电量 (红)
        textColor = Qt::white; // 白色文字在红色上最清晰
    } else { // 中/高电量 (黄/青绿)
        textColor = Qt::black; // 深色文字在亮色背景上更清晰
    }

    // 将文字绘制在整个背景区域的中心
    painter->setPen(textColor);
    painter->drawText(backgroundRect, Qt::AlignCenter, text);
}

QSize BatteryBarDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    return QSize(100, 30);
}

// ========================
// TieShoePage 实现 (其余部分保持不变)
// ========================
TieShoePage::TieShoePage(QWidget *parent) : QWidget(parent) {
    auto statsLayout = new QHBoxLayout;
    onlineLabel = new QLabel("在线: 0");
    offlineLabel = new QLabel("离线: 0");

    onlineLabel->setStyleSheet("color: #00b894; font-size: 16px; font-weight: bold; border: none; background: transparent;");
    offlineLabel->setStyleSheet("color: #7f8c8d; font-size: 16px; font-weight: bold; border: none; background: transparent;");

    auto layout = new QVBoxLayout(this);
    QLabel *titleLabel = new QLabel("终端铁鞋状态");
    titleLabel->setStyleSheet(R"(
        font-size: 24px;
        font-weight: bold;
        color: white;
        border: none;
        background: transparent;
        margin-bottom: 4px;
    )");

    statsLayout->addWidget(titleLabel);
    statsLayout->addStretch();
    statsLayout->addWidget(onlineLabel);
    statsLayout->addSpacing(20);
    statsLayout->addWidget(offlineLabel);

    layout->addLayout(statsLayout);

    table = new QTableWidget(0, static_cast<int>(Column::Count));
    QStringList headers;
    for (int i = 0; i < static_cast<int>(Column::Count); ++i) {
        headers << columnHeader(static_cast<Column>(i));
    }
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setDefaultSectionSize(38);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);

    table->setStyleSheet(R"(
        QTableWidget {
            background-color: #1e1e2e;
            gridline-color: #2d2d44;
            color: #e0e0ff;
            border: none;
            font-size: 13px;
        }
        QHeaderView::section {
            background-color: #252535;
            color: #a0a0c0;
            padding: 8px;
            border: none;
            font-weight: bold;
            font-size: 14px;
        }
        QTableWidget::item {
            padding: 8px;
            border-bottom: 1px solid #2d2d44;
        }
        QTableWidget::item:selected {
            background-color: #2a2a40;
        }
    )");

    batteryDelegate = new BatteryBarDelegate(this);
    table->setItemDelegateForColumn(columnIndex(Column::BatVal), batteryDelegate);

    layout->addWidget(table);
}

void TieShoePage::dataUpdated(){
    QMap<quint16, std::shared_ptr<IconShoe>> shoeMap = DeviceManager::instance()->getShoeMap();
    if (shoeMap.isEmpty()) return;

    updateStatistics(shoeMap);
    updateFromDeviceManager(shoeMap);

    table->horizontalHeader()->setSectionResizeMode(columnIndex(Column::Status), QHeaderView::Fixed);
    table->setColumnWidth(columnIndex(Column::Status), 120); // 增加宽度以容纳文字

    for (int col = 0; col < table->columnCount(); ++col) {
        if (col != columnIndex(Column::Status)) {
            table->resizeColumnToContents(col);
            if (table->columnWidth(col) > 180) {
                table->setColumnWidth(col, 180);
            }
        }
    }
}

void TieShoePage::updateFromDeviceManager(const QMap<quint16, std::shared_ptr<IconShoe>>& shoeMap) {
    const int targetRowCount = shoeMap.size();
    const int currentRowCount = table->rowCount();

    if (targetRowCount != currentRowCount) {
        table->setRowCount(targetRowCount);
        for (int row = 0; row < targetRowCount; ++row) {
            for (int col = 0; col < static_cast<int>(Column::Count); ++col) {
                if (!table->item(row, col)) {
                    table->setItem(row, col, new QTableWidgetItem());
                    table->item(row, col)->setTextAlignment(Qt::AlignCenter);
                }
            }
        }
    }

    int row = 0;
    for (auto it = shoeMap.constBegin(); it != shoeMap.constEnd(); ++it) {
        auto shoe = it.value();
        if (!shoe) continue;

        quint16 shoeId = it.key();
        table->item(row, columnIndex(Column::ShoeId))->setText(QString::number(shoeId));
        table->item(row, columnIndex(Column::PaintedId))->setText(DeviceManager::instance()->getPaintedID(shoeId));

        // === 使用新的 StatusItem ===
        auto shoeData = shoe->GetShoeData();
        delete table->takeItem(row, columnIndex(Column::Status));
        table->setItem(row, columnIndex(Column::Status), new StatusItem(shoeData.byOnline));

        QString posStr = QString("经:%1, 纬:%2")
                             .arg(shoeData.lng, 0, 'f', 6)
                             .arg(shoeData.lat, 0, 'f', 6);
        table->item(row, columnIndex(Column::Position))->setText(posStr);

        // === 电量列 ===
        int batteryPercentage = static_cast<int>(shoeData.byBatVal);
        // int batteryPercentage = qBound(0, static_cast<int>((shoeData.byBatVal * 100) / 255.0f), 100);
        table->item(row, columnIndex(Column::BatVal))->setText(QString::number(batteryPercentage));

        table->item(row, columnIndex(Column::PosQuality))->setText(EnumtoString(shoeData.byPosQuality));
        table->item(row, columnIndex(Column::StarNum))->setText(QString::number(shoeData.byStarNum));
        table->item(row, columnIndex(Column::CabinetId))->setText(QString::number(shoe->GetCabinetID()));

        row++;
    }
}

void TieShoePage::updateStatistics(const QMap<quint16, std::shared_ptr<IconShoe>>& shoeMap) {
    int online = 0, offline = 0;
    for (const auto& shoe : shoeMap) {
        if (!shoe) continue;
        auto status = shoe->GetShoeData().byOnline;
        if (status == ShoeStatus::Online || status == ShoeStatus::InCabinet) online++;
        else if (status == ShoeStatus::Offline) offline++;
    }
    onlineLabel->setText(QString("在线: %1").arg(online));
    offlineLabel->setText(QString("离线: %1").arg(offline));
}
