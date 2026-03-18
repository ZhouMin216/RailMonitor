#include "DeviceManager.h"

#include <QMessageBox>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDir>

IconShoe::IconShoe(quint16 id, quint16 cabinet_id, quint8 store_id)
    : shoe_id_(id), cabinet_id_(cabinet_id), store_id_(store_id) {
    shoeData_.wDevID = shoe_id_;
}

ShoeCabinet::ShoeCabinet(quint16 id, quint8 store_num, QPointF pos)
    : cabinet_id_(id), store_num_(store_num), pos_(pos){
    initStoreStatus();
}

void ShoeCabinet::initStoreStatus(){
    data_.byStoreNum = store_num_;

    data_.abyStatus.resize(store_num_);
    data_.abyStatus.fill(static_cast<quint8>(StorageStatus::Unregister));
}

QVector<quint16> ShoeCabinet::GetStoreShoeID(){
    QVector<quint16> id_vec;
    for (auto it = store_map_.constBegin(); it != store_map_.constEnd(); ++it) {
        auto shoe = it.value();
        if (!shoe) continue;
        id_vec.push_back(shoe->GetShoeID());
    }
    return id_vec;
}

bool ShoeCabinet::ShoeIsInStore(quint8 store_idx){
    if (store_idx >= data_.abyStatus.size() || store_idx == 0) return false;

    return data_.abyStatus.at(store_idx - 1) == 0x02; // StorageStatus::Online 在位
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

    cabinet_Map_.clear();
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

                std::shared_ptr<IconShoe> shoe = std::make_shared<IconShoe>(shoe_id, cabinet_id, store_idx);
                if (shoe_map_.contains(shoe_id)){
                    QMessageBox::critical(nullptr, tr("错误"), tr("铁鞋ID重复：") + QString("%1").arg(shoe_id));
                    return;
                }
                shoe_map_[shoe_id] = shoe;
                cabinet->AddIconShoe(store_idx, shoe);

                // 调试输出 (可选)
                // qDebug() << "Cabinet " << cabinet_id << " store_idx" << store_idx << "-> shoe_id:" << shoe_id;
            }
        }

        if (cabinet_Map_.contains(cabinet_id)){
            QMessageBox::critical(nullptr, tr("错误"), tr("铁鞋柜ID重复：") + QString("%1").arg(cabinet_id));
            return;
        }
        cabinet_Map_[cabinet_id] = cabinet;
    }
    // qDebug() << "Cabinet size: " << cabinet_Map_.size() << " shoe_map_ size" << shoe_map_.size() ;
}

void DeviceManager::updateCabinetStatus(const QList<CabinetData>& data){
    qDebug() << "updateCabinetStatus::updateCabinetStatus  " << data.size();
    for(const auto& cabinet : data)
    {
        if (cabinet_Map_.contains(cabinet.wDevID))
        {
            auto cabinet_ptr = cabinet_Map_[cabinet.wDevID];
            cabinet_ptr->SetData(cabinet);
        }
        // qDebug() << "wDevID: " << cabinet.wDevID
        //          << " byStoreNum: " << cabinet.byStoreNum
        //          << " abyStatus: " << cabinet.abyStatus.toHex(' ').toUpper();
    }
    updateCabinet();
}

void DeviceManager::updateShoeStatus(const QList<ShoeData>& data)
{
    for(const auto& shoe : data)
    {
        if (shoe_map_.contains(shoe.wDevID))
        {
            auto shoe_ptr = shoe_map_[shoe.wDevID];
            shoe_ptr->SetData(shoe);

            quint16 cabinetId = shoe_ptr->GetCabinetID();
            if (cabinet_Map_.contains(cabinetId)){
                quint8 storeIdx = shoe_ptr->GetStoreID();
                auto cabinet_ptr = cabinet_Map_[cabinetId];
                if (cabinet_ptr->ShoeIsInStore(storeIdx)){
                    shoe_ptr->ChangeShoeStatus(DeviceStatus::InCabinet);
                }
            }
        }
    }
    updateIconShoe();
}
