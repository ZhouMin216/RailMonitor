#include "ProtocolPacket.h"
#include <QIODevice>
#include <QDebug>
/*
 U8 comm_xor_calc(U8 *pbyData, U32 dwLen)
{
U8 byRet = 0x00U;
U16 i;
for(i = 0U; i < dwLen; i++)
{
byRet += pbyData[i];
}
return byRet;
}
 */

quint8 ProtocolPacket::calculateXor(const QByteArray &data)
{
    quint8 xorSum = 0;
    for (quint8 byte : data) {
        xorSum ^= byte;
    }
    return xorSum;
}

bool ProtocolPacket::validateFrame(const QByteArray &data)
{
    if (data.size() < 9) {
        qDebug() << "data.size() < 9";
        return false;
    }

    QDataStream stream(data);
    stream.setByteOrder(QDataStream::LittleEndian);

    quint8 head;
    stream >> head;
    if (head != 0xAA) {
        qDebug() << "head != 0xAA";
        return false;
    }

    // 校验码位置：倒数第2字节
    quint8 receivedXor = static_cast<quint8>(data[data.size() - 2]);
    quint8 tail = static_cast<quint8>(data[data.size() - 1]);
    if (tail != 0x55) {
        qDebug() << "tail != 0x55";
        return false;
    }

    // return true; // 测试，不做crc校验

    QByteArray checkData = data.left(data.size() - 2); // 去掉帧尾和接收到的异或值
    quint8 expectedXor = calculateXor(checkData);
    qDebug() << "expectedXor: " << expectedXor << " receivedXor: " << receivedXor;
    return receivedXor == expectedXor;
}

bool ProtocolPacket::parseCommonFields(const QByteArray &fullPacket)
{
    if (!validateFrame(fullPacket)) {
        qDebug() << "validateFrame error ";
        return false;
    }

    QDataStream stream(fullPacket);
    stream.setByteOrder(QDataStream::LittleEndian);

    quint8 head;
    stream >> head; // 0xAA

    stream >> m_byDevType;
    stream >> m_byDevID;
    quint8 dataType;
    stream >> dataType; // 子类需自行检查
    stream >> m_wPackLen;

    // 检查类型
    if (m_byDevType != static_cast<quint8>(BASE_STATION)) {
        qDebug() << "m_byDevType error ";
        return false;
    }

    if (m_wPackLen > 0) {
        m_abyData.resize(m_wPackLen);
        stream.readRawData(m_abyData.data(), m_wPackLen);
    } else {
        m_abyData.clear();
    }

    // 校验和帧尾已在 validateFrame 中验证，此处跳过
    return true;
}
