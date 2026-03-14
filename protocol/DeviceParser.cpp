#include "DeviceParser.h"
#include <QDebug>

double convertDmToDecimal(qint16 degrees, qint32 minutes) {
    // return degrees + minutes / 60.0;
    return degrees + minutes / 10000000.0;
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
