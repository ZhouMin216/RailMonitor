#pragma once
#include <QWidget>
#include <QTableWidget>

#include "DeviceManager.h"

class ShoeCabinetPage : public QWidget {
    Q_OBJECT
public:
    ShoeCabinetPage(QWidget *parent = nullptr);

    // 列定义（必须从 0 开始连续）
    enum class Column {
        CabinetId = 0,
        Status,
        Position,
        StoreNum,
        DetailsButton, // 新增：详情按钮列
        Count  // 用于校验列数，必须放在最后
    };

    // 获取列标题（用于初始化表头）
    static QString columnHeader(Column col) {
        switch (col) {
        case Column::CabinetId:  return "柜编号";
        case Column::Status:  return "状态";
        case Column::Position:   return "位置";
        case Column::StoreNum:   return "仓位数";
        case Column::DetailsButton:   return "仓位详情";
        default:                 return "";
        }
    }

    // 获取列索引（类型安全）
    static int columnIndex(Column col) {
        return static_cast<int>(col);
    }

private:
    void showShoeDetailsDialog(
        quint8 storeNum,
        quint16 cabinetId,
        const QByteArray& statusArray,
        const QMap<quint8, quint16>& shoeIds);

public slots:
    void updateFromDeviceManager(
        const QMap<quint16, std::shared_ptr<ShoeCabinet>>& cabinetMap);

signals:

private:
    QTableWidget *table;
};
