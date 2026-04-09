#ifndef WHITELISTSYNC_H
#define WHITELISTSYNC_H

#include <QString>
#include "ProtocolPacket.h"

struct WhitelistEntry {
    quint32 uid;
    QString name;
    QString department;
    bool operator==(const WhitelistEntry& other) const {
        return uid == other.uid && name == other.name && department == other.department;
    }
};

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
