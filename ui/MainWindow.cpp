#include "MainWindow.h"
#include <QApplication>
#include <QStatusBar>
#include <QLabel>

#include "protocol/TimeSyncResponse.h"
#include "protocol/WhitelistSync.h"
#include "utils.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    networkManager = new NetworkManager(this);
    m_databaseManager = new DatabaseManager(this);
    DeviceManager::instance()->loadConfig();

    setupUI();
    setupStatusBar();

    connect(networkManager, &NetworkManager::statusDataUpdated, cabinetPage, &ShoeCabinetPage::dataUpdated);
    connect(networkManager, &NetworkManager::statusDataUpdated, tieShoePage, &TieShoePage::dataUpdated);
    connect(networkManager, &NetworkManager::statusDataUpdated, mapPage, &RailMapViewerWidget::dataUpdated);
    connect(networkManager, &NetworkManager::statusDataUpdated, dataInventoryPage_, &DataInventoryPage::dataUpdated);


    connect(mapPage, &RailMapViewerWidget::getGeoFence, m_databaseManager, &DatabaseManager::handleGetGeoFence);
    connect(mapPage, &RailMapViewerWidget::saveGeoFence, m_databaseManager, &DatabaseManager::handleSaveGeoFence);
    connect(mapPage, &RailMapViewerWidget::clearGeoFence, m_databaseManager, &DatabaseManager::handleClearGeoFence);

    connect(m_databaseManager, &DatabaseManager::geoFenceData, mapPage, &RailMapViewerWidget::handleIncomingFencePoint);

    connect(networkManager, &NetworkManager::stateChanged, this, &MainWindow::updateNetworkStatus);
    connect(networkManager, &NetworkManager::serverDiscovered, this, [this](const QString &ip, int port) {
        // 可选：提示用户已发现服务器
    });


    connect(whiteListPage_, &WhiteListPage::entryAdded, m_databaseManager, &DatabaseManager::handleAddToWhitelist);
    connect(whiteListPage_, &WhiteListPage::entryUpdated, m_databaseManager, &DatabaseManager::handleUpdateWhitelist);
    connect(whiteListPage_, &WhiteListPage::entryRemoved, m_databaseManager, &DatabaseManager::handleRemoveFromWhitelist);
    connect(whiteListPage_, &WhiteListPage::getWhitelist, m_databaseManager, &DatabaseManager::handleGetWhitelist);
    connect(m_databaseManager, &DatabaseManager::whitelistOperationResult, whiteListPage_, &WhiteListPage::handleOperateResult);
    connect(m_databaseManager, &DatabaseManager::whitelistData, whiteListPage_, &WhiteListPage::refreshTable);
    connect(m_databaseManager, &DatabaseManager::whitelistData, this, [this](const QMap<quint32, WhitelistEntry>& entries){
        std::vector<quint32> whitelist;
        for (auto it = entries.cbegin(); it != entries.cend(); ++it) {
            quint32 uid = it.key();
            // const WhitelistEntry &e = it.value();

            whitelist.push_back(uid);
        }
        WhitelistSync response(0x00, whitelist);
        networkManager->sendRequest(response);
    });
    connect(dataInventoryPage_, &DataInventoryPage::dataInventoryConfig, m_databaseManager, &DatabaseManager::handleDataInventoryConfig);
    connect(m_databaseManager, &DatabaseManager::dataInventoryConfigLoaded, dataInventoryPage_, &DataInventoryPage::handleDataInventoryConfig);


    m_databaseManager->initDatabase();
    m_databaseManager->loadDataInventoryConfig();

    // 启动自动发现
    networkManager->startDiscovery();

    mapPage->loadGeoFence();

    cabinetPage->dataUpdated();
    tieShoePage->dataUpdated();
    dataInventoryPage_->dataUpdated();
    whiteListPage_->getAllWhitelist();

    // 登录窗口
    loginDialog = new LoginDialog(this);
    QObject::connect(loginDialog, &LoginDialog::loginSuccess, [=]() {
        loginDialog->hide();           // 隐藏登录框
    });
    loginDialog->show();
}

