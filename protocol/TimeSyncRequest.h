#ifndef TIMESYNCREQUEST_H
#define TIMESYNCREQUEST_H

#include "ProtocolPacket.h"

class TimeSyncRequest : public ProtocolPacket
{
public:
    explicit TimeSyncRequest(quint16 deviceId = 0, quint32 unixTime = 0);

    void setUnixTime(quint32 time) { m_unixTime = time; }
    quint32 unixTime() const { return m_unixTime; }

    QByteArray pack() const override;
    bool unpack(const QByteArray &fullPacket) override;

private:
    quint32 m_unixTime = 0;
};

#endif // TIMESYNCREQUEST_H
