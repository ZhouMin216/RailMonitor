#include "NetworkManager.h"

#include <QDateTime>
#include <QDebug>
#include <QtEndian>
#include <QNetworkInterface>
#include <QNetworkAddressEntry>

#include "protocol/TimeSyncResponse.h"
#include "protocol/TimeSyncRequest.h"
#include "protocol/DeviceParser.h"

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent),
    m_udpSocket(new QUdpSocket(this)),
    m_tcpSocket(new QTcpSocket(this)),
    m_discoveryTimer(new QTimer(this)),
    m_discoveryTimeoutTimer(new QTimer(this)),
    m_reconnectTimer(new QTimer(this))
{
    // 绑定到任意地址和端口（用于接收服务器回复）
    m_udpSocket->bind(QHostAddress::Any, 0); // 临时端口即可

    connect(m_udpSocket, &QUdpSocket::readyRead, this, &NetworkManager::onUdpReadyRead);

    // TCP 信号
    connect(m_tcpSocket, &QTcpSocket::connected, this, &NetworkManager::onTcpConnected);
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, &NetworkManager::onTcpDisconnected);
    connect(m_tcpSocket, &QTcpSocket::errorOccurred, this, &NetworkManager::onTcpError);
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &NetworkManager::onTcpReadyRead);

    // 定时器
    connect(m_discoveryTimer, &QTimer::timeout, this, &NetworkManager::startDiscovery);
    connect(m_discoveryTimeoutTimer, &QTimer::timeout, this, &NetworkManager::onDiscoveryTimeout);
    connect(m_reconnectTimer, &QTimer::timeout, this, &NetworkManager::onReconnectTimer);

    m_reconnectTimer->setSingleShot(true);
}

NetworkManager::~NetworkManager() {
    // stop();
    m_discoveryTimer->stop();
    m_discoveryTimeoutTimer->stop();
    m_reconnectTimer->stop();

    if (m_tcpSocket){
        disconnect(m_tcpSocket, nullptr, this, nullptr);
        m_tcpSocket->disconnectFromHost();
    }

}

void NetworkManager::startDiscovery() {
    if (m_state == Connected || m_state == Connecting) return;

    setState(Discovering);
    m_discoveryTimeoutTimer->start(DISCOVERY_TIMEOUT_MS);

    QByteArray msg = "HZQYSWSJ_DISCOVER";
    bool sentAtLeastOne = false;

    // 遍历所有网络接口
    for (const QNetworkInterface &interface : QNetworkInterface::allInterfaces()) {
        // 跳过无效、回环、未启用的接口
        if (!interface.isValid() ||
            (interface.flags() & QNetworkInterface::IsLoopBack) ||
            !(interface.flags() & QNetworkInterface::IsUp) ||
            !(interface.flags() & QNetworkInterface::IsRunning)) {
            continue;
        }

        // 遍历该接口的所有地址条目（一个接口可能有多个 IP）
        for (const QNetworkAddressEntry &entry : interface.addressEntries()) {
            QHostAddress ip = entry.ip();

            // 只处理 IPv4
            if (ip.protocol() != QAbstractSocket::IPv4Protocol) {
                continue;
            }

            // 获取广播地址（Qt 已计算好！）
            QHostAddress broadcast = entry.broadcast();

            // 检查广播地址是否有效（非空且不是 0.0.0.0）
            if (broadcast.isNull() || broadcast == QHostAddress(QHostAddress::Null)) {
                // 如果 broadcast() 返回 null，手动计算（罕见情况）
                QHostAddress netmask = entry.netmask();
                if (!netmask.isNull()) {
                    quint32 ip4 = ip.toIPv4Address();
                    quint32 mask4 = netmask.toIPv4Address();
                    quint32 broadcast4 = (ip4 & mask4) | (~mask4);
                    broadcast = QHostAddress(broadcast4);
                }
            }

            if (!broadcast.isNull() && broadcast != QHostAddress("0.0.0.0")) {
                qint64 sent = m_udpSocket->writeDatagram(msg, broadcast, BROADCAST_PORT);
                if (sent > 0) {
                    qDebug() << "Sent discovery to broadcast address"
                             << broadcast.toString()
                             << "via interface" << interface.name();
                    sentAtLeastOne = true;
                } else {
                    qDebug() << "Failed to send to" << broadcast.toString()
                    << ":" << m_udpSocket->errorString();
                }
            }
        }
    }

    if (!sentAtLeastOne) {
        emit errorOccurred("未能向任何子网发送广播发现包");
        onDiscoveryTimeout(); // 或其他错误处理
    }
}

