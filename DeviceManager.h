#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "./protocol/DeviceParser.h"
#include <QPointF>
#include <QObject>
#include <QMap>

class IconShoe
{
public:
    IconShoe(quint16 id, quint16 cabinet_id, quint8 store_id);

    const ShoeData& GetShoeData() { return shoeData_;}
    void SetData(const ShoeData& data) { shoeData_ = data;}
    quint16 GetCabinetID() { return cabinet_id_;}
    quint16 GetShoeID() { return shoe_id_;}
    quint8 GetStoreID() { return store_id_;}
    void ChangeShoeStatus(DeviceStatus status) {
        shoeData_.byOnline = status;
    }

private:
    quint16 shoe_id_; // 铁鞋ID
    ShoeData shoeData_;
    quint16 cabinet_id_; // 铁鞋所在的鞋柜ID
    quint8 store_id_; // 铁鞋所在的仓位ID
};

class ShoeCabinet
{
public:
    ShoeCabinet(quint16 id, quint8 store_num, QPointF pos);

    const CabinetData& GetCabinetData() { return data_;}
    void SetData(const CabinetData& data) { data_ = data;}
    quint16 GetCabinetID() { return cabinet_id_;}
    quint8 GetStoreNum() { return store_num_;}
    void AddIconShoe(quint8 store_idx, std::shared_ptr<IconShoe> shoe){
        store_map_[store_idx] = shoe;
    }
    const QPointF& GetPos() { return pos_;}
    QVector<quint16> GetStoreShoeID();
    bool ShoeIsInStore(quint8 store_idx); // 判断仓位上的鞋是否在位
private:
    void initStoreStatus();

private:
    quint16 cabinet_id_; // 鞋柜ID
    CabinetData data_;
    quint8 store_num_; // 仓位数量
    QVector<quint16> store_shoe_id_; // 仓位存储的铁鞋id
    QMap<quint8, std::shared_ptr<IconShoe>> store_map_; // key是仓位序号 1~N

    QPointF pos_; // 经纬度
};



class DeviceManager: public QObject
{
    Q_OBJECT
public:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager();
    void updateCabinet() {
        emit shoeCabinetUpdated(cabinet_Map_);
    }
    void updateIconShoe() {
        emit iconShoeUpdated(shoe_map_);
    }

    void loadConfig();

public slots:
    // 接收tcp数据
    void updateCabinetStatus(const QList<CabinetData>& data);
    void updateShoeStatus(const QList<ShoeData>& data);

signals:
    void shoeCabinetUpdated(const QMap<quint16, std::shared_ptr<ShoeCabinet>>& data);
    void iconShoeUpdated(const QMap<quint16, std::shared_ptr<IconShoe>>& data);

private:
    QMap<quint16, std::shared_ptr<ShoeCabinet>> cabinet_Map_;
    QMap<quint16, std::shared_ptr<IconShoe>> shoe_map_;
};

#endif // DEVICEMANAGER_H
