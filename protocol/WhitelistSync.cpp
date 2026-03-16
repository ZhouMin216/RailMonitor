#include "WhitelistSync.h"
#include <QIODevice>
#include <QDebug>

WhitelistSync::WhitelistSync(quint16 deviceId, std::vector<quint32> whiteList):m_whiteList(whiteList)
{
    m_byDevType = static_cast<quint8>(PC_END);
    m_byDevID = deviceId;
}

QByteArray WhitelistSync::pack() const
{
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream << quint8(0xAA);
    stream << quint8(static_cast<quint8>(PC_END));
    stream << m_byDevID;
    stream << quint8(static_cast<quint8>(DT_WHITE_LIST_CONFIG));
    stream << quint16(static_cast<quint16>(m_whiteList.size() * 4 + 1)); // 数据长度 字节数 +1 是白名单个数字节

    stream << quint8(static_cast<quint8>(m_whiteList.size()));  // 白名单数量个数
    for(const auto& item : m_whiteList){
        stream << item;
    }
    quint8 xorCheck = calculateXor(packet);
    qDebug() << "xorCheck: " <<  xorCheck;
    stream << xorCheck;
    stream << quint8(0x55);

    return packet;
}

bool WhitelistSync::unpack(const QByteArray &fullPacket)
{
    if (!parseCommonFields(fullPacket)) {
        return false;
    }

    // if (m_wPackLen != 4) {
    //     return false;
    // }

    // QDataStream stream(m_abyData);
    // stream.setByteOrder(QDataStream::LittleEndian);
    // stream >> m_unixTime;

    return true;
}
