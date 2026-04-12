// ShoeCabinetPage.cpp
#include "ShoeCabinetPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QApplication>

#include "DeviceManager.h"


// ==================== ShoeCabinetPage 实现 ====================

ShoeCabinetPage::ShoeCabinetPage(QWidget *parent)
    : QWidget(parent) {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // === 顶部统计栏 ===
    auto statsLayout = new QHBoxLayout;
    onlineLabel = new QLabel("在线: 0");
    offlineLabel = new QLabel("离线: 0");

    onlineLabel->setStyleSheet("color: #00b894; font-size: 16px; font-weight: bold; border: none; background: transparent;");
    offlineLabel->setStyleSheet("color: #7f8c8d; font-size: 16px; font-weight: bold; border: none; background: transparent;");

    QLabel *titleLabel = new QLabel("终端鞋柜群监控");
    titleLabel->setStyleSheet(R"(
        font-size: 24px;
        font-weight: bold;
        color: white;
        border: none;               /* ← 显式声明无边框 */
        background: transparent;    /* ← 确保背景透明 */
        margin-bottom: 4px;
    )");
    statsLayout->addWidget(titleLabel);
    statsLayout->addStretch();
    statsLayout->addWidget(onlineLabel);
    statsLayout->addSpacing(20);
    statsLayout->addWidget(offlineLabel);

    mainLayout->addLayout(statsLayout);

    // === 滚动区域 ===
    scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background: transparent; border: none;");

    container = new QWidget;
    cardGrid = new QGridLayout(container);
    cardGrid->setSpacing(15);           // 卡片间距
    cardGrid->setContentsMargins(10, 10, 10, 10);
    container->setLayout(cardGrid);
    container->setStyleSheet("background: transparent;");

    scrollArea->setWidget(container);
    mainLayout->addWidget(scrollArea);

    // 设置整体背景
    setStyleSheet("background: #0f0f1b; color: white;");
}

void ShoeCabinetPage::updateFromDeviceManager(
    const QMap<quint16, std::shared_ptr<ShoeCabinet>>& cabinetMap)
{
    // currentCabinets = cabinetMap;

    updateStatistics(cabinetMap);
    clearCards();

    int col = 0;
    const int maxCols = 4; // 一行最多 4 个鞋柜（可根据窗口宽度调整为 2/3/4）
    int row = 0;
    for (auto it = cabinetMap.constBegin(); it != cabinetMap.constEnd(); ++it) {
        QWidget* card = createCabinetCard(it.key(), it.value());

        // 将新卡片加入网格
        if (card) {
            cardGrid->addWidget(card , row, col);
            col++;
            if (col >= maxCols) {
                col = 0;
                row++;
            }
        }
    }
}

void ShoeCabinetPage::dataUpdated(){
    QMap<quint16, std::shared_ptr<ShoeCabinet>> cabinetMap = DeviceManager::instance()->getCabinetMap();
    if (cabinetMap.isEmpty()) return;
    updateStatistics(cabinetMap);
    clearCards();

    int col = 0;
    const int maxCols = 4; // 一行最多 4 个鞋柜（可根据窗口宽度调整为 2/3/4）
    int row = 0;
    for (auto it = cabinetMap.constBegin(); it != cabinetMap.constEnd(); ++it) {
        QWidget* card = createCabinetCard(it.key(), it.value());

        // 将新卡片加入网格
        if (card) {
            cardGrid->addWidget(card , row, col);
            col++;
            if (col >= maxCols) {
                col = 0;
                row++;
            }
        }
    }
}

void ShoeCabinetPage::updateStatistics(const QMap<quint16, std::shared_ptr<ShoeCabinet>>& cabinetMap) {
    int online = 0, offline = 0;
    for (const auto& cabinet : cabinetMap) {
        if (!cabinet) continue;
        auto status = cabinet->GetCabinetData().byOnline;
        if (status == CabinetStatus::Online) online++;
        else if (status == CabinetStatus::Offline) offline++;
    }
    onlineLabel->setText(QString("在线: %1").arg(online));
    offlineLabel->setText(QString("离线: %1").arg(offline));
}

