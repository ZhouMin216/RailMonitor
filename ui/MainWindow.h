#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include "RailMapViewerWidget.h"
#include "TieShoePage.h"
#include "ShoeCabinetPage.h"
#include "NetworkConfigPage.h"
#include "network/NetworkManager.h"
#include "database/DatabaseManager.h"
#include "DeviceManager.h"
#include "LoginDialog.h"
#include "WhiteListPage.h"
#include "ui/DataInventoryPage.h"
#include "EventPage.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void switchPage(int index);
    void handleTcpMessage(const QVariantMap &msg);
    void onConnectRequested(const QString &ip, int port);
    void onDisconnectRequested();
    void confirmExit();
    void updateNetworkStatus(NetworkManager::ConnectionState state);
    void updateTime();

private:
    void applyFlatStyle();
    void setupUI();
    void setupStatusBar();

    QStackedWidget *contentStack;
    RailMapViewerWidget *mapPage;
    TieShoePage *tieShoePage;
    ShoeCabinetPage *cabinetPage;
    NetworkConfigPage *netPage;
    WhiteListPage *whiteListPage_;
    DataInventoryPage *dataInventoryPage_;
    EventPage *eventPage_;

    NetworkManager *networkManager;
    DatabaseManager *m_databaseManager;
    QLabel *statusLabel; // 用于显示状态

    // DeviceManager* device_mgr_;

    QPushButton *mapBtn, *tieShoeBtn, *cabinetBtn, *netBtn, *whiteListBtn_, *dataInventoryBtn_, *eventPageBtn_, *exitBtn;

    LoginDialog *loginDialog;
    QLabel *timeLabel; // 时间显示标签
};
