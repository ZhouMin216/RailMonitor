#include "DeviceManager.h"

#include <QMessageBox>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDir>

IconShoe::IconShoe(quint16 id, quint16 cabinet_id, quint8 store_id)
    : shoe_id_(id), cabinet_id_(cabinet_id), store_id_(store_id) {

}

ShoeCabinet::ShoeCabinet(quint16 id, quint8 store_num, QPointF pos)
    : cabinet_id_(id), store_num_(store_num), pos_(pos){

}


DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent){

}

DeviceManager::~DeviceManager(){

}

void DeviceManager::loadConfig(){
    QString configPath = QDir::currentPath() + "/rail_config.json";
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(nullptr, tr("错误"), tr("配置文件未找到：") + configPath);
        return;
    }

    QByteArray data = file.readAll();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        QMessageBox::critical(nullptr, tr("错误"), tr("配置文件格式错误"));
        return;
    }

    cabiner_Map_.clear();
    shoe_map_.clear();
    QJsonObject root = doc.object();

    const QJsonArray& shoeCabinetArray = root["shoeCabinets"].toArray();
    for (const QJsonValue &val : shoeCabinetArray) {
        if (!val.isObject()) continue;
        QJsonObject p = val.toObject();
        // --- 解析基础字段 ---
        quint16 cabinet_id = static_cast<quint16>(p["id"].toInt());
        // info.name = p["name"].toString();
        double lng = p["lng"].toDouble();
        double lat = p["lat"].toDouble();
        QPointF pos = QPointF(lat,lng);
        quint8 store_num = static_cast<quint16>(p["store_num"].toInt());

        std::shared_ptr<ShoeCabinet> cabinet = std::make_shared<ShoeCabinet>(cabinet_id, store_num, pos);

        // --- 解析 store_arr 嵌套数组 ---
        QJsonArray storeArr = p["store_arr"].toArray();

        for (const QJsonValue &storeVal : storeArr) {
            if (!storeVal.isObject()) continue;
            QJsonObject storeObj = storeVal.toObject();

            // JSON 对象只有一个 key-value 对，例如 {"1": 10000}
            // 需要遍历这个对象来获取 key 和 value
            QStringList keys = storeObj.keys();
            if (!keys.isEmpty()) {
                QString keyStr = keys.first();          // 获取 "1", "2"...
                int store_idx = keyStr.toInt();         // 转换为 int: 1, 2...
                quint16 shoe_id = static_cast<quint16>(storeObj[keyStr].toInt());

                // 存入临时 map
                std::shared_ptr<IconShoe> shoe = std::make_shared<IconShoe>(shoe_id, cabinet_id, store_idx);
                if (shoe_map_.contains(shoe_id)){
                    QMessageBox::critical(nullptr, tr("错误"), tr("铁鞋ID重复：") + QString("%1").arg(shoe_id));
                    return;
                }
                shoe_map_[shoe_id] = shoe;
                cabinet->AddIconShoe(store_idx, shoe);

                // 调试输出 (可选)
                qDebug() << "Cabinet " << cabinet_id << " store_idx" << store_idx << "-> shoe_id:" << shoe_id;
            }
        }

        if (cabiner_Map_.contains(cabinet_id)){
            QMessageBox::critical(nullptr, tr("错误"), tr("铁鞋柜ID重复：") + QString("%1").arg(cabinet_id));
            return;
        }
        cabiner_Map_[cabinet_id] = cabinet;

        // shoeCabinetPoints[id] = QPointF(lat,lng);
    }
    qDebug() << "Cabinet size: " << cabiner_Map_.size() << " shoe_map_ size" << shoe_map_.size() ;
}