void NetworkManager::stop() {
    m_discoveryTimer->stop();
    m_discoveryTimeoutTimer->stop();
    m_reconnectTimer->stop();
    m_tcpSocket->disconnectFromHost();
    setState(Idle);
}

void NetworkManager::manualConnect(const QString &ip, int port) {
    stop(); // 停止自动发现
    connectToServer(ip, port);
}

void NetworkManager::onUdpReadyRead() {
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        QHostAddress sender;
        quint16 senderPort;

        datagram.resize(m_udpSocket->pendingDatagramSize());
        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        // 假设服务器回复格式为 "HZQYSWSJ_RESPONSE:PORT"
        QString start_msg = "HZQYSWSJ_RESPONSE:";
        if (datagram.startsWith("HZQYSWSJ_RESPONSE:")) {
            bool ok;
            int port = datagram.mid(start_msg.size()).toInt(&ok);
            if (ok && port > 0 && port < 65536) {
                QString ip = sender.toString();
                qDebug() << "Server discovered via broadcast at" << ip << ":" << port;

                m_discoveryTimer->stop();
                m_discoveryTimeoutTimer->stop();
                emit serverDiscovered(ip, port);
                connectToServer(ip, port);
                return;
            }
        }
    }
}

void NetworkManager::connectToServer(const QString &ip, int port) {
    if (m_state == Connecting || m_state == Connected) return;

    m_serverAddress = ip;
    m_serverPort = port;
    setState(Connecting);
    m_tcpSocket->connectToHost(ip, port);
}

void NetworkManager::onTcpConnected() {
    resetReconnect();
    setState(Connected);
    qDebug() << "TCP connected to" << m_serverAddress << ":" << m_serverPort;
}

void NetworkManager::onTcpDisconnected() {
    qDebug() << "TCP disconnected" << m_state;
    if (m_state == Connected) {
        setState(Disconnected);
        scheduleReconnect();
    } else if (m_state == Connecting) {
        // 连接失败，可能是无效地址，回到发现
        setState(Disconnected);
        startDiscovery(); // 或者等重连失败后再发现
    }
}

void NetworkManager::onTcpError(QAbstractSocket::SocketError error) {
    Q_UNUSED(error);
    QString errStr = m_tcpSocket->errorString();
    qDebug() << "TCP Error:" << errStr;
    emit errorOccurred(errStr);
    onTcpDisconnected(); // 触发重连逻辑
}

void NetworkManager::scheduleReconnect() {
    if (m_reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
        m_reconnectAttempts++;
        int delay = 2000 * m_reconnectAttempts; // 指数退避
        m_reconnectTimer->start(delay);
        qDebug() << "Scheduling reconnect attempt" << m_reconnectAttempts << "in" << delay << "ms";
    } else {
        // 重连失败，重新开始组播发现
        resetReconnect();
        startDiscovery();
    }
}

void NetworkManager::resetReconnect() {
    m_reconnectAttempts = 0;
    m_reconnectTimer->stop();
}

void NetworkManager::onReconnectTimer() {
    if (!m_serverAddress.isEmpty() && m_serverPort > 0) {
        connectToServer(m_serverAddress, m_serverPort);
    }
}

void NetworkManager::onDiscoveryTimeout() {
    // 未收到响应，继续轮询
    if (m_state == Discovering) {
        m_discoveryTimer->start(DISCOVERY_INTERVAL_MS);
    }
}

