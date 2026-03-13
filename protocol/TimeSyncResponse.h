#ifndef TIMESYNCRESPONSE_H
#define TIMESYNCRESPONSE_H

#include "ProtocolPacket.h"

class TimeSyncResponse : public ProtocolPacket
{
public:
    explicit TimeSyncResponse(quint16 deviceId = 0, quint32 unixTime = 0);

    void setUnixTime(quint32 time) { m_unixTime = time; }
    quint32 unixTime() const { return m_unixTime; }

    QByteArray pack() const override;
    bool unpack(const QByteArray &fullPacket) override;

private:
    quint32 m_unixTime = 0;
};

#endif // TIMESYNCRESPONSE_H
