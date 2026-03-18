#include "MainWindow.h"
#include <QApplication>
#include <QStatusBar>
#include <QLabel>

#include "protocol/TimeSyncResponse.h"
#include "protocol/WhitelistSync.h"

#include <QElapsedTimer>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    networkManager = new NetworkManager(this);
    m_databaseManager = new DatabaseManager(this);

    device_mgr_ = new DeviceManager(this);
    device_mgr_->loadConfig();

    setupUI();
    setupStatusBar();

    connect(device_mgr_, &DeviceManager::shoeCabinetUpdated, cabinetPage, &ShoeCabinetPage::updateFromDeviceManager);
    connect(device_mgr_, &DeviceManager::iconShoeUpdated, tieShoePage, &TieShoePage::updateFromDeviceManager);
    connect(networkManager, &NetworkManager::cabinetData, device_mgr_, &DeviceManager::updateCabinetStatus);
    connect(networkManager, &NetworkManager::shoeData, device_mgr_, &DeviceManager::updateShoeStatus);

    connect(mapPage, &RailMapViewerWidget::getGeoFence, m_databaseManager, &DatabaseManager::handleGetGeoFence);
    connect(mapPage, &RailMapViewerWidget::saveGeoFence, m_databaseManager, &DatabaseManager::handleSaveGeoFence);
    connect(mapPage, &RailMapViewerWidget::clearGeoFence, m_databaseManager, &DatabaseManager::handleClearGeoFence);

    connect(m_databaseManager, &DatabaseManager::geoFenceData, mapPage, &RailMapViewerWidget::handleIncomingFencePoint);

    connect(networkManager, &NetworkManager::stateChanged, this, &MainWindow::updateNetworkStatus);
    connect(networkManager, &NetworkManager::serverDiscovered, this, [this](const QString &ip, int port) {
        // 可选：提示用户已发现服务器
    });

    connect(networkManager, &NetworkManager::cabinetData, mapPage, &RailMapViewerWidget::updateCabinets);
    connect(networkManager, &NetworkManager::shoeData, mapPage, &RailMapViewerWidget::updateShoes);

    m_databaseManager->initDatabase();

    // 启动自动发现
    networkManager->startDiscovery();

    mapPage->loadGeoFence();

    device_mgr_->updateCabinet();
    device_mgr_->updateIconShoe();
}

void MainWindow::applyFlatStyle() {
    QString style = R"(
        QMainWindow {
            background-color: #f5f7fa;
        }

        /* 导航按钮通用样式 */
        QPushButton {
            border: none;
            padding: 12px 16px;
            /*text-align: left;*/
            font-size: 14px;
            border-radius: 6px;
            background-color: transparent;
            color: #4a5568;
            outline: none;
        }

        /* 悬停效果 */
        QPushButton:hover {
            background-color: #e2e8f0;
        }

        /* 选中状态（高亮） */
        QPushButton:checked {
            background-color: #3182ce;
            color: white;
        }

        /* 退出按钮特殊样式 */
        QPushButton#exitButton {
            background-color: #e53e3e;
            color: white;
            font-weight: bold;
        }
        QPushButton#exitButton:hover {
            background-color: #c53030;
        }

        /* 页面内容背景 */
        QStackedWidget {
            background: #f5f7fa;
            border: none;
        }

        QMessageBox {
            background-color: #f5f7fa;
            border: 1px solid #d0d0d0;
            border-radius: 8px;
        }
        QMessageBox QLabel {
            color: #333;
            font-size: 13px;
            padding: 8px 16px;
        }
        QMessageBox QPushButton {
            width: 80px;
            padding: 6px 12px;
            border: none;
            border-radius: 4px;
            background: #4A90E2;
            color: white;
            font-weight: bold;
        }
        QMessageBox QPushButton:hover {
            background: #357ABD;
        }
        QMessageBox QPushButton:pressed {
            background: #2C66A3;
        }
    )";

    // 给退出按钮设置 objectName 以便单独样式
    exitBtn->setObjectName("exitButton");

    this->setStyleSheet(style);
}

