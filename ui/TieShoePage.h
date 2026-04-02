#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QStyledItemDelegate>

#include "DeviceManager.h"

// ========================
// 内部辅助类：电量进度条委托
// ========================
class BatteryBarDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit BatteryBarDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

// ========================
// 内部辅助类：状态指示器 (新)
// ========================
class StatusItem : public QTableWidgetItem {
public:
    explicit StatusItem(ShoeStatus status);

    // 重写 data() 方法以提供文字
    QVariant data(int role) const override;

private:
    ShoeStatus m_status;
    QString m_statusText;
    QPixmap createCirclePixmap(const QColor& color) const;
};

class TieShoePage : public QWidget {
    Q_OBJECT
public:
    TieShoePage(QWidget *parent = nullptr);

    enum class Column {
        PaintedId = 0,
        Status,
        BatVal,
        PosQuality,
        StarNum,
        CabinetId,
        ShoeId,
        Position,
        Count
    };

    static QString columnHeader(Column col) {
        switch (col) {
        case Column::PaintedId: return "铁鞋编号";
        case Column::ShoeId:  return "铁鞋序列号";
        case Column::Status:  return "状态";
        case Column::Position:   return "位置";
        case Column::BatVal:   return "电量值";
        case Column::PosQuality:   return "位置质量";
        case Column::StarNum:   return "卫星数量";
        case Column::CabinetId:   return "所在鞋柜ID";
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
    BatteryBarDelegate *batteryDelegate = nullptr;
};
