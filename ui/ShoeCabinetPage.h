// ShoeCabinetPage.h
#pragma once
#include <QWidget>
#include <QMap>
#include <memory>
#include <QPointF>
#include <QGridLayout>

#include "DeviceManager.h"

class QScrollArea;
class QVBoxLayout;
class QLabel;

class ShoeCabinetPage : public QWidget {
    Q_OBJECT

public:
    explicit ShoeCabinetPage(QWidget *parent = nullptr);

public slots:
    // 接收设备管理器数据
    void updateFromDeviceManager(const QMap<quint16, std::shared_ptr<ShoeCabinet>>& cabinetMap);
    void dataUpdated();

private:
    void clearCards();
    QWidget* createCabinetCard(quint16 cabinetId, const std::shared_ptr<ShoeCabinet>& cabinet);
    void updateStatistics(const QMap<quint16, std::shared_ptr<ShoeCabinet>>& cabinetMap);


    // UI
    QLabel *onlineLabel = nullptr;
    QLabel *offlineLabel = nullptr;
    QScrollArea *scrollArea = nullptr;
    QWidget *container = nullptr;
    // QVBoxLayout *cardLayout = nullptr;
    QGridLayout *cardGrid = nullptr;

    // 存储卡片指针（用于清理）
    // QMap<quint16, QWidget*> cardMap;
    // QList<QWidget*> cardWidgets; // 临时存储本次创建的卡片

    // 数据缓存（用于统计）
    QMap<quint16, std::shared_ptr<ShoeCabinet>> currentCabinets;

    // 对话框
    void showShoeDetailsDialog(quint8 storeNum, quint16 cabinetId, const QByteArray& statusArray,
                               const QMap<quint8, quint16>& shoeIds);
};