void ShoeCabinetPage::clearCards() {
    QLayoutItem* item;
    while ((item = cardGrid->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
}

QWidget* ShoeCabinetPage::createCabinetCard(quint16 cabinetId, const std::shared_ptr<ShoeCabinet>& cabinet) {
    if (!cabinet) nullptr;

    auto card = new QFrame;
    card->setObjectName("cabinetCard");
    card->setStyleSheet(
        "#cabinetCard {"
        "  background: #1a1a2e;"
        "  border: 1px solid #2d2d44;"
        "  border-radius: 12px;"
        "  padding: 16px;"
        "}"
        // 鼠标悬停时的样式
        "#cabinetCard:hover {"
        "  background: rgba(0, 116, 217, 0.8);" // 蓝色背景
        "  border: 1px solid #005bb5;" // 较深的蓝色边框
        "}"
        );

    auto layout = new QVBoxLayout(card);
    layout->setSpacing(12);

    // === 标题区 ===
    auto titleLayout = new QHBoxLayout;
    QString name = "鞋柜" ;
    auto titleLabel = new QLabel(QString("#%1 %2").arg(cabinetId, 3, 10, QChar('0')).arg(name));
    titleLabel->setFont(QFont("Microsoft YaHei", 14, QFont::Bold));
    titleLayout->addWidget(titleLabel);

    // 状态标签
    CabinetStatus cabinet_status = DeviceManager::instance()->getCabinetStatus(cabinetId);
    QString statusText = EnumtoString(cabinet_status);
    QString statusStyle;

    if (cabinet_status == CabinetStatus::Online) {
        statusStyle = "background: #00b894; color: white;";
    } else if (cabinet_status == CabinetStatus::Offline) {
        statusStyle = "background: #7f8c8d; color: white;";
    } else {
        statusStyle = "background: #e74c3c; color: white;";
    }
    auto statusLabel = new QLabel(statusText);
    statusLabel->setStyleSheet(statusStyle + "padding: 3px 10px; border-radius: 4px; font-size: 10pt;");
    statusLabel->setFixedHeight(24);
    titleLayout->addStretch();
    titleLayout->addWidget(statusLabel);
    layout->addLayout(titleLayout);

    // === 新增的中间文本行 ===
    QString paintedIds;
    QList<quint16> shoeList = DeviceManager::instance()->getCabinetBindShoes(cabinetId);
    for (const auto& shoe : shoeList) {
        paintedIds.append(DeviceManager::instance()->getPaintedID(shoe));
        paintedIds.append(" ");
    }
    auto infoLabel = new QLabel(QString("应存放铁鞋:%1").arg(paintedIds));
    infoLabel->setStyleSheet("color: #aaa; font-size: 10pt; padding: 4px 0;");
    infoLabel->setAlignment(Qt::AlignLeft);
    layout->addWidget(infoLabel); // 直接加入主 layout

    // === 仓位网格 ===
    // 铁鞋id和仓位状态的映射
    auto statusList = cabinet->GetCabinetData().storeStatus;
    static const int cols = 2;        // 固定 2 列

    auto gridLayout = new QGridLayout;
    gridLayout->setSpacing(5);
    for (int i = 0; i < statusList.size(); ++i) {
        int row = i / cols;
        int col = i % cols;

        quint16 shoe_id = statusList[i].first;
        StorageStatus status = statusList[i].second;

        QString paintedId = DeviceManager::instance()->getPaintedID(shoe_id);
        QString text = EnumtoString(status);
        QString style;
        if (status == StorageStatus::Online) {
            style = "background: #00b894; color: white;";
        } else if (status == StorageStatus::Offline) {
            style = "background: #2d2d44; color: #ccc;";
        } else if (status == StorageStatus::Unusual || status == StorageStatus::PosFault) {
            style = "background: #e74c3c; color: white;";
        } else {
            style = "background: #7f8c8d; color: white;";
        }

        auto cell = new QLabel(QString("%1\n%2").arg(paintedId).arg(text));
        cell->setFixedSize(60, 60);
        cell->setAlignment(Qt::AlignCenter);
        cell->setStyleSheet(style + "border-radius: 6px; font-size: 10pt; font-weight: bold;");
        // if (status == StorageStatus::PosFault)
        {
            quint16 real_cabinet = DeviceManager::instance()->getCabinetIdByShoeId(shoe_id);
            QString name = QString("#%1 鞋柜").arg(real_cabinet, 3, 10, QChar('0'));
            cell->setToolTip(QString("%1应存放在 %2").arg(paintedId).arg(name));
        }

        gridLayout->addWidget(cell, row, col);
    }

    layout->addLayout(gridLayout);
    return  card;
}

// ==================== 详情对话框 ====================
void ShoeCabinetPage::showShoeDetailsDialog(quint8 storeNum, quint16 cabinetId, const QByteArray& statusArray,
                                            const QMap<quint8, quint16>& shoeIds)
{
    QDialog dialog(this);
    dialog.setWindowTitle(QString("鞋柜 %1 仓位详情").arg(cabinetId));
    dialog.resize(500, 400);
    dialog.setStyleSheet("background: #1a1a2e; color: white;");

    auto layout = new QVBoxLayout(&dialog);
    auto detailTable = new QTableWidget(storeNum, 3);
    layout->addWidget(detailTable);

    detailTable->setHorizontalHeaderLabels({"仓位编号", "状态", "绑定铁鞋ID"});
    detailTable->horizontalHeader()->setStretchLastSection(true);
    detailTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    detailTable->verticalHeader()->setVisible(false);
    detailTable->setStyleSheet("QTableWidget { background: #2d2d44; gridline-color: #333; }"
                               "QHeaderView::section { background: #1a1a2e; color: white; }");

    for (int i = 0; i < storeNum; ++i) {
        detailTable->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        detailTable->item(i, 0)->setTextAlignment(Qt::AlignCenter);

        StorageStatus status = StorageStatus::Unregister;
        if (i < statusArray.size()) {
            status = static_cast<StorageStatus>(static_cast<quint8>(statusArray[i]));
        }
        auto statusItem = new QTableWidgetItem(EnumtoString(status));
        detailTable->setItem(i, 1, statusItem);
        detailTable->item(i, 1)->setTextAlignment(Qt::AlignCenter);

        if (status == StorageStatus::Online) {
            detailTable->item(i, 1)->setForeground(Qt::green);
        } else if (status == StorageStatus::Offline) {
            detailTable->item(i, 1)->setForeground(Qt::gray);
        } else if (status == StorageStatus::Unregister) {
            detailTable->item(i, 1)->setForeground(Qt::red);
        }

        if (shoeIds.contains(i + 1)) {
            detailTable->setItem(i, 2, new QTableWidgetItem(QString::number(shoeIds[i + 1])));
        } else {
            detailTable->setItem(i, 2, new QTableWidgetItem("无"));
        }
        detailTable->item(i, 2)->setTextAlignment(Qt::AlignCenter);
    }

    dialog.exec();
}
