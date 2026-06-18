#pragma once
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariantMap>
#include <QPointF>

#include "protocol/DeviceParser.h"
#include "protocol/WhitelistSync.h"

class DatabaseManager: public QObject {
    Q_OBJECT
public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    void initDatabase();
    void loadDataInventoryConfig();

public slots:
    // 电子围栏相关
    void handleGetGeoFence();
    void handleSaveGeoFence(const QList<QPointF>& point);
    void handleClearGeoFence();

    // 白名单相关
    void handleAddToWhitelist(const WhitelistEntry& entry);
    void handleUpdateWhitelist(const WhitelistEntry& entry);
    void handleRemoveFromWhitelist(quint32 id);
    void handleGetWhitelist(quint32 id);
    void handleReplaceAllWhitelist(const QMap<quint32, WhitelistEntry>& whitelist_map);

    // 数据盘点配置
    void handleDataInventoryConfig(const QString &path, const QTime &time);

    // 事件记录
    void handleAddEventLog(const QString& level, const QString& message);
    void handleGetEventLogs(int limit, int offset);
    // void handleRemoveEventLog(quint32 id);
    void handleGetTotalEventCount();

signals:
    // 电子围栏
    void geoFenceData(const QList<QPointF>& data);

    // 白名单
    void whitelistData(const QMap<quint32, WhitelistEntry>& entries);
    void whitelistOperationResult(bool success, const QString& message = QString());

    // 数据盘点配置
    void dataInventoryConfigLoaded(const QString &path, const QTime &time);

    // 通知错误
    void errorOccurred(const QString &msg);

    void eventLogsLoaded(const QList<EventLogEntry>& logs);
    void eventLogOperationResult(bool success, const QString& message = QString());
    void totalEventCountLoaded(int count);


private:
    QList<QPointF> loadGeoFence();
    void saveGeoFence(const QList<QPointF>& points);

    QMap<quint32, WhitelistEntry> loadWhitelist(quint32 id);
    bool addToWhitelist(const WhitelistEntry& entry);
    bool updateWhitelist(const WhitelistEntry& entry);
    bool removeFromWhitelist(quint32 id);
    bool replaceAllWhitelist(const QMap<quint32, WhitelistEntry>& whitelist_map);

    void cleanupOldEvents();
    void addEventLog(const QString& level, const QString& message);
    QList<EventLogEntry> getEventLogs(int limit, int offset);
    int getTotalEventCount();

private:
    QSqlDatabase m_db;
};
