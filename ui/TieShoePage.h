#pragma once
#include <QWidget>
#include <QTableWidget>

#include "DeviceManager.h"

class TieShoePage : public QWidget {
    Q_OBJECT
public:
    TieShoePage(QWidget *parent = nullptr);

    // 列定义（必须从 0 开始连续）
    enum class Column {
        ShoeId = 0,
        Status,
        Position,
        BatVal,     // 电量值
        PosQuality, // 位置质量
        StarNum,    // 卫星数量
        CabinetId,  // 所在鞋柜id
        StoreIdx,   // 所在仓位序号
        Count       // 用于校验列数，必须放在最后
    };

    // 获取列标题（用于初始化表头）
    static QString columnHeader(Column col) {
        switch (col) {
        case Column::ShoeId:  return "铁鞋编号";
        case Column::Status:  return "状态";
        case Column::Position:   return "位置";
        case Column::BatVal:   return "电量值";
        case Column::PosQuality:   return "位置质量";
        case Column::StarNum:   return "卫星数量";
        case Column::CabinetId:   return "所在鞋柜ID";
        case Column::StoreIdx:   return "所在仓位序号";
        default:                 return "";
        }
    }

    // 获取列索引（类型安全）
    static int columnIndex(Column col) {
        return static_cast<int>(col);
    }

public slots:
    void updateFromDeviceManager(const QMap<quint16, std::shared_ptr<IconShoe>>& shoeMap);

signals:

private:
    QTableWidget *table;
};
