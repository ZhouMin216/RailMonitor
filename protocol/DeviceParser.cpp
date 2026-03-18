#include "DeviceParser.h"
#include <QDebug>

// DeviceStatus -> string
QString EnumtoString(DeviceStatus status) {
    switch (status) {
    case DeviceStatus::Offline: return QStringLiteral("离线");
    case DeviceStatus::Online:  return QStringLiteral("在线");
    case DeviceStatus::InCabinet:  return QStringLiteral("在柜");
    case DeviceStatus::Unregister:  return QStringLiteral("未注册");
    case DeviceStatus::NoEnter:  return QStringLiteral("未录入");
    default: return QStringLiteral("未知");
    }
}
// CabinetStatus -> string
QString EnumtoString(CabinetStatus status) {
    switch (status) {
    case CabinetStatus::Offline: return QStringLiteral("离线");
    case CabinetStatus::Online:  return QStringLiteral("在线");
    case CabinetStatus::Unregister:  return QStringLiteral("未注册");
    case CabinetStatus::NoEnter:  return QStringLiteral("未录入");
    default: return QStringLiteral("未知");
    }
}
// StorageStatus -> string
QString EnumtoString(StorageStatus status) {
    switch (status) {
    case StorageStatus::Offline: return QStringLiteral("铁鞋离位");
    case StorageStatus::Online:  return QStringLiteral("铁鞋在位");
    case StorageStatus::Unusual:  return QStringLiteral("异常");
    case StorageStatus::Unregister: return QStringLiteral("未注册");
    default: return "未知";
    }
}
// PosQuality -> string
QString EnumtoString(PosQuality quality) {
    switch (quality) {
    case NoPos:           return QStringLiteral("无定位");
    case SinglepointPos:  return QStringLiteral("单点定位");
    case DiffPos:         return QStringLiteral("差分定位");
    case PPSpos:          return QStringLiteral("PPS定位");
    case RTKfixed:        return QStringLiteral("RTK固定");
    case RTKfloating:     return QStringLiteral("RTK浮点");
    default:              return QStringLiteral("未知");
    }
}

double convertDmToDecimal(qint16 degrees, qint32 minutes) {
    // return degrees + minutes / 60.0;
    QString tmp = QString("0.%1").arg(minutes);
    qDebug() << " ================= tmp " <<  tmp  << " ------------------ ";
    float value = tmp.toDouble();
    // return degrees + minutes / 10000000.0;
    return degrees + value;
}

DeviceParser::DeviceParser(quint16 deviceId)
{
    m_byDevType = static_cast<quint8>(PC_END);
    m_byDevID = deviceId;
}

QByteArray DeviceParser::pack() const
{
    QByteArray packet;
    // QDataStream stream(&packet, QIODevice::WriteOnly);
    // stream.setByteOrder(QDataStream::LittleEndian);

    // stream << quint8(0xAA);
    // stream << quint8(static_cast<quint8>(PC_END));
    // stream << m_byDevID;
    // stream << quint8(static_cast<quint8>(DT_TIME_SYNC));
    // stream << quint16(4);
    // stream << m_unixTime;

    // quint8 xorCheck = calculateXor(packet);
    // stream << xorCheck;
    // stream << quint8(0x55);

    return packet;
}

