#include "TieShoePage.h"
#include "database/DatabaseManager.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>

TieShoePage::TieShoePage(QWidget *parent) : QWidget(parent) {
    auto layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("铁鞋信息"));

    table = new QTableWidget(0, 4);
    table->setHorizontalHeaderLabels({"ID", "鞋编号", "位置", "状态"});
    table->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(table);

    reloadData();
}

void TieShoePage::reloadData() {
    auto rows = DatabaseManager::getTieShoes();
    table->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        table->setItem(i, 0, new QTableWidgetItem(QString::number(rows[i]["id"].toInt())));
        table->setItem(i, 1, new QTableWidgetItem(rows[i]["shoe_id"].toString()));
        table->setItem(i, 2, new QTableWidgetItem(rows[i]["location"].toString()));
        table->setItem(i, 3, new QTableWidgetItem(rows[i]["status"].toString()));
        for (int j = 0; j < 4; ++j)
            if (table->item(i, j))
                table->item(i, j)->setTextAlignment(Qt::AlignCenter);
    }
}
