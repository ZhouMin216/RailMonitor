#include "DeviceParser.h"
#include <QDebug>

// ShoeStatus -> string
QString EnumtoString(ShoeStatus status) {
    switch (status) {
    case ShoeStatus::Offline: return QStringLiteral("离线");
    case ShoeStatus::Online:  return QStringLiteral("在线");
    case ShoeStatus::InCabinet:  return QStringLiteral("在柜");
    case ShoeStatus::Unregister:  return QStringLiteral("未注册");
    case ShoeStatus::NoEnter:  return QStringLiteral("未录入");
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
    case StorageStatus::Unusual:  return QStringLiteral("仓位异常");
    case StorageStatus::Unregister: return QStringLiteral("未注册");
    case StorageStatus::PosFault: return QStringLiteral("错位");
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

inline double sf_extract_to_wgs84(double value)
{
    int deg = static_cast<int>(value);
    double min = (value - static_cast<double>(deg)) * 100.0;

    return deg + min / 60.0;
}

double convertDmToDecimal(qint16 degrees, qint32 minutes) {
    // return degrees + minutes / 60.0;
    QString tmp = QString("0.%1").arg(minutes);
    float value = tmp.toDouble();
    // return degrees + value;

    // qDebug() << " ================= protocol value: " <<  degrees + value  << " ------------------ ";
    return sf_extract_to_wgs84(degrees + value);
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

    // 检查类型
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

        // 读取仓位数(和配置文件中实际绑定的铁鞋数量相等)
        cabinet.byStoreNum = static_cast<quint8>(m_abyData[offset++]);

        // 读取仓位状态（共 byStoreNum 组状态）
        // 仓位状态数据由仓位状态(quint8)和铁鞋id(quint16)构成
        quint16 status_data_size = cabinet.byStoreNum * (sizeof(quint8) + sizeof(quint16));
        if (offset + status_data_size > m_abyData.size()) {
            qDebug() << "Not enough data for status array. status_data_size:" << status_data_size;
            return false;
        }
        QList<QPair<quint16, StorageStatus>> statusList;
        int idx = offset;
        while(idx < offset + status_data_size){
            StorageStatus status = static_cast<StorageStatus>(m_abyData[idx++]);
            quint16 shoeId = 0;
            if (!LittleEndianReader::tryReadUInt16(m_abyData, idx, shoeId)) {
                qDebug() << "parse shoeId error";
                return false;
            }
            idx += 2;
            qDebug() << "shoeId:" << shoeId << " -> status:" << static_cast<quint8>(status);
            statusList.append(qMakePair(shoeId, status));
        }
        // cabinet.abyStatus = m_abyData.mid(offset, cabinet.byStoreNum);
        // offset += cabinet.byStoreNum;
        offset += status_data_size;
        cabinet.storeStatus = statusList;

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
        shoe.byOnline = static_cast<ShoeStatus>(m_abyData[offset++]);

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
        qDebug() << " ================= converted shoe.lng " << QString::number(shoe.lng, 'f', 6) << " ------------------ ";

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
        qDebug() << " ================= converted shoe.lat " <<  QString::number(shoe.lat, 'f', 6) << " ------------------ ";

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
        qDebug() << "wDevID: " << cabinet.wDevID
                 << " byStoreNum: " << cabinet.byStoreNum;
        for (const auto& pair : cabinet.storeStatus) {
            qDebug() << "shoeId:" << pair.first << " -> status:" << static_cast<quint8>(pair.second);

        }
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
