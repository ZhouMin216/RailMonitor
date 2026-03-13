#pragma once
#include <QWidget>
#include <QTableWidget>

class TieShoePage : public QWidget {
    Q_OBJECT
public:
    TieShoePage(QWidget *parent = nullptr);
    void reloadData();
private:
    QTableWidget *table;
};
