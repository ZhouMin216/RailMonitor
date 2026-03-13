#pragma once
#include <QWidget>
#include <QTableWidget>

class ShoeCabinetPage : public QWidget {
    Q_OBJECT
public:
    ShoeCabinetPage(QWidget *parent = nullptr);
    void reloadData();
private:
    QTableWidget *table;
};
