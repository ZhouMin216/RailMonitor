#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "./protocol/DeviceParser.h"
#include <QPointF>
#include <QObject>
#include <QMap>
#include <QApplication>
#include <QDateTime>

class IconShoe
{
public:
    IconShoe(quint16 id, quint16 cabinet_id, QString painted_id);

    const ShoeData& GetShoeData() { return shoeData_;}
    void SetData(const ShoeData& data) {
        status_update_time_ = QDateTime::currentSecsSinceEpoch();
        shoeData_ = data;
    }
    quint16 GetCabinetID() { return cabinet_id_;}
    quint16 GetShoeID() { return shoe_id_;}
    quint8 GetStoreID() { return store_id_;}
    void ChangeShoeStatus(ShoeStatus status) {
        status_update_time_ = QDateTime::currentSecsSinceEpoch();
        shoeData_.byOnline = status;
    }

    bool IsLost(){
        if (shoeData_.byOnline != ShoeStatus::Offline) return false;

        auto cur_time = QDateTime::currentSecsSinceEpoch();
        return cur_time - status_update_time_ >= 10 * 60;
    }

private:
    quint16 shoe_id_; // 铁鞋ID
    ShoeData shoeData_;
    quint16 cabinet_id_; // 铁鞋所在的鞋柜ID
    quint8 store_id_; // 铁鞋所在的仓位ID
    qint64 status_update_time_{0}; // 状态更新时间，如果超过10分钟没有更新、并且离线离柜，认为丢失
};

class ShoeCabinet
{
public:
    ShoeCabinet(quint16 id, quint8 store_num, QPointF pos, QList<quint16> shoe_ids);

    const CabinetData& GetCabinetData() { return data_;}
    void SetData(const CabinetData& data) { data_ = data;}
    quint16 GetCabinetID() { return cabinet_id_;}
    quint8 GetStoreNum() { return store_num_;}
    void AddIconShoe(quint8 store_idx, std::shared_ptr<IconShoe> shoe){
        store_map_[store_idx] = shoe;
        store_idx_ = store_map_.keys();
    }
    const QPointF& GetPos() { return pos_;}
    // 获取仓位和铁鞋的绑定关系，key:仓位下标，value:铁鞋ID
    QMap<quint8, quint16> GetStoreShoeID();
    bool ShoeIsInStore(quint8 store_idx); // 判断仓位上的鞋是否在位
private:
    void initStoreStatus(QList<quint16> shoe_ids);

private:
    quint16 cabinet_id_; // 鞋柜ID
    CabinetData data_;
    quint8 store_num_; // 仓位数量
    // quint8 enable_store_num_; // 实际配置了铁鞋的仓位数量
    QVector<quint16> store_shoe_id_; // 仓位存储的铁鞋id
    QMap<quint8, std::shared_ptr<IconShoe>> store_map_; // key是仓位序号 1~N

    QPointF pos_; // 经纬度
    QList<quint8> store_idx_;
};



class DeviceManager: public QObject
{
    Q_OBJECT
public:
    static DeviceManager* instance() {
        static DeviceManager inst; // Meyers Singleton
        return &inst;
    }

    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;

    // explicit DeviceManager(QObject *parent = nullptr);
    // ~DeviceManager();

    void loadConfig();

    const QMap<quint16, std::shared_ptr<ShoeCabinet>>& getCabinetMap() {
        return cabinet_Map_;
    }

    const QMap<quint16, std::shared_ptr<IconShoe>>& getShoeMap() {
        return shoe_map_;
    }

    CabinetStatus getCabinetStatus(quint16 cabinetId) {
        if (cabinet_Map_.contains(cabinetId))
            return cabinet_Map_[cabinetId]->GetCabinetData().byOnline;
        return CabinetStatus::NoEnter;
    }

    QString getPaintedID(quint16 shoeId);
    quint16 getCabinetIdByShoeId(quint16 shoeId);

    const QList<quint16>& getCabinetBindShoes(quint16 cabinetId) {
        static const QList<quint16> emptyList;
        if (cabinet_config_.contains(cabinetId)) return cabinet_config_[cabinetId];
        return emptyList;
    }

// public slots:
    // 接收tcp数据
    bool updateCabinetStatus(QList<CabinetData> data);
    bool updateShoeStatus(QList<ShoeData> data);

private:
    void checkBinding(CabinetData& data);


signals:
    // void shoeCabinetUpdated(const QMap<quint16, std::shared_ptr<ShoeCabinet>>& data);
    // void iconShoeUpdated(const QMap<quint16, std::shared_ptr<IconShoe>>& data);
    // void shoeData(const QList<ShoeData>& data);

private:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager() override = default;

    QMap<quint16, std::shared_ptr<ShoeCabinet>> cabinet_Map_;
    QMap<quint16, std::shared_ptr<IconShoe>> shoe_map_;

    // 车辆段给铁鞋喷涂的编号和定位设备编号的绑定关系
    // 例如：key:"#1-1", value: 10001
    QMap<QString, quint16> shoe_id_config_;

    // key:鞋柜id，value:鞋柜绑定的铁鞋id列表
    QMap<quint16, QList<quint16>> cabinet_config_;
};

#endif // DEVICEMANAGER_H
