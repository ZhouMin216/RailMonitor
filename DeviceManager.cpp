#include "DeviceManager.h"

#include <QMessageBox>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDir>

IconShoe::IconShoe(quint16 id, quint16 cabinet_id,  QString painted_id)
    : shoe_id_(id), cabinet_id_(cabinet_id) {
    shoeData_.wDevID = shoe_id_;
    shoeData_.paintedID = painted_id;
}

ShoeCabinet::ShoeCabinet(quint16 id, quint8 store_num, QPointF pos, QList<quint16> shoe_ids)
    : cabinet_id_(id), store_num_(store_num), pos_(pos){
    initStoreStatus(shoe_ids);
}

void ShoeCabinet::initStoreStatus(QList<quint16> shoe_ids){
    data_.wDevID = cabinet_id_;
    data_.byStoreNum = store_num_;
    data_.storeStatus.clear();
    for (const auto& shoeId : shoe_ids){
        data_.storeStatus.append(qMakePair(shoeId, StorageStatus::Unregister));
    }

    // data_.abyStatus.resize(store_num_);
    // data_.abyStatus.fill(static_cast<quint8>(StorageStatus::Unregister));
}

QMap<quint8, quint16> ShoeCabinet::GetStoreShoeID(){
    QMap<quint8, quint16> id_map;
    for (auto it = store_map_.constBegin(); it != store_map_.constEnd(); ++it) {
        auto shoe = it.value();
        if (!shoe) continue;
        id_map[it.key()] = shoe->GetShoeID();
    }
    return id_map;
}

bool ShoeCabinet::ShoeIsInStore(quint8 store_idx){
    // 将仓位下标转换成状态数组的下标
    // 因为实际接收到的状态数据是根据实际配置的铁鞋数绑定的
    // 例如：仓位数有6个，但是只有1-3-5绑定了铁鞋，所以状态数组只有3个状态数据
    // store_idx_在鞋柜绑定铁鞋时记录了每一个仓位下标
    int status_idx = store_idx_.indexOf(store_idx);
    if (status_idx >= data_.abyStatus.size() || status_idx == -1) return false;

    return data_.abyStatus.at(status_idx) == 0x02; // StorageStatus::Online 在位
}

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent){
    // loadConfig();
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
        QApplication::quit();
        return;
    }

    shoe_map_.clear();
    shoe_id_config_.clear();

    QJsonObject root = doc.object();
    if (!root.contains("shoes") || !root["shoes"].isObject()) {
        QMessageBox::critical(nullptr, tr("错误"), tr("缺少铁鞋配置或者配置错误"));
        QApplication::quit();
        return;
    }

    // 解析铁鞋设备ID和喷涂编号的绑定关系
    QJsonObject shoesObj = root["shoes"].toObject();
    QSet<QString> seenKeys;
    QSet<quint16> seenValues;
    for (auto it = shoesObj.constBegin(); it != shoesObj.constEnd(); ++it) {
        QString key = it.key();
        if (seenKeys.contains(key)) {
            QMessageBox::critical(nullptr, tr("错误"), tr("铁鞋编号重复：") + key);
            QApplication::quit();
        }
        seenKeys.insert(key);

        QJsonValue value = it.value();
        if (!value.isDouble()) { // JSON 中整数也以 double 存储
            QMessageBox::critical(nullptr, tr("错误"), tr("铁鞋序列号不合法！对应编号：") + key);
            QApplication::quit();
        }

        quint16 val = static_cast<quint16>(value.toInteger());
        if (seenValues.contains(val)) {
            QMessageBox::critical(nullptr, tr("错误"), tr("铁鞋序列号重复：") + key);
            QApplication::quit();
        }
        seenValues.insert(val);

        shoe_id_config_.insert(key, static_cast<quint16>(val));
    }

    qDebug() << "解析到" << shoe_id_config_.size() << "个鞋柜-仓位映射：";
    for (auto it = shoe_id_config_.constBegin(); it != shoe_id_config_.constEnd(); ++it) {
        qDebug() << it.key() << "=>" << it.value();
    }

    cabinet_Map_.clear();
    cabinet_config_.clear();
    // 解析鞋柜配置
    const QJsonArray& shoeCabinetArray = root["shoeCabinets"].toArray();
    for (const QJsonValue &val : shoeCabinetArray) {
        if (!val.isObject()) continue;
        QJsonObject p = val.toObject();

        quint16 cabinet_id = static_cast<quint16>(p["id"].toInt());
        if (cabinet_Map_.contains(cabinet_id) || cabinet_config_.contains(cabinet_id)){
            QMessageBox::critical(nullptr, tr("错误"), tr("铁鞋柜ID重复：") + QString("%1").arg(cabinet_id));
            QApplication::quit();
        }

        double lng = p["lng"].toDouble();
        double lat = p["lat"].toDouble();
        QPointF pos = QPointF(lng, lat);

        // --- 解析 store_arr 铁鞋喷涂的编号数组 --- ["#1-1", "#2-1", "#2-2"]
        QJsonArray storeArr = p["store_arr"].toArray();
        quint8 store_num = static_cast<quint16>(storeArr.size());

        QList<quint16> shoe_id_list;
        for (const QJsonValue &storeVal : storeArr) {
            if (!storeVal.isString()) continue;
            QString painted_id = storeVal.toString(); // 喷涂编号
            if (!shoe_id_config_.contains(painted_id)){
                QMessageBox::critical(nullptr, tr("错误"), tr("铁鞋编号不存在：") + painted_id);
                QApplication::quit();
            }
            quint16 shoe_id = shoe_id_config_[painted_id];
            shoe_id_list.push_back(shoe_id);

            std::shared_ptr<IconShoe> shoe = std::make_shared<IconShoe>(shoe_id, cabinet_id, painted_id);
            if (shoe_map_.contains(shoe_id)){
                QMessageBox::critical(nullptr, tr("错误"), tr("铁鞋ID重复：") + QString("%1").arg(shoe_id));
                QApplication::quit();
            }
            shoe_map_[shoe_id] = shoe;
            qDebug() << "Cabinet " << cabinet_id << " painted_id" << painted_id << "-> shoe_id:" << shoe_id;
        }

        std::shared_ptr<ShoeCabinet> cabinet = std::make_shared<ShoeCabinet>(cabinet_id, store_num, pos, shoe_id_list);

        cabinet_Map_[cabinet_id] = cabinet;
        cabinet_config_[cabinet_id] = shoe_id_list;
    }
    qDebug() << "Cabinet size: " << cabinet_Map_.size() << " shoe_map_ size" << shoe_map_.size() ;
}

