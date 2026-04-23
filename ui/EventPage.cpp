#include "EventPage.h"
#include "database/DatabaseManager.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QApplication> // 用于获取调色板

EventPage::EventPage(QWidget *parent) : QWidget(parent) {
    auto statsLayout = new QHBoxLayout;
    onlineLabel = new QLabel("在线: 0");
    offlineLabel = new QLabel("离线: 0");

    onlineLabel->setStyleSheet("color: #00b894; font-size: 16px; font-weight: bold; border: none; background: transparent;");
    offlineLabel->setStyleSheet("color: #7f8c8d; font-size: 16px; font-weight: bold; border: none; background: transparent;");

    auto layout = new QVBoxLayout(this);
    QLabel *titleLabel = new QLabel("事件列表");
    titleLabel->setStyleSheet(R"(
        font-size: 24px;
        font-weight: bold;
        color: white;
        border: none;
        background: transparent;
        margin-bottom: 4px;
    )");

    statsLayout->addWidget(titleLabel);
    // statsLayout->addStretch();
    // statsLayout->addWidget(onlineLabel);
    // statsLayout->addSpacing(20);
    // statsLayout->addWidget(offlineLabel);

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

    layout->addWidget(table);
}

void EventPage::dataUpdated(){
    QMap<quint16, std::shared_ptr<IconShoe>> shoeMap = DeviceManager::instance()->getShoeMap();
    if (shoeMap.isEmpty()) return;

    // updateStatistics(shoeMap);
    updateFromDeviceManager(shoeMap);

    for (int col = 0; col < table->columnCount(); ++col) {
        table->resizeColumnToContents(col);
        if (table->columnWidth(col) > 180) {
            table->setColumnWidth(col, 180);
        }
    }
}

void EventPage::updateFromDeviceManager(const QMap<quint16, std::shared_ptr<IconShoe>>& shoeMap) {
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


        // === 使用新的 StatusItem ===
        // auto shoeData = shoe->GetShoeData();
        // delete table->takeItem(row, columnIndex(Column::Status));
        // table->setItem(row, columnIndex(Column::Status), new StatusItem(shoeData.byOnline));

        // QString posStr = QString("经:%1, 纬:%2")
        //                      .arg(shoeData.lng, 0, 'f', 6)
        //                      .arg(shoeData.lat, 0, 'f', 6);
        // table->item(row, columnIndex(Column::Position))->setText(posStr);

        // // === 电量列 ===
        // int batteryPercentage = static_cast<int>(shoeData.byBatVal);
        // // int batteryPercentage = qBound(0, static_cast<int>((shoeData.byBatVal * 100) / 255.0f), 100);
        // table->item(row, columnIndex(Column::BatVal))->setText(QString::number(batteryPercentage));

        // table->item(row, columnIndex(Column::PosQuality))->setText(EnumtoString(shoeData.byPosQuality));
        // table->item(row, columnIndex(Column::StarNum))->setText(QString::number(shoeData.byStarNum));
        // table->item(row, columnIndex(Column::CabinetId))->setText(QString::number(shoe->GetCabinetID()));

        row++;
    }
}

void EventPage::updateStatistics(const QMap<quint16, std::shared_ptr<IconShoe>>& shoeMap) {
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