void MainWindow::setupUI() {
    // 创建页面
    mapPage = new RailMapViewerWidget(this);
    tieShoePage = new TieShoePage(this);
    cabinetPage = new ShoeCabinetPage(this);
    netPage = new NetworkConfigPage(this);

    contentStack = new QStackedWidget;
    contentStack->addWidget(mapPage);
    contentStack->addWidget(tieShoePage);
    contentStack->addWidget(cabinetPage);
    contentStack->addWidget(netPage);

    // 创建导航按钮（用于互斥选中）
    mapBtn = new QPushButton("地图监控");
    tieShoeBtn = new QPushButton("铁鞋管理");
    cabinetBtn = new QPushButton("鞋柜管理");
    netBtn = new QPushButton("网络设置");

    // 设置按钮为可选中（checkable），实现互斥
    mapBtn->setCheckable(true);
    tieShoeBtn->setCheckable(true);
    cabinetBtn->setCheckable(true);
    netBtn->setCheckable(true);

    // 默认选中第一个
    mapBtn->setChecked(true);

    // 连接切换逻辑
    connect(mapBtn, &QPushButton::clicked, this, [this]() { switchPage(0); });
    connect(tieShoeBtn, &QPushButton::clicked, this, [this]() { switchPage(1); });
    connect(cabinetBtn, &QPushButton::clicked, this, [this]() { switchPage(2); });
    connect(netBtn, &QPushButton::clicked, this, [this]() { switchPage(3); });

    connect(netPage, &NetworkConfigPage::connectRequested,
            this, &MainWindow::onConnectRequested);

    connect(netPage, &NetworkConfigPage::disconnectRequested,
            this, &MainWindow::onDisconnectRequested);

    // 退出按钮（不可选中，独立位置）
    exitBtn = new QPushButton("退出系统");
    connect(exitBtn, &QPushButton::clicked, this, &MainWindow::confirmExit);

    // 导航按钮容器（上半部分）
    QWidget *navButtonArea = new QWidget;
    QVBoxLayout *navLayout = new QVBoxLayout(navButtonArea);
    navLayout->setSpacing(8);
    navLayout->setContentsMargins(0, 0, 0, 0);
    navLayout->addWidget(mapBtn);
    navLayout->addWidget(tieShoeBtn);
    navLayout->addWidget(cabinetBtn);
    navLayout->addWidget(netBtn);
    navLayout->addStretch(); // 推开下方的退出按钮

    // 左侧整体容器：导航 + 退出
    QWidget *leftSidebar = new QWidget;
    leftSidebar->setFixedWidth(140); // 固定宽度
    QVBoxLayout *sidebarLayout = new QVBoxLayout(leftSidebar);
    sidebarLayout->setContentsMargins(10, 10, 10, 10);
    sidebarLayout->setSpacing(0);
    sidebarLayout->addWidget(navButtonArea, 1); // 占据上部空间
    sidebarLayout->addWidget(exitBtn, 0, Qt::AlignBottom); // 固定在底部

    // 主内容区
    QWidget *centralArea = new QWidget;
    QHBoxLayout *mainLayout = new QHBoxLayout(centralArea);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(leftSidebar, 0);
    mainLayout->addWidget(contentStack, 1); // 自动拉伸

    setCentralWidget(centralArea);
    switchPage(0);

    // 应用扁平化样式表
    applyFlatStyle();
}

void MainWindow::switchPage(int index) {
    contentStack->setCurrentIndex(index);

    // 同步按钮选中状态
    mapBtn->setChecked(index == 0);
    tieShoeBtn->setChecked(index == 1);
    cabinetBtn->setChecked(index == 2);
    netBtn->setChecked(index == 3);
}

void MainWindow::handleTcpMessage(const QVariantMap &msg) {
    QString device = msg.value("device").toString();
    double lat = msg.value("lat").toDouble();
    double lon = msg.value("lon").toDouble();
    if (!device.isEmpty() && qIsFinite(lat) && qIsFinite(lon)) {
        mapPage->addDeviceMarker(device, lat, lon);
    }
}

void MainWindow::onConnectRequested(const QString &ip, int port) {
    qDebug() << ip << " " << port;
    // if (tcpClient->connectToServer(ip, port)) {
    //     QMessageBox::information(this, "成功", "已连接到服务器");
    // } else {
    //     QMessageBox::warning(this, "失败", "连接失败");
    // }
}

void MainWindow::onDisconnectRequested() {
    qDebug() << "disconnect";
    // tcpClient->disconnectFromServer();
    QMessageBox::information(this, "提示", "已断开连接");
}

void MainWindow::confirmExit() {
    if (QMessageBox::question(this, "确认退出", "确定要退出系统吗？") == QMessageBox::Yes) {
        QApplication::quit();
    }
}

void MainWindow::setupStatusBar() {
    statusLabel = new QLabel("就绪");
    statusLabel->setStyleSheet("color: #666; padding: 4px 8px;");
    statusBar()->addWidget(statusLabel);
}

QString stateToString(NetworkManager::ConnectionState state) {
    switch (state) {
    case NetworkManager::Idle: return "空闲";
    case NetworkManager::Discovering: return "服务器查询中...";
    case NetworkManager::Connecting: return "连接中...";
    case NetworkManager::Connected: return "已连接";
    case NetworkManager::Disconnected: return "已断开";
    }
    return "未知";
}

void MainWindow::updateNetworkStatus(NetworkManager::ConnectionState state) {
    statusLabel->setText("网络状态: " + stateToString(state));
    if (state == NetworkManager::Connected) {
        statusLabel->setStyleSheet("color: green; font-weight: bold; padding: 4px 8px;");
        {
            // 连接成功后主动发送时间同步给基站
            // qint64 secs = QDateTime::currentSecsSinceEpoch();
            // qDebug() << "Unix timestamp (seconds):" << secs;
            // QDateTime localTime = QDateTime::fromSecsSinceEpoch(secs);
            // qDebug() << "Local time:" << localTime.toString("yyyy-MM-dd HH:mm:ss");
            // TimeSyncResponse response(0x00, secs);
            // networkManager->sendRequest(response);

            std::vector<quint32> whitelist;
            whitelist.push_back(0x12345678);
            whitelist.push_back(0x34567890);
            whitelist.push_back(0x56789ABC);
            whitelist.push_back(0x789ABCDE);
            whitelist.push_back(0x87654321);
            WhitelistSync response(0x00, whitelist);
            networkManager->sendRequest(response);
        }
    } else if (state == NetworkManager::Discovering || state == NetworkManager::Connecting) {
        statusLabel->setStyleSheet("color: orange; padding: 4px 8px;");
    } else {
        statusLabel->setStyleSheet("color: red; padding: 4px 8px;");
    }
}
