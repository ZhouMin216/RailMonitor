#ifndef DEVICEPARSER_H
#define DEVICEPARSER_H

#include "ProtocolPacket.h"
#include <QList>

enum CabinetStatus{Unusual = 0x00, Offline = 0x01, Online = 0x02};
enum PosQuality{
    NoPos = 0x00, SinglepointPos = 0x01, DiffPos = 0x02,
    PPSpos = 0x03, RTKfixed = 0x04, RTKfloating = 0x05 // RTKfloating 定位质量最好
};
// 铁鞋数据结构体
struct ShoeData {
    quint16 wDevID;         // 设备 ID (U16)
    quint8 byBatVal;        // 电量值
    PosQuality byPosQuality;    // 位置质量
    quint8 byStarNum;       // 卫星数量
    double lng;              // 经度
    double lat;              // 纬度
};

// 柜子数据结构体
struct CabinetData {
    quint16 wDevID;        // 设备 ID (U16)
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