void NetworkManager::onTcpReadyRead() {
    m_recvBuffer.append(m_tcpSocket->readAll());
    qDebug() << "onTcpReadyRead " << m_recvBuffer.size() << " m_recvBuffer: " << m_recvBuffer.toHex(' ').toUpper();
    while (m_recvBuffer.size() >= 9) { // 最小包长度
        // 检查是否有完整包
        if (static_cast<quint8>(m_recvBuffer[0]) != 0xAA) {
            // 帧头错误，丢弃一个字节（同步）
            qDebug() << "m_recvBuffer[0]: " << static_cast<quint8>(m_recvBuffer[0]);
            m_recvBuffer.remove(0, 1);
            continue;
        }

        if (m_recvBuffer.size() < 9) break;

        // 尝试读取 wPackLen（偏移 6 字节：AA + DevType + DevID(2) + DataType + wPackLen(2)）
        // quint16 rawWord = *reinterpret_cast<const quint16*>(m_recvBuffer.constData() + 5);
        // quint16 dataLen = qFromLittleEndian(rawWord);
        quint16 dataLen = 0;
        if (!LittleEndianReader::tryReadUInt16(m_recvBuffer, 5, dataLen)) {
            qDebug() << "parse data len error";
            break;
        }

        int totalLen = 9 + dataLen; // AA + ... + XOR + 55
        qDebug() << "dataLen:" << dataLen
                 << "totalLen:" << totalLen;
        if (m_recvBuffer.size() < totalLen) {
            qDebug() << "data len error";
            break; // 包未收完
        }

        // 提取完整包
        qDebug() << "m_recvBuffer: " << m_recvBuffer.toHex(' ').toUpper();
        QByteArray packet = m_recvBuffer.left(totalLen);
        m_recvBuffer.remove(0, totalLen);

        // 解析类型
        quint8 devType = static_cast<quint8>(packet[1]);
        quint8 dataType = static_cast<quint8>(packet[4]);

        switch (dataType) {
        case ProtocolPacket::DT_TIME_SYNC: {
            if (devType == static_cast<quint8>(ProtocolPacket::PC_END)) {
                TimeSyncRequest req;
                if (req.unpack(packet)) {
                    TimeSyncResponse response(0x00, QDateTime::currentSecsSinceEpoch());
                    sendRequest(response);
                } else {
                    qDebug() << "m_recvBuffer unpack error ";
                }
            } else {
                TimeSyncResponse resp;
                if (resp.unpack(packet)) {
                    emit timeSyncReceived(resp.unixTime(), resp.deviceId());
                }
            }
            break;
        }
        case ProtocolPacket::DT_EVENT_DATA: {
            break;
        }
        case ProtocolPacket::DT_STATUS_DATA: {
            DeviceParser device(0x00);
            if (device.unpack(packet)) {
                auto shoes = device.getShoeData();
                auto cabinets = device.getCabinetData();
                if (!cabinets.empty()) {
                    emit cabinetData(cabinets);
                }
                if (!shoes.empty()) {
                    emit shoeData(shoes);
                }
            } else {
                qDebug() << "StatusData: m_recvBuffer unpack error ";
            }
            break;
        }
        // 可扩展其他类型...
        default:
            qDebug() << "Unknown data type:" << dataType;
            break;
        }
    }
}

void NetworkManager::sendTcpMessage(const QByteArray &message) {
    if (m_state != Connected) {
        emit errorOccurred("Cannot send: not connected to server");
        return;
    }

    qint64 bytesSent = m_tcpSocket->write(message);
    if (bytesSent == -1) {
        emit errorOccurred("Failed to send TCP message: " + m_tcpSocket->errorString());
    } else {
        qDebug() << "Sent TCP message (" << bytesSent << " bytes):" << message;
    }
}

void NetworkManager::sendRequest(const ProtocolPacket &packet)
{
    if (m_state != Connected) {
        emit errorOccurred("Cannot send: not connected");
        return;
    }

    QByteArray data = packet.pack();
    qint64 sent = m_tcpSocket->write(data);
    if (sent == -1) {
        emit errorOccurred("Send failed: " + m_tcpSocket->errorString());
    } else {
        qDebug() << "Sent request:" << data.toHex();
    }
}

void NetworkManager::setState(ConnectionState newState) {
    if (m_state != newState) {
        m_state = newState;
        emit stateChanged(m_state);
    }
}