void MainWindow::applyFlatStyle() {
    // 从 Qt 资源系统加载 QSS
    QFile file(":/styles/tech_dark.qss");
    if (file.open(QFile::ReadOnly)) {
        qApp->setStyleSheet(file.readAll());
        file.close();
    } else {
        qDebug() << " open file failed !!!!!!";
    }

    // 为退出按钮设置特殊 objectName（用于 QSS 选择器）
    if (exitBtn) {
        exitBtn->setObjectName("exitButton");
    }

    // 确保主窗口能被 #MainWindow 选中
    this->setObjectName("MainWindow");
}

void MainWindow::setupUI() {
    // 创建页面
    mapPage = new RailMapViewerWidget(this);
    tieShoePage = new TieShoePage(this);
    cabinetPage = new ShoeCabinetPage(this);
    // netPage = new NetworkConfigPage(this);
    dataInventoryPage_ = new DataInventoryPage(this);
    whiteListPage_ = new WhiteListPage(this);
    eventPage_ = new EventPage(this);

    contentStack = new QStackedWidget;
    contentStack->addWidget(mapPage);
    contentStack->addWidget(tieShoePage);
    contentStack->addWidget(cabinetPage);
    // contentStack->addWidget(netPage);
    contentStack->addWidget(dataInventoryPage_);
    contentStack->addWidget(eventPage_);
    contentStack->addWidget(whiteListPage_);
    // ========== 添加顶部标题栏 ==========
    QWidget *headerWidget = new QWidget;

    // 主垂直布局：两行
    QVBoxLayout *mainHeaderLayout = new QVBoxLayout(headerWidget);
    mainHeaderLayout->setContentsMargins(10, 5, 10, 5);
    mainHeaderLayout->setSpacing(4); // 行间距

    // --- 第一行：Logo + 标题 ---
    QWidget *titleRow = new QWidget;
    QHBoxLayout *titleLayout = new QHBoxLayout(titleRow);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(8);

    // Logo 图标
    QLabel *logoLabel = new QLabel;
    logoLabel->setPixmap(coloredSvg(":/icon/logo.svg", QColor("#38BDF8"), 32,32));
    logoLabel->setAttribute(Qt::WA_TranslucentBackground); // ← 透明背景

    // 软件标题
    QLabel *titleLabel = new QLabel("管理中心");
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: white;");
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    titleLayout->addWidget(logoLabel);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch(); // 防止拉伸，保持靠左

    // --- 第二行：系统时间 ---
    // 系统时间标签
    timeLabel = new QLabel();
    timeLabel->setStyleSheet("color: #ccc; font-size: 12px;");
    timeLabel->setAlignment(Qt::AlignCenter);
    updateTime(); // 初始化时间

    // 添加到主垂直布局
    mainHeaderLayout->addWidget(titleRow);
    mainHeaderLayout->addWidget(timeLabel);
    mainHeaderLayout->addStretch(); // 可选：防止时间被推下去


    // 创建导航按钮（用于互斥选中）
    mapBtn = new QPushButton("地图监控");
    tieShoeBtn = new QPushButton("铁鞋管理");
    cabinetBtn = new QPushButton("鞋柜管理");
    // netBtn = new QPushButton("网络设置");
    dataInventoryBtn_ = new QPushButton("数据盘点");
    eventPageBtn_ =  new QPushButton("事件记录");
    whiteListBtn_ = new QPushButton("白名单配置");

    // 设置按钮为可选中（checkable），实现互斥
    mapBtn->setCheckable(true);
    tieShoeBtn->setCheckable(true);
    cabinetBtn->setCheckable(true);
    // netBtn->setCheckable(true);
    dataInventoryBtn_->setCheckable(true);
    eventPageBtn_->setCheckable(true);
    whiteListBtn_->setCheckable(true);

    // 默认选中第一个
    mapBtn->setChecked(true);

    // 连接切换逻辑
    connect(mapBtn, &QPushButton::clicked, this, [this]() { switchPage(0); });
    connect(tieShoeBtn, &QPushButton::clicked, this, [this]() { switchPage(1); });
    connect(cabinetBtn, &QPushButton::clicked, this, [this]() { switchPage(2); });
    connect(dataInventoryBtn_, &QPushButton::clicked, this, [this]() { switchPage(3); });
    connect(eventPageBtn_, &QPushButton::clicked, this, [this]() { switchPage(4); });
    connect(whiteListBtn_, &QPushButton::clicked, this, [this]() { switchPage(5); });

    // connect(netPage, &NetworkConfigPage::connectRequested,
    //         this, &MainWindow::onConnectRequested);

    // connect(netPage, &NetworkConfigPage::disconnectRequested,
    //         this, &MainWindow::onDisconnectRequested);

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
    navLayout->addWidget(dataInventoryBtn_);
    navLayout->addWidget(eventPageBtn_);
    navLayout->addWidget(whiteListBtn_);
    navLayout->addStretch(); // 推开下方的退出按钮

    // 左侧整体容器：导航 + 退出
    QWidget *leftSidebar = new QWidget;
    leftSidebar->setFixedWidth(150); // 固定宽度
    QVBoxLayout *sidebarLayout = new QVBoxLayout(leftSidebar);
    sidebarLayout->setContentsMargins(10, 10, 10, 10);
    sidebarLayout->setSpacing(0);
    sidebarLayout->addWidget(headerWidget); // 头部

    sidebarLayout->addSpacing(30);

    // 2. 添加横线分隔符
    QLabel *separator = new QLabel;
    separator->setFixedHeight(1); // 高度 1px
    separator->setStyleSheet("background-color: #444; border: none; margin: 6px 0;"); // 灰色线 + 上下间距
    sidebarLayout->addWidget(separator);

    sidebarLayout->addSpacing(30);

    // sidebarLayout->setSpacing(8);
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

    // 启动定时器刷新时间
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateTime);
    timer->start(1000); // 每秒更新一次
}

