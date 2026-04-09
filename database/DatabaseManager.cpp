#include "DatabaseManager.h"
#include <QDir>
#include <QSqlError>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPointF>
#include <QDebug>


DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent) {
}

DatabaseManager::~DatabaseManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}


void DatabaseManager::initDatabase() {
    QString dbPath = QDir::currentPath() + "/devices.db";
    qDebug() << "================ Database path:" << dbPath;
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);


    if (!m_db.open()) {
        emit errorOccurred("Database open failed: " + m_db.lastError().text());
        return;
    }

    // 执行建表语句，确保表存在
    QSqlQuery query(m_db);

    // 白名单表
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS whitelist (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            department TEXT NOT NULL
        )
    )");

    // 电子围栏表
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS geo_fence (
            id INTEGER PRIMARY KEY CHECK (id = 1),  -- 只允许一行
            points TEXT  -- 存储 JSON 字符串: [{"lng":120.7,"lat":30.7}, ...]
        )
    )");

    // 数据盘点配置表
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS data_inventory_config (
            id INTEGER PRIMARY KEY CHECK (id = 1),
            export_path TEXT,
            export_time TEXT  -- 格式 "HH:mm"
        )
    )");
}



void DatabaseManager::handleGetGeoFence()
{
    qDebug() << "=============== DatabaseManager::handleGetGeoFence ====================";
    emit geoFenceData(loadGeoFence());
    qDebug() << "===================================";
}

void DatabaseManager::handleSaveGeoFence(const QList<QPointF>& point)
{
    qDebug() << "=============== handleSaveGeoFence ====================";
    saveGeoFence(point);
    qDebug() << "===================================";
}

void DatabaseManager::handleClearGeoFence()
{
    qDebug() << "=============== handleClearGeoFence ====================";

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM geo_fence WHERE id = 1");
    if (!query.exec()) {
        qWarning() << "Failed to delete geo fence:" << query.lastError();
    }
    qDebug() << "===================================";
}




QList<QPointF> DatabaseManager::loadGeoFence() {
    QList<QPointF> result;
    QSqlQuery query(m_db);
    query.prepare("SELECT points FROM geo_fence WHERE id = 1");
    // QSqlQuery query("SELECT points FROM geo_fence WHERE id = 1");
    if (!query.exec()) { // 必须 exec()
        qWarning() << "Query failed:" << query.lastError();
        return result;
    }

    if (query.next()) {
        QByteArray jsonBytes = query.value(0).toByteArray();
        QJsonDocument doc = QJsonDocument::fromJson(jsonBytes);
        if (!doc.isArray()) return result;
        QJsonArray arr = doc.array();
        for (const QJsonValue& v : arr) {
            QJsonObject obj = v.toObject();
            double lng = obj["lng"].toDouble();
            double lat = obj["lat"].toDouble();
            result.append(QPointF(lng, lat));
        }
    }
    return result;
}

void DatabaseManager::saveGeoFence(const QList<QPointF>& points) {
    QJsonArray arr;
    for (const QPointF& pt : points) {
        QJsonObject obj;
        obj["lng"] = pt.x();
        obj["lat"] = pt.y();
        arr.append(obj);
    }
    QJsonDocument doc(arr);
    QString jsonStr = doc.toJson(QJsonDocument::Compact);

    QSqlQuery query(m_db);
    query.prepare("INSERT OR REPLACE INTO geo_fence (id, points) VALUES (1, ?)");
    query.addBindValue(jsonStr);
    if (!query.exec()) {
        qWarning() << "Failed to save geo fence:" << query.lastError();
    }
}


// ==================== Whitelist CRUD ====================

void DatabaseManager::handleGetWhitelist(quint32 id) {
    qDebug() << "=============== handleGetWhitelist id：" << id << " ====================";
    emit whitelistData(loadWhitelist(id));
    qDebug() << "===================================";
}

