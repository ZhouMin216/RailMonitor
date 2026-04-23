#ifndef EVENTPAGE_H
#define EVENTPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QStyledItemDelegate>

#include "DeviceManager.h"

class EventPage : public QWidget
{
    Q_OBJECT
public:
    explicit EventPage(QWidget *parent = nullptr);

    enum class Column {
        EventTime = 0,
        Level,
        Details,
        Count
    };

    static QString columnHeader(Column col) {
        switch (col) {
        case Column::EventTime: return "时间";
        case Column::Level:  return "等级";
        case Column::Details:  return "详情";
        default:                 return "";
        }
    }

    static int columnIndex(Column col) {
        return static_cast<int>(col);
    }

public slots:
    void dataUpdated();

signals:

private:
    void updateFromDeviceManager(const QMap<quint16, std::shared_ptr<IconShoe>>& shoeMap);
    void updateStatistics(const QMap<quint16, std::shared_ptr<IconShoe>>& shoeMap);

private:
    QTableWidget *table;
    QLabel *onlineLabel = nullptr;
    QLabel *offlineLabel = nullptr;
};

#endif // EVENTPAGE_H
