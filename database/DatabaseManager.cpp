#include "DatabaseManager.h"
#include <QDir>
#include <QSqlError>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPointF>
#include <QDebug>

/*
void DatabaseManager::initDatabase() {
    QString dbPath = QDir::currentPath() + "/devices.db";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qWarning() << "无法打开数据库：" << db.lastError().text();
        return;
    }

    QSqlQuery query;
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS tie_shoes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            shoe_id TEXT UNIQUE,
            location TEXT,
            status TEXT DEFAULT 'free'
        )
    )");

    query.exec(R"(
        CREATE TABLE IF NOT EXISTS shoe_cabinets (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            cabinet_id TEXT UNIQUE,
            position TEXT,
            capacity INTEGER
        )
    )");

    query.exec(R"(
        CREATE TABLE IF NOT EXISTS geo_fence (
            id INTEGER PRIMARY KEY CHECK (id = 1),  -- 只允许一行
            points TEXT  -- 存储 JSON 字符串: [{"lng":120.7,"lat":30.7}, ...]
        )
    )");

    // 插入示例数据（仅当不存在时）
    query.prepare("INSERT OR IGNORE INTO tie_shoes (shoe_id, location) VALUES (?, ?)");
    query.addBindValue("TS001");
    query.addBindValue("辆3道");
    query.exec();

    query.prepare("INSERT OR IGNORE INTO shoe_cabinets (cabinet_id, position, capacity) VALUES (?, ?, ?)");
    query.addBindValue("CB01");
    query.addBindValue("检修库东侧");
    query.addBindValue(20);
    query.exec();
}

QList<QVariantMap> DatabaseManager::getTieShoes() {
    QList<QVariantMap> result;
    QSqlQuery query("SELECT id, shoe_id, location, status FROM tie_shoes");
    while (query.next()) {
        QVariantMap row;
        row["id"] = query.value(0).toInt();
        row["shoe_id"] = query.value(1).toString();
        row["location"] = query.value(2).toString();
        row["status"] = query.value(3).toString();
        result.append(row);
    }
    return result;
}

QList<QVariantMap> DatabaseManager::getShoeCabinets() {
    QList<QVariantMap> result;
    QSqlQuery query("SELECT id, cabinet_id, position, capacity FROM shoe_cabinets");
    while (query.next()) {
        QVariantMap row;
        row["id"] = query.value(0).toInt();
        row["cabinet_id"] = query.value(1).toString();
        row["position"] = query.value(2).toString();
        row["capacity"] = query.value(3).toInt();
        result.append(row);
    }
    return result;
}

QList<QPointF> DatabaseManager::loadGeoFence() {
    QList<QPointF> result;
    QSqlQuery query("SELECT points FROM geo_fence WHERE id = 1");
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

    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO geo_fence (id, points) VALUES (1, ?)");
    query.addBindValue(jsonStr);
    if (!query.exec()) {
        qWarning() << "Failed to save geo fence:" << query.lastError();
    }
}
*/


DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent) {
}

DatabaseManager::~DatabaseManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
    if (!m_connectionName.isEmpty()) {
        QSqlDatabase::removeDatabase(m_connectionName); // ✅ 必须调用！
    }
}


void DatabaseManager::initDatabase() {
    // 注意：每个线程需要独立的数据库连接名
    QString dbPath = QDir::currentPath() + "/devices.db";
    qDebug() << "================ Database path:" << dbPath;
    m_connectionName = "WorkerConnection_" + QString::number((quint64)this);
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    // m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);


    if (!m_db.open()) {
        emit errorOccurred("Database open failed: " + m_db.lastError().text());
        return;
    }

    // // 这里可以执行建表语句，确保表存在
    // QSqlQuery query(m_db);
    // query.exec("CREATE TABLE IF NOT EXISTS devices (id INTEGER PRIMARY KEY AUTOINCREMENT, "
    //            "device_id TEXT UNIQUE NOT NULL, name TEXT, value REAL, status TEXT, last_update_time DATETIME)");
    // query.exec("CREATE INDEX IF NOT EXISTS idx_device_id ON devices(device_id)");

    QSqlQuery query(m_db);
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS tie_shoes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            shoe_id TEXT UNIQUE,
            location TEXT,
            status TEXT DEFAULT 'free'
        )
    )");

    query.exec(R"(
        CREATE TABLE IF NOT EXISTS shoe_cabinets (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            cabinet_id TEXT UNIQUE,
            position TEXT,
            capacity INTEGER
        )
    )");

    query.exec(R"(
        CREATE TABLE IF NOT EXISTS geo_fence (
            id INTEGER PRIMARY KEY CHECK (id = 1),  -- 只允许一行
            points TEXT  -- 存储 JSON 字符串: [{"lng":120.7,"lat":30.7}, ...]
        )
    )");

    // 插入示例数据（仅当不存在时）
    query.prepare("INSERT OR IGNORE INTO tie_shoes (shoe_id, location) VALUES (?, ?)");
    query.addBindValue("TS001");
    query.addBindValue("辆3道");
    query.exec();

    query.prepare("INSERT OR IGNORE INTO shoe_cabinets (cabinet_id, position, capacity) VALUES (?, ?, ?)");
    query.addBindValue("CB01");
    query.addBindValue("检修库东侧");
    query.addBindValue(20);
    query.exec();
}

