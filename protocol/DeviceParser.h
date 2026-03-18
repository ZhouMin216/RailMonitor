#ifndef DEVICEPARSER_H
#define DEVICEPARSER_H

#include "ProtocolPacket.h"
#include <QList>

// 铁鞋状态
enum class DeviceStatus{Offline = 0x00, Online = 0x01, InCabinet = 0x02, Unregister = 0x03, NoEnter = 0x04};
// 鞋柜状态
enum class CabinetStatus{Offline = 0x00, Online = 0x01, Unregister = 0x02, NoEnter = 0x04};
// 鞋柜仓位状态
enum class StorageStatus{Unusual = 0x00, Offline = 0x01, Online = 0x02, Unregister = 0x03};
enum PosQuality{
    NoPos = 0x00, SinglepointPos = 0x01, DiffPos = 0x02,
    PPSpos = 0x03, RTKfixed = 0x04, RTKfloating = 0x05 // RTKfixed 定位质量最好
};

// DeviceStatus -> string
QString EnumtoString(DeviceStatus status);

// CabinetStatus -> string
QString EnumtoString(CabinetStatus status);

// StorageStatus -> string
QString EnumtoString(StorageStatus status);

// PosQuality -> string
QString EnumtoString(PosQuality quality);

// 铁鞋数据结构体
struct ShoeData {
    quint16 wDevID;         // 设备 ID (U16)
    DeviceStatus byOnline{DeviceStatus::Unregister}; // 在线状态
    quint8 byBatVal{0};        // 电量值
    PosQuality byPosQuality{PosQuality::NoPos};    // 位置质量
    quint8 byStarNum{0};       // 卫星数量
    double lng{0.0};              // 经度
    double lat{0.0};              // 纬度
};

// 柜子数据结构体
struct CabinetData {
    quint16 wDevID;        // 设备 ID (U16)
    CabinetStatus byOnline{CabinetStatus::Unregister}; // 在线状态
    quint8 byStoreNum;     // 仓位数
    QByteArray abyStatus;  // 仓位状态数组
};


class DeviceParser : public ProtocolPacket
{
public:
    explicit DeviceParser(quint16 deviceId);

    QByteArray pack() const override;
    bool unpack(const QByteArray &fullPacket) override;

    void Print();

    const QList<CabinetData>& getCabinetData() { return m_cabinetList;}
    const QList<ShoeData>& getShoeData() { return m_shoeList;}

private:
    quint8 m_byCabinetNum;     // 铁鞋柜数量
    quint16 m_wShoeNum;        // 铁鞋总数
    QList<CabinetData> m_cabinetList;   // 每个柜子的状态
    QList<ShoeData> m_shoeList;         // 每个铁鞋的状态
};



#endif // DEVICEPARSER_H
