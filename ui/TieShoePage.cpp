#include "TieShoePage.h"
#include "database/DatabaseManager.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>

TieShoePage::TieShoePage(QWidget *parent) : QWidget(parent) {
    auto layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("铁鞋信息"));

    // 初始化表头
    table = new QTableWidget(0, static_cast<int>(Column::Count));
    QStringList headers;
    for (int i = 0; i < static_cast<int>(Column::Count); ++i) {
        headers << columnHeader(static_cast<Column>(i));
    }
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setDefaultSectionSize(38); // 设置默认行高为 38px
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);
    layout->addWidget(table);
}

void TieShoePage::updateFromDeviceManager(const QMap<quint16, std::shared_ptr<IconShoe>>& shoeMap)
{
    const int targetRowCount = shoeMap.size();
    const int currentRowCount = table->rowCount();

    // 调整行数（只在必要时）
    if (targetRowCount != currentRowCount) {
        table->setRowCount(targetRowCount);
        // 初始化新单元格（防止 setText 崩溃）
        for (int row = 0; row < targetRowCount; ++row) {
            for (int col = 0; col < static_cast<int>(Column::Count); ++col) {
                if (!table->item(row, col)) {
                    table->setItem(row, col, new QTableWidgetItem());
                    table->item(row, col)->setTextAlignment(Qt::AlignCenter);
                }
            }
        }
    }

    // 按固定顺序（如 shoe_id 升序）填充数据 ===
    int row = 0;
    for (auto it = shoeMap.constBegin(); it != shoeMap.constEnd(); ++it) {
        auto shoe = it.value();
        if (!shoe) continue;

        // 更新第 row 行的内容
        quint16 shoeId = it.key();
        table->item(row, columnIndex(Column::ShoeId))->setText(QString::number(shoeId));

        auto shoeData = shoe->GetShoeData();
        table->item(row, columnIndex(Column::Status))->setText(EnumtoString(shoeData.byOnline));
        if (shoeData.byOnline == DeviceStatus::Unregister){
            table->item(row, columnIndex(Column::Status))->setForeground(Qt::red);
        } else if (shoeData.byOnline == DeviceStatus::Online){
            table->item(row, columnIndex(Column::Status))->setForeground(Qt::green);
        } else if (shoeData.byOnline == DeviceStatus::Offline){
            table->item(row, columnIndex(Column::Status))->setForeground(Qt::gray);
        } else {
            table->item(row, columnIndex(Column::Status))->setForeground(Qt::black);
        }

        // 格式化位置（经纬度）
        QString posStr = QString("经:%1, 纬:%2")
                             .arg(shoeData.lng, 0, 'f', 6)
                             .arg(shoeData.lat, 0, 'f', 6);
        table->item(row, columnIndex(Column::Position))->setText(posStr);

        table->item(row, columnIndex(Column::BatVal))->setText(QString::number(shoeData.byBatVal));
        table->item(row, columnIndex(Column::PosQuality))->setText(EnumtoString(shoeData.byPosQuality));
        table->item(row, columnIndex(Column::StarNum))->setText(QString::number(shoeData.byStarNum));
        table->item(row, columnIndex(Column::CabinetId))->setText(QString::number(shoe->GetCabinetID()));
        table->item(row, columnIndex(Column::StoreIdx))->setText(QString::number(shoe->GetStoreID()));

        row++;
    }
}
