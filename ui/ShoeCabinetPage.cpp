#include "ShoeCabinetPage.h"
#include "database/DatabaseManager.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>

ShoeCabinetPage::ShoeCabinetPage(QWidget *parent) : QWidget(parent) {
    auto layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("鞋柜信息"));

    table = new QTableWidget(0, 4);
    table->setHorizontalHeaderLabels({"ID", "柜编号", "位置", "容量"});
    table->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(table);

    // reloadData();
}

void ShoeCabinetPage::reloadData() {
    qDebug() << "=============== ShoeCabinetPage::reloadData ====================";
    emit getShoeCabinetData();
    qDebug() << "===================================";

    // auto rows = DatabaseManager::getShoeCabinets();
    // table->setRowCount(rows.size());
    // for (int i = 0; i < rows.size(); ++i) {
    //     table->setItem(i, 0, new QTableWidgetItem(QString::number(rows[i]["id"].toInt())));
    //     table->setItem(i, 1, new QTableWidgetItem(rows[i]["cabinet_id"].toString()));
    //     table->setItem(i, 2, new QTableWidgetItem(rows[i]["position"].toString()));
    //     table->setItem(i, 3, new QTableWidgetItem(QString::number(rows[i]["capacity"].toInt())));
    //     for (int j = 0; j < 4; ++j)
    //         if (table->item(i, j))
    //             table->item(i, j)->setTextAlignment(Qt::AlignCenter);
    // }
}

void ShoeCabinetPage::handleIncomingShoeCabinetData(const QList<QVariantMap>& data)
{
    table->setRowCount(data.size());
    for (int i = 0; i < data.size(); ++i) {
        table->setItem(i, 0, new QTableWidgetItem(QString::number(data[i]["id"].toInt())));
        table->setItem(i, 1, new QTableWidgetItem(data[i]["cabinet_id"].toString()));
        table->setItem(i, 2, new QTableWidgetItem(data[i]["position"].toString()));
        table->setItem(i, 3, new QTableWidgetItem(QString::number(data[i]["capacity"].toInt())));
        for (int j = 0; j < 4; ++j)
            if (table->item(i, j))
                table->item(i, j)->setTextAlignment(Qt::AlignCenter);
    }
}
