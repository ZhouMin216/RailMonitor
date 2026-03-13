#ifndef WHITELISTSYNC_H
#define WHITELISTSYNC_H

#include "ProtocolPacket.h"

class WhitelistSync : public ProtocolPacket
{
public:
    explicit WhitelistSync(quint16 deviceId, std::vector<quint32> whiteList);

    QByteArray pack() const override;
    bool unpack(const QByteArray &fullPacket) override;

private:
    std::vector<quint32> m_whiteList;
};

#endif // WHITELISTSYNC_H