void MainWindow::switchPage(int index) {
    contentStack->setCurrentIndex(index);

    // 同步按钮选中状态
    mapBtn->setChecked(index == 0);
    tieShoeBtn->setChecked(index == 1);
    cabinetBtn->setChecked(index == 2);
    dataInventoryBtn_->setChecked(index == 3);
    eventPageBtn_->setChecked(index == 4);
    whiteListBtn_->setChecked(index == 5);
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
            qint64 secs = QDateTime::currentSecsSinceEpoch();
            qDebug() << "Unix timestamp (seconds):" << secs;
            QDateTime localTime = QDateTime::fromSecsSinceEpoch(secs);
            qDebug() << "Local time:" << localTime.toString("yyyy-MM-dd HH:mm:ss");
            TimeSyncResponse response(0x00, secs);
            networkManager->sendRequest(response);

            // std::vector<quint32> whitelist;
            // whitelist.push_back(0x12345678);
            // whitelist.push_back(0x34567890);
            // whitelist.push_back(0x56789ABC);
            // whitelist.push_back(0x789ABCDE);
            // whitelist.push_back(0x87654321);
            // WhitelistSync response(0x00, whitelist);
            // networkManager->sendRequest(response);
        }
    } else if (state == NetworkManager::Discovering || state == NetworkManager::Connecting) {
        statusLabel->setStyleSheet("color: orange; padding: 4px 8px;");
    } else {
        statusLabel->setStyleSheet("color: red; padding: 4px 8px;");
    }
}

void MainWindow::updateTime() {
    QDateTime now = QDateTime::currentDateTime();
    QString timeStr = now.toString("yyyy-MM-dd\nHH:mm:ss");
    timeLabel->setText(timeStr);
}
