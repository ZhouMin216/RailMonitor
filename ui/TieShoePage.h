#pragma once
#include <QWidget>
#include <QTableWidget>

class TieShoePage : public QWidget {
    Q_OBJECT
public:
    TieShoePage(QWidget *parent = nullptr);
    void reloadData();

public slots:
    void handleIncomingShoeData(const QList<QVariantMap>& data);

signals:
    void getShoeData();

private:
    QTableWidget *table;
};