void DatabaseManager::handleAddToWhitelist(const WhitelistEntry& entry) {
    qDebug() << "=============== handleAddToWhitelist ====================";
    bool ok = addToWhitelist(entry);
    emit whitelistOperationResult(ok, ok ? "添加成功" : "添加失败");
    qDebug() << "===================================";
}

void DatabaseManager::handleUpdateWhitelist(const WhitelistEntry& entry) {
    qDebug() << "=============== handleUpdateWhitelist ====================";
    bool ok = updateWhitelist(entry);
    emit whitelistOperationResult(ok, ok ? "更新成功" : "更新失败");
    qDebug() << "===================================";
}

void DatabaseManager::handleRemoveFromWhitelist(quint32 id) {
    qDebug() << "=============== handleRemoveFromWhitelist ====================";
    bool ok = removeFromWhitelist(id);
    emit whitelistOperationResult(ok, ok ? "删除成功" : "删除失败");
    qDebug() << "===================================";
}

// --- Private implementations ---

QMap<quint32, WhitelistEntry> DatabaseManager::loadWhitelist(quint32 id) {
    // TODO 当id等于0时获取全部数据，不等于0时获取指定id数据
    QMap<quint32, WhitelistEntry> list;
    QSqlQuery query(m_db);
    query.prepare("SELECT id, name, department FROM whitelist ORDER BY id");
    if (!query.exec()) {
        qWarning() << "Failed to load whitelist:" << query.lastError();
        return list;
    }

    while (query.next()) {
        WhitelistEntry entry;
        entry.uid = static_cast<quint32>(query.value(0).toUInt()); // SQLite 返回 unsigned int
        entry.name = query.value(1).toString();
        entry.department = query.value(2).toString();
        list[entry.uid] = entry;
    }
    return list;
}

bool DatabaseManager::addToWhitelist(const WhitelistEntry& entry) {
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO whitelist (id, name, department) VALUES (?, ?, ?)");
    query.addBindValue(entry.uid);
    query.addBindValue(entry.name);
    query.addBindValue(entry.department);
    if (!query.exec()) {
        qWarning() << "Add to whitelist failed:" << query.lastError();
        return false;
    }
    return true;
}

bool DatabaseManager::updateWhitelist(const WhitelistEntry& entry) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE whitelist SET name = ?, department = ? WHERE id = ?");
    query.addBindValue(entry.name);
    query.addBindValue(entry.department);
    query.addBindValue(entry.uid);
    if (!query.exec()) {
        qWarning() << "Update whitelist failed:" << query.lastError();
        return false;
    }
    return query.numRowsAffected() > 0; // 确保有行被更新
}

bool DatabaseManager::removeFromWhitelist(quint32 id) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM whitelist WHERE id = ?");
    query.addBindValue(id);
    if (!query.exec()) {
        qWarning() << "Remove from whitelist failed:" << query.lastError();
        return false;
    }
    return query.numRowsAffected() > 0;
}

void DatabaseManager::loadDataInventoryConfig() {
    QSqlQuery query(m_db);
    query.prepare("SELECT export_path, export_time FROM data_inventory_config WHERE id = 1");
    QString path;
    QTime time;

    if (query.exec() && query.next()) {
        path = query.value(0).toString();
        QString timeStr = query.value(1).toString();
        time = QTime::fromString(timeStr, "HH:mm");
        if (!time.isValid()) time = QTime::currentTime(); // 容错
    } else {
        // 无记录，使用默认值或空值
        path = "未选择目录";
        time = QTime::currentTime();
    }

    emit dataInventoryConfigLoaded(path, time);
}

void DatabaseManager::handleDataInventoryConfig(const QString &path, const QTime &time) {
    QSqlQuery query(m_db);
    bool ok = true;
    query.prepare("INSERT OR REPLACE INTO data_inventory_config (id, export_path, export_time) VALUES (1, ?, ?)");
    query.addBindValue(path);
    query.addBindValue(time.toString("HH:mm"));
    if (!query.exec()) {
        qWarning() << "Failed to save config:" << query.lastError();
        ok = false;
    }

    emit whitelistOperationResult(ok, ok ? "保存成功" : "保存失败");
}