void DatabaseManager::handleBaseStationShoeData(const QList<ShoeData>& data)
{
    qDebug() << "============= DatabaseManager::handleBaseStationShoeData Start  ==============";
    for(const auto& shoe:data)
    {
        qDebug() << "wDevID: " << shoe.wDevID
                 << " byBatVal: " << shoe.byBatVal
                 << " byPosQuality: " << shoe.byPosQuality
                 << " lng: " << QString::number(shoe.lng, 'f', 6)
                 << " lat: " << QString::number(shoe.lat, 'f', 6);
    }

    emit shoeDataUpdate(data);
    qDebug() << "========= DatabaseManager::handleBaseStationShoeData End===========";
}

void DatabaseManager::handleBaseStationCabinetData(const QList<CabinetData>& data)
{
    qDebug() << "=============== DatabaseManager::handleBaseStationCabinetData Start ==========";
    for(const auto& shoe:data)
    {
        qDebug() << "wDevID: " << shoe.wDevID
                 << " byStoreNum: " << shoe.byStoreNum
                 << " abyStatus: " << shoe.abyStatus.toHex(' ').toUpper();
    }

    emit cabinetDataUpdated(data);
    qDebug() << "=======DatabaseManager::handleBaseStationCabinetData End =========";
}

void DatabaseManager::handleGetShoeData()
{
    qDebug() << "=============== DatabaseManager::handleGetShoeData ====================";
    auto data = getTieShoes();
    qDebug() << "=============== emit data_size:" << data.size()<< " ====================";
    emit allShoeData(data);
    qDebug() << "===================================";
}

void DatabaseManager::handleGetCabinetData()
{
    qDebug() << "=============== handleGetCabinetData ====================";
    emit allShoeCabinetData(getShoeCabinets());
    qDebug() << "===================================";
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

QList<QVariantMap> DatabaseManager::getTieShoes() {
    QList<QVariantMap> result;
    QSqlQuery query(m_db);
    query.prepare("SELECT id, shoe_id, location, status FROM tie_shoes");
    // QSqlQuery query("SELECT id, shoe_id, location, status FROM tie_shoes");

    if (!query.exec()) { // 👈 必须 exec()
        qWarning() << "Query failed:" << query.lastError();
        return result;
    }

    while (query.next()) {
        QVariantMap row;
        row["id"] = query.value(0).toInt();
        row["shoe_id"] = query.value(1).toString();
        row["location"] = query.value(2).toString();
        row["status"] = query.value(3).toString();
        result.append(row);
    }
    return result;
}

QList<QVariantMap> DatabaseManager::getShoeCabinets() {
    QList<QVariantMap> result;
    QSqlQuery query(m_db);
    query.prepare("SELECT id, cabinet_id, position, capacity FROM shoe_cabinets");
    // QSqlQuery query("SELECT id, cabinet_id, position, capacity FROM shoe_cabinets");
    if (!query.exec()) { // 👈 必须 exec()
        qWarning() << "Query failed:" << query.lastError();
        return result;
    }

    while (query.next()) {
        QVariantMap row;
        row["id"] = query.value(0).toInt();
        row["cabinet_id"] = query.value(1).toString();
        row["position"] = query.value(2).toString();
        row["capacity"] = query.value(3).toInt();
        result.append(row);
    }
    return result;
}

QList<QPointF> DatabaseManager::loadGeoFence() {
    QList<QPointF> result;
    QSqlQuery query(m_db);
    query.prepare("SELECT points FROM geo_fence WHERE id = 1");
    // QSqlQuery query("SELECT points FROM geo_fence WHERE id = 1");
    if (!query.exec()) { // 👈 必须 exec()
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
