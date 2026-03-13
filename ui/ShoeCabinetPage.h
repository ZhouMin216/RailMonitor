#pragma once
#include <QWidget>
#include <QTableWidget>

class ShoeCabinetPage : public QWidget {
    Q_OBJECT
public:
    ShoeCabinetPage(QWidget *parent = nullptr);
    void reloadData();

public slots:
    void handleIncomingShoeCabinetData(const QList<QVariantMap>& data);

signals:
    void getShoeCabinetData();
private:
    QTableWidget *table;
};
