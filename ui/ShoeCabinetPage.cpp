#include "ShoeCabinetPage.h"
#include "database/DatabaseManager.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QDialog>
#include <QTableWidgetItem>

ShoeCabinetPage::ShoeCabinetPage(QWidget *parent) : QWidget(parent) {
    auto layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("鞋柜信息"));

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

void ShoeCabinetPage::updateFromDeviceManager(
    const QMap<quint16, std::shared_ptr<ShoeCabinet>>& cabinetMap)
{
    const int targetRowCount = cabinetMap.size();
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

    // 按固定顺序（如 cabinet_id 升序）填充数据 ===
    int row = 0;
    for (auto it = cabinetMap.constBegin(); it != cabinetMap.constEnd(); ++it) {
        auto cabinet = it.value();
        if (!cabinet) continue;

         // 更新第 row 行的内容
        quint16 cabinetId = it.key();
        table->item(row, columnIndex(Column::CabinetId))->setText(QString::number(cabinetId));

        auto cabinetData = cabinet->GetCabinetData();
        table->item(row, columnIndex(Column::Status))->setText(EnumtoString(cabinetData.byOnline));
        if (cabinetData.byOnline == CabinetStatus::Unregister){
            table->item(row, columnIndex(Column::Status))->setForeground(Qt::red);
        } else if (cabinetData.byOnline == CabinetStatus::Online){
            table->item(row, columnIndex(Column::Status))->setForeground(Qt::green);
        } else if (cabinetData.byOnline == CabinetStatus::Offline){
            table->item(row, columnIndex(Column::Status))->setForeground(Qt::gray);
        } else {
            table->item(row, columnIndex(Column::Status))->setForeground(Qt::black);
        }

        // 格式化位置（经纬度）
        auto pos = cabinet->GetPos();
        QString posStr = QString("经:%1, 纬:%2")
                             .arg(pos.x(), 0, 'f', 6)
                             .arg(pos.y(), 0, 'f', 6);
        table->item(row, columnIndex(Column::Position))->setText(posStr);
        table->item(row, columnIndex(Column::StoreNum))->setText(QString::number(cabinet->GetStoreNum()));

        // === 设置按钮 ===
        QPushButton* btn = qobject_cast<QPushButton*>(table->cellWidget(row, columnIndex(Column::DetailsButton)));
        if (!btn) {
            btn = new QPushButton("查看");
            connect(btn, &QPushButton::clicked, this, [this, cabinet]() {
                if (!cabinet) return;
                auto data = cabinet->GetCabinetData();
                showShoeDetailsDialog(cabinet->GetStoreNum(), cabinet->GetCabinetID(), data.abyStatus, cabinet->GetStoreShoeID());
            });
            table->setCellWidget(row, columnIndex(Column::DetailsButton), btn);
        }
        row++;
    }
}

void ShoeCabinetPage::showShoeDetailsDialog(quint8 storeNum, quint16 cabinetId, const QByteArray& statusArray,
                                            const QMap<quint8, quint16>& shoeIds)
{
    QDialog dialog(this);
    dialog.setWindowTitle(QString("鞋柜 %1 仓位详情").arg(cabinetId));
    dialog.resize(500, 400);

    auto layout = new QVBoxLayout(&dialog);
    auto detailTable = new QTableWidget();
    layout->addWidget(detailTable);

    // 设置子表列
    detailTable->setColumnCount(3);
    detailTable->setHorizontalHeaderLabels({"仓位编号", "状态", "绑定铁鞋ID"});
    detailTable->horizontalHeader()->setStretchLastSection(true);

    auto store_shoe_id = shoeIds;

    detailTable->setRowCount(storeNum);
    int status_idx = 0;
    for (int i = 0; i < storeNum; ++i) {
        int row = i;
        detailTable->setItem(row, 0, new QTableWidgetItem(QString::number(i + 1))); // 仓位从1开始
        detailTable->item(row, 0)->setTextAlignment(Qt::AlignCenter);

        StorageStatus status = StorageStatus::Unregister;
        if (status_idx<statusArray.size() && store_shoe_id.contains(i+1)){
            status = static_cast<StorageStatus>(statusArray.at(status_idx++));
        }
        detailTable->setItem(row, 1, new QTableWidgetItem(EnumtoString(status)));
        if (status == StorageStatus::Unregister){
            detailTable->item(row, 1)->setForeground(Qt::red);
        } else if (status == StorageStatus::Online){
            detailTable->item(row, 1)->setForeground(Qt::green);
        } else if (status == StorageStatus::Offline){
            detailTable->item(row, 1)->setForeground(Qt::gray);
        } else {
            detailTable->item(row, 1)->setForeground(Qt::black);
        }
        detailTable->item(row, 1)->setTextAlignment(Qt::AlignCenter);

        if (store_shoe_id.contains(i+1)) {
            detailTable->setItem(row, 2, new QTableWidgetItem(QString::number(store_shoe_id[i+1])));
        } else {
            detailTable->setItem(row, 2, new QTableWidgetItem("无"));
        }
        detailTable->item(row, 2)->setTextAlignment(Qt::AlignCenter);
    }

    dialog.exec(); // 模态对话框
}