bool DeviceParser::unpack(const QByteArray &fullPacket)
{
    if (!parseCommonFields(fullPacket)) {
        qDebug() << "parseCommonFields error ";
        return false;
    }

    // 检查类型和长度
    // if (m_byDevType != static_cast<quint8>(BASE_STATION)) {
    //     return false;
    // }

    int offset = 0;

    // 读取柜子数量
    m_byCabinetNum = static_cast<quint8>(m_abyData[offset++]);

    // 读取铁鞋总数
    if (!LittleEndianReader::tryReadUInt16(m_abyData, offset, m_wShoeNum)) {
        qDebug() << "parse shoe number error";
        return false;
    }
    offset += 2;

    int cnt = 0;
    // 读取柜子数据,总共有m_byCabinetNum组数据
    while(offset < m_abyData.size() && cnt < m_byCabinetNum){
        CabinetData cabinet;

        // 读取设备 ID（U16）
        if (!LittleEndianReader::tryReadUInt16(m_abyData, offset, cabinet.wDevID)) {
            qDebug() << "parse cabinet.wDevID error";
            return false;
        }
        offset += 2;

        // 读取设备在线状态
        cabinet.byOnline = static_cast<CabinetStatus>(m_abyData[offset++]);

        // 读取仓位数
        cabinet.byStoreNum = static_cast<quint8>(m_abyData[offset++]);

        // 读取仓位状态（byStoreNum 个字节）
        if (offset + cabinet.byStoreNum > m_abyData.size()) {
            qDebug() << "Error: Not enough data for status array";
            return false;
        }
        cabinet.abyStatus = m_abyData.mid(offset, cabinet.byStoreNum);
        offset += cabinet.byStoreNum;

        m_cabinetList.append(cabinet);
        cnt++;
    }

    cnt = 0;
    // 读取铁鞋数据,总共有m_wShoeNum组数据
    while(offset < m_abyData.size() && cnt < m_wShoeNum){
        ShoeData shoe;

        // 读取设备 ID（U16）
        if (!LittleEndianReader::tryReadUInt16(m_abyData, offset, shoe.wDevID)) {
            qDebug() << "parse cabinet.wDevID error";
            return false;
        }
        offset += 2;

        // 读取设备在线状态
        shoe.byOnline = static_cast<DeviceStatus>(m_abyData[offset++]);

        // 读取电量值
        shoe.byBatVal = static_cast<quint8>(m_abyData[offset++]);
        // 读取位置质量
        shoe.byPosQuality = static_cast<PosQuality>(m_abyData[offset++]);
        shoe.byStarNum = static_cast<quint8>(m_abyData[offset++]);

        quint16 LngDegree = 0;
        if (!LittleEndianReader::tryReadUInt16(m_abyData, offset, LngDegree)) {
            qDebug() << "parse LngDegree error";
            return false;
        }
        offset += 2;
        quint32 LngMinutes = 0;
        if (!LittleEndianReader::tryReadUInt32(m_abyData, offset, LngMinutes)) {
            qDebug() << "parse LngMinutes error";
            return false;
        }
        offset += 4;
        shoe.lng = convertDmToDecimal(LngDegree, LngMinutes);
        qDebug() << " ================= shoe.lng " << QString::number(shoe.lng, 'f', 6) << "  LngMinutes= " <<LngMinutes << " ------------------ ";

        quint16 LatDegree = 0;
        if (!LittleEndianReader::tryReadUInt16(m_abyData, offset, LatDegree)) {
            qDebug() << "parse LatDegree error";
            return false;
        }
        offset += 2;
        quint32 LatMinutes = 0;
        if (!LittleEndianReader::tryReadUInt32(m_abyData, offset, LatMinutes)) {
            qDebug() << "parse LatMinutes error";
            return false;
        }
        offset += 4;
        shoe.lat = convertDmToDecimal(LatDegree, LatMinutes);
        qDebug() << " ================= shoe.lat " <<  QString::number(shoe.lat, 'f', 6) << "  LatMinutes= " <<LatMinutes << " ------------------ ";

        m_shoeList.append(shoe);
        cnt++;
    }
    Print();
    return true;
}

void DeviceParser::Print()
{
    qDebug() << "===================================";
    qDebug() << "m_byCabinetNum: " << m_byCabinetNum;
    for(const auto& cabinet : m_cabinetList){
        /*
         *     quint16 wDevID;        // 设备 ID (U16)
    quint8 byStoreNum;     // 仓位数
    QByteArray abyStatus;  // 仓位状态数组
         */
        qDebug() << "wDevID: " << cabinet.wDevID
                 << " byStoreNum: " << cabinet.byStoreNum
                 << " abyStatus: " << cabinet.abyStatus.toHex(' ').toUpper();
    }

    qDebug() << " m_wShoeNum: " << m_wShoeNum;
    for(const auto& shoe : m_shoeList){
        /*
         *     quint16 wDevID;        // 设备 ID (U16)
    quint8 byStoreNum;     // 仓位数
    QByteArray abyStatus;  // 仓位状态数组
         */
        qDebug() << "wDevID: " << shoe.wDevID
                 << " byBatVal: " << shoe.byBatVal
                 << " byStarNum: " << shoe.byStarNum
                 << " byPosQuality: " << shoe.byPosQuality
                 << " lng: " << QString::number(shoe.lng, 'f', 6)
                 << " lat: " << QString::number(shoe.lat, 'f', 6);
    }
    qDebug() << "===================================";
}
