#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTimer>
#include <QHostAddress>
#include "protocol/ProtocolPacket.h"
#include "protocol/DeviceParser.h"

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    enum ConnectionState {
        Idle,
        Discovering,      // 正在组播查询
        Connecting,       // 正在 TCP 连接
        Connected,        // 已连接
        Disconnected      // 已断开（可能正在重连）
    };
    Q_ENUM(ConnectionState)

    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();

    ConnectionState state() const { return m_state; }
    QString serverAddress() const { return m_serverAddress; }
    int serverPort() const { return m_serverPort; }

public slots:
    void startDiscovery();
    void stop();
    void manualConnect(const QString &ip, int port);
    void sendTcpMessage(const QByteArray &message);
    void sendRequest(const ProtocolPacket &packet);

signals:
    void stateChanged(ConnectionState state);
    void serverDiscovered(const QString &ip, int port);
    void tcpMessageReceived(const QByteArray &data);
    void errorOccurred(const QString &error);
    void timeSyncReceived(quint32 unixTime, quint16 deviceId);
    void cabinetData(const QList<CabinetData>& data);
    void shoeData(const QList<ShoeData>& data);

private slots:
    void onUdpReadyRead();
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpError(QAbstractSocket::SocketError error);
    void onReconnectTimer();
    void onDiscoveryTimeout();
    void onTcpReadyRead();

private:
    void setState(ConnectionState newState);
    void connectToServer(const QString &ip, int port);
    void scheduleReconnect();
    void resetReconnect();

    QUdpSocket *m_udpSocket;
    QTcpSocket *m_tcpSocket;
    QByteArray m_recvBuffer; // 粘包缓冲区

    QString m_serverAddress;
    int m_serverPort = 0;

    ConnectionState m_state = Idle;

    // 组播配置
    // static const QHostAddress MULTICAST_ADDRESS;
    static const quint16 BROADCAST_PORT = 8888;
    static const int DISCOVERY_INTERVAL_MS = 2000;
    static const int DISCOVERY_TIMEOUT_MS = 5000;

    QTimer *m_discoveryTimer;
    QTimer *m_discoveryTimeoutTimer;
    QTimer *m_reconnectTimer;

    int m_reconnectAttempts = 0;
    static const int MAX_RECONNECT_ATTEMPTS = 3;
};

#endif // NETWORKMANAGER_H
