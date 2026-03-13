#pragma once
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariantMap>

class DatabaseManager {
public:
    static void initDatabase();
    static QList<QVariantMap> getTieShoes();
    static QList<QVariantMap> getShoeCabinets();
    static QList<QPointF> loadGeoFence();
    static void saveGeoFence(const QList<QPointF>& points);
};