bool DeviceManager::updateCabinetStatus(QList<CabinetData> data){
    if (data.empty()) return false;
    for(auto& cabinet : data) {
        if (cabinet_Map_.contains(cabinet.wDevID)) {
            checkBinding(cabinet);
            auto cabinet_ptr = cabinet_Map_[cabinet.wDevID];
            cabinet_ptr->SetData(cabinet);
        }

        // qDebug() << "========= DeviceManager::updateCabinetStatus ===========";
        // qDebug() << "wDevID: " << cabinet.wDevID
        //          << " byStoreNum: " << cabinet.byStoreNum;
        // for (const auto& pair : cabinet.storeStatus) {
        //     qDebug() << "shoeId:" << pair.first << " -> status:" << static_cast<quint8>(pair.second);
        // }
    }
    // updateCabinet();

    return true;
}

bool DeviceManager::updateShoeStatus(QList<ShoeData> data)
{
    if (data.empty()) return false;
    for(auto& shoe : data) {
        if (shoe_map_.contains(shoe.wDevID)) {
            auto shoe_ptr = shoe_map_[shoe.wDevID];

            shoe.paintedID = getPaintedID(shoe.wDevID);
            shoe_ptr->SetData(shoe);

            // quint16 cabinetId = shoe_ptr->GetCabinetID();
            // if (cabinet_Map_.contains(cabinetId)){
            //     quint8 storeIdx = shoe_ptr->GetStoreID();
            //     auto cabinet_ptr = cabinet_Map_[cabinetId];
            //     if (cabinet_ptr->ShoeIsInStore(storeIdx)){
            //         shoe_ptr->ChangeShoeStatus(ShoeStatus::InCabinet);
            //         shoe.byOnline = ShoeStatus::InCabinet;
            //     }
            // }
        }
    }
    // emit shoeData(data); // 发送给地图
    // updateIconShoe();

    return true;
}

void DeviceManager::checkBinding(CabinetData& data){
    auto shoe_id_list = cabinet_config_[data.wDevID]; // 获取鞋柜绑定的铁鞋列表
    for (auto &pair : data.storeStatus) {   // 判断鞋柜状态中的铁鞋ID是否在 鞋柜绑定的铁鞋列表中
        if (pair.second == StorageStatus::Online){
            if (!shoe_id_list.contains(pair.first)) pair.second = StorageStatus::PosFault; // 位置错误

            // 只要在柜就更新对应铁鞋的状态，暂时不管是不是在正确的柜子里
            if (shoe_map_.contains(pair.first)) {
                shoe_map_[pair.first]->ChangeShoeStatus(ShoeStatus::InCabinet);
            }
        }
    }
}

QString DeviceManager::getPaintedID(quint16 shoeId){
    static QMap<quint16, QString> reverseMap;
    if (reverseMap.isEmpty()) {
        for (auto it = shoe_id_config_.constBegin(); it != shoe_id_config_.constEnd(); ++it) {
            reverseMap.insert(it.value(), it.key());
        }
    }

    if (reverseMap.contains(shoeId)) return reverseMap[shoeId];

    return "Unknown";
}

quint16 DeviceManager::getCabinetIdByShoeId(quint16 shoeId){
    // key:鞋柜id，value:鞋柜绑定的铁鞋id列表
    for (auto it = cabinet_config_.constBegin(); it != cabinet_config_.constEnd(); ++it) {
        quint16 cabinetId = it.key();
        const QList<quint16> &shoeList = it.value();

        if (shoeList.contains(shoeId)) {
            return cabinetId;
        }
    }
    return 0;
}
