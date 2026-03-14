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
     // 接收基站传来的数据
    void handleBaseStationShoeData(const QList<ShoeData>& data);
    void handleBaseStationCabinetData(const QList<CabinetData>& data);

    // 接收ui发送的请求
    void handleGetShoeData();
    void handleGetCabinetData();
    void handleGetGeoFence();
    void handleSaveGeoFence(const QList<QPointF>& point);

    // 可选：定时清理或批量处理槽

signals:
    // 通知 UI 有新数据加入
    void shoeDataUpdate(const QList<ShoeData>& data);
    void cabinetDataUpdated(const QList<CabinetData>& data);

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
    QString m_connectionName;
};
