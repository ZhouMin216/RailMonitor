#include "TimeSyncRequest.h"
#include <QIODevice>
#include <QDebug>

TimeSyncRequest::TimeSyncRequest(quint16 deviceId, quint32 unixTime)
    : m_unixTime(unixTime)
{
    m_byDevType = static_cast<quint8>(PC_END);
    m_byDevID = deviceId;
}

QByteArray TimeSyncRequest::pack() const
{
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream << quint8(0xAA);
    stream << quint8(static_cast<quint8>(PC_END));
    stream << m_byDevID;
    stream << quint8(static_cast<quint8>(DT_TIME_SYNC));
    stream << quint16(4);
    stream << m_unixTime;

    quint8 xorCheck = calculateXor(packet);
    stream << xorCheck;
    stream << quint8(0x55);

    return packet;
}

bool TimeSyncRequest::unpack(const QByteArray &fullPacket)
{
    if (!parseCommonFields(fullPacket)) {
        qDebug() << "parseCommonFields error ";
        return false;
    }

    // 检查类型和长度
    if (m_byDevType != static_cast<quint8>(PC_END) ||
        m_wPackLen != 4) {
        return false;
    }

    QDataStream stream(m_abyData);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> m_unixTime;

    return true;
}
