#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class NetworkConfigPage : public QWidget {
    Q_OBJECT
public:
    NetworkConfigPage(QWidget *parent = nullptr);
    QString getIP() const;
    int getPort() const;

signals:
    void connectRequested(const QString &ip, int port);
    void disconnectRequested();

private:
    QLineEdit *ipEdit;
    QLineEdit *portEdit;
    QPushButton *connectBtn;
    QPushButton *disconnectBtn;
    QLabel *statusLabel;
};
