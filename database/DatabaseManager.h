#pragma once
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariantMap>
#include <QPointF>

#include "protocol/DeviceParser.h"

class DatabaseManager: public QObject {
    Q_OBJECT
public:
    // static void initDatabase();
    // static QList<QVariantMap> getTieShoes();
    // static QList<QVariantMap> getShoeCabinets();
    // static QList<QPointF> loadGeoFence();
    // static void saveGeoFence(const QList<QPointF>& points);
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    // 初始化数据库连接（需在 moveToThread 之前或在线程启动时调用）
    void initDatabase();

public slots:
    // 核心槽函数：接收网络层传来的原始数据
    void handleIncomingShoeData(const ShoeData &data);
    void handleIncomingCabinetData(const CabinetData &data);

    void handleGetShoeData();
    void handleGetCabinetData();
    void handleGetGeoFence();
    void handleSaveGeoFence(const QList<QPointF>& point);

    // 可选：定时清理或批量处理槽

signals:
    // 通知 UI 有新数据加入（包含完整行数据，方便 UI 直接插入）
    void shoeDataUpdate(const ShoeData &data);
    void cabinetDataUpdated(const CabinetData &data);

    void allShoeData(const QList<QVariantMap>& data);
    void allShoeCabinetData(const QList<QVariantMap>& data);
    void geoFenceData(const QList<QPointF>& data);

    // 通知错误
    void errorOccurred(const QString &msg);

private:
    QList<QVariantMap> getTieShoes();
    QList<QVariantMap> getShoeCabinets();
    QList<QPointF> loadGeoFence();
    void saveGeoFence(const QList<QPointF>& points);

private:
    QSqlDatabase m_db;
};
