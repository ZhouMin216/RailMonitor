// ProtocolPacket.h
#ifndef PROTOCOLPACKET_H
#define PROTOCOLPACKET_H

#include <QByteArray>
#include <QDataStream>

class LittleEndianReader
{
public:
    // 从 buf 的 offset 处读取小端 uint16
    static quint16 readUInt16(const QByteArray& buf, int offset, bool* ok = nullptr) {
        if (ok) *ok = true;
        if (offset + 1 >= buf.size()) {
            if (ok) *ok = false;
            return 0;
        }
        return static_cast<quint8>(buf[offset]) |
               (static_cast<quint8>(buf[offset + 1]) << 8);
    }

    // 从 buf 的 offset 处读取小端 uint32
    static quint32 readUInt32(const QByteArray& buf, int offset, bool* ok = nullptr) {
        if (ok) *ok = true;
        if (offset + 3 >= buf.size()) {
            if (ok) *ok = false;
            return 0;
        }
        return static_cast<quint8>(buf[offset])         |
               (static_cast<quint8>(buf[offset + 1]) << 8)  |
               (static_cast<quint8>(buf[offset + 2]) << 16) |
               (static_cast<quint8>(buf[offset + 3]) << 24);
    }

    // 可选：带边界检查的便捷重载（返回是否成功）
    static bool tryReadUInt16(const QByteArray& buf, int offset, quint16& out) {
        if (offset + 1 >= buf.size()) return false;
        out = readUInt16(buf, offset);
        return true;
    }

    static bool tryReadUInt32(const QByteArray& buf, int offset, quint32& out) {
        if (offset + 3 >= buf.size()) return false;
        out = readUInt32(buf, offset);
        return true;
    }
};

class ProtocolPacket
{
public:
    enum DeviceType {
        PC_END = 0x00,
        BASE_STATION = 0x01,
        IRON_CABINET = 0x02,
        IRON_SHOE = 0x03
    };

    enum DataType {
        DT_HANDSHAKE = 0x00,
        DT_TIME_SYNC = 0x01,
        DT_WHITE_LIST_CONFIG = 0x02,
        DT_EVENT_DATA = 0x03,
        DT_STATUS_DATA = 0x04
    };

    virtual ~ProtocolPacket() = default;

    // 公共字段访问
    quint8 deviceType() const { return m_byDevType; }
    quint16 deviceId() const { return m_byDevID; }
    quint16 dataLength() const { return m_wPackLen; }
    QByteArray rawData() const { return m_abyData; }

    // 虚函数：子类必须实现
    virtual QByteArray pack() const = 0;
    virtual bool unpack(const QByteArray &fullPacket) = 0;

    // 静态工具：验证完整帧（含校验）
    static bool validateFrame(const QByteArray &data);

    // 解析公共字段（供子类调用）
    bool parseCommonFields(const QByteArray &fullPacket);

protected:
    quint8 m_byDevType = 0;
    quint16 m_byDevID = 0;
    quint16 m_wPackLen = 0;
    QByteArray m_abyData;

protected:
    static quint8 calculateXor(const QByteArray &data);
};

#endif // PROTOCOLPACKET_H
