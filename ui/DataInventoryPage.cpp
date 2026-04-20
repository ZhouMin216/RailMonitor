#include "DataInventoryPage.h"

#include <QFontDatabase>
#include <QGraphicsDropShadowEffect>
#include <QDir>
#include <QTimeEdit>
#include <QDialog>
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include "DeviceManager.h"

DataInventoryPage::DataInventoryPage(QWidget *parent)
    : QWidget(parent), last_export_date_(QDate())
{
    setStyleSheet("background: #0a0f25;");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);
    m_mainLayout->setSpacing(20);

    // ===== 页面标题 =====
    auto pageTitle = new QLabel("智能资产实时盘点");
    pageTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: white; margin-bottom: 10px; background: transparent;");
    m_mainLayout->addWidget(pageTitle);

    // 顶部三卡片行
    m_topRow = new QHBoxLayout;
    m_topRow->setSpacing(20);

    m_cabinetCard = new StatCard("鞋柜总数", "🗄️", QColor("#4a6aff"));
    m_shoeCard = new StatCard("铁鞋总数", "👟", QColor("#00ffcc"));
    m_lowBatCard = new StatCard("低电量铁鞋", "🔋", QColor("#ff6b35"));

    m_topRow->addWidget(m_cabinetCard);
    m_topRow->addWidget(m_shoeCard);
    m_topRow->addWidget(m_lowBatCard);

    m_mainLayout->addLayout(m_topRow);

    // 中间：环形图 + 图例
    QHBoxLayout *centerLayout = new QHBoxLayout;
    centerLayout->setSpacing(30);

    // --- 创建 Qt Charts 环形图 ---
    m_shoePieSeries = new QPieSeries();
    m_shoePieSeries->setHoleSize(0.00); // 设置为环形图 (0.0 = 饼图, 1.0 = 空心)

    m_cabinetPieSeries = new QPieSeries();
    m_cabinetPieSeries->setHoleSize(0.00);

    // 添加切片并设置初始颜色
    m_shoePieSeries->append("在线", 0)->setColor(QColor("#00ffcc"));
    m_shoePieSeries->append("在柜", 0)->setColor(QColor("#4a6aff"));
    m_shoePieSeries->append("离线", 0)->setColor(QColor("#ffd166"));
    m_shoePieSeries->append("低电量", 0)->setColor(QColor("#ff0000"));

    m_cabinetPieSeries->append("在线", 0)->setColor(QColor("#00ffcc"));
    m_cabinetPieSeries->append("离线", 0)->setColor(QColor("#ffd166"));

    // ---隐藏饼图上的所有标签 ---
    m_shoePieSeries->setLabelsVisible(false);
    m_cabinetPieSeries->setLabelsVisible(false);

    // 配置切片标签和交互
    for (auto slice : m_shoePieSeries->slices()) {
        slice->setLabelVisible(false);
        slice->setLabelPosition(QPieSlice::LabelOutside);
        slice->setBorderColor(Qt::transparent);
        slice->setLabelBrush(QBrush(Qt::white)); // <-- 关键修改
        slice->setBorderWidth(0);
        // 可选：鼠标悬停时高亮
        // slice->setExploded(true);
    }
    for (auto slice : m_cabinetPieSeries->slices()) {
        slice->setLabelVisible(false);
        slice->setLabelPosition(QPieSlice::LabelOutside);
        slice->setBorderColor(Qt::transparent);
        slice->setLabelBrush(QBrush(Qt::white)); // <-- 关键修改
        slice->setBorderWidth(0);
        // 可选：鼠标悬停时高亮
        // slice->setExploded(true);
    }

    QChart *chart = new QChart();
    chart->addSeries(m_shoePieSeries);
    chart->setBackgroundVisible(false); // 使背景透明，与主窗口融合
    chart->setBackgroundBrush(QBrush(Qt::transparent)); // <-- 关键修改
    chart->setBackgroundPen(Qt::NoPen);                 // <-- 关键修改
    // chart->legend()->setVisible(false); // 我们使用自定义图例
    chart->setTitle("铁鞋数据详情");
    chart->setTitleBrush(QBrush(Qt::white));

    QFont titleFont;
    titleFont.setPixelSize(18);
    titleFont.setBold(true);
    chart->setTitleFont(titleFont);

    QChart *cabinet_chart = new QChart();
    cabinet_chart->addSeries(m_cabinetPieSeries);
    cabinet_chart->setBackgroundVisible(false); // 使背景透明，与主窗口融合
    cabinet_chart->setBackgroundBrush(QBrush(Qt::transparent)); // <-- 关键修改
    cabinet_chart->setBackgroundPen(Qt::NoPen);                 // <-- 关键修改
    // chart->legend()->setVisible(false); // 我们使用自定义图例
    cabinet_chart->setTitle("鞋柜数据详情");
    cabinet_chart->setTitleBrush(QBrush(Qt::white));
    cabinet_chart->setTitleFont(titleFont);

    m_shoeChartView = new QChartView(chart);
    m_shoeChartView->setStyleSheet(R"(
        QChartView {
            background: transparent;
            border: 2px solid #00ffcc;   /* 边框颜色，可自定义 */
            border-radius: 16px;         /* 圆角半径 */
        }
    )");
    m_shoeChartView->setRenderHint(QPainter::Antialiasing);
    m_shoeChartView->setMinimumSize(160, 160);

    m_cabinetChartView = new QChartView(cabinet_chart);
    m_cabinetChartView->setStyleSheet(R"(
        QChartView {
            background: transparent;
            border: 2px solid #4a6aff;   /* 边框颜色，可自定义 */
            border-radius: 16px;         /* 圆角半径 */
        }
    )");
    m_cabinetChartView->setRenderHint(QPainter::Antialiasing);
    m_cabinetChartView->setMinimumSize(160, 160);

    centerLayout->addWidget(m_cabinetChartView);
    centerLayout->addWidget(m_shoeChartView);

    m_mainLayout->addLayout(centerLayout);

    // ===== 底部导出信息栏 =====
    auto bottomLayout = new QHBoxLayout;
    bottomLayout->setSpacing(15);

    m_exportTimeLabel = new QLabel("数据导出时间: 未设置");
    m_exportPathLabel = new QLabel("数据导出路径: 未设置");

    m_exportTimeLabel->setStyleSheet("background: transparent; color: #aaa; font-size: 16px;");
    m_exportPathLabel->setStyleSheet("background: transparent; color: #aaa; font-size: 16px;");

    m_configButton = new QPushButton("⚙配置");
    m_configButton->setStyleSheet(R"(
    QPushButton {
        background: #4a6aff;
        color: white;
        border: none;
        border-radius: 6px;
        padding: 6px 12px;
        font-size: 16px;
    }
    QPushButton:hover {
        background: #5a7aff;
    }
)");

    connect(m_configButton, &QPushButton::clicked, this, &DataInventoryPage::showExportConfigDialog);

    bottomLayout->addStretch();
    bottomLayout->addWidget(m_exportTimeLabel);
    bottomLayout->addSpacing(20);
    bottomLayout->addWidget(m_exportPathLabel);
    // bottomLayout->addStretch();
    bottomLayout->addSpacing(20);
    bottomLayout->addWidget(m_configButton);

    m_mainLayout->addLayout(bottomLayout);

    // 启动定时器刷新时间
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &DataInventoryPage::exportData);
    timer->start(1000); // 每秒更新一次
}

void DataInventoryPage::updateData(int totalCabinets, int onlineCabinets, int offlineCabinets,
                                   int totalShoes, int onlineShoes, int offlineShoes, int inCabinetShoes,
                                   int lowBatteryShoes)
{
    m_totalCabinets = totalCabinets;
    m_onlineCabinets = onlineCabinets;
    m_offlineCabinets = offlineCabinets;

    m_totalShoes = totalShoes;
    m_onlineShoes = onlineShoes;
    m_offlineShoes = offlineShoes;
    m_inCabinetShoes = inCabinetShoes;
    m_lowBatteryShoes = lowBatteryShoes;

    // 更新卡片
    m_cabinetCard->updateTitle(totalCabinets);
    m_cabinetCard->setValue(QString("<font color='#99ccff'>在线: %1 | 离线: %2</font>")
                            .arg(onlineCabinets).arg(offlineCabinets));
    m_shoeCard->updateTitle(totalShoes);
    m_shoeCard->setValue(QString("<font color='#99ffcc'>在线: %2 | 离线: %3 | 在柜: %4</font>")
                            .arg(onlineShoes).arg(offlineShoes).arg(inCabinetShoes));
    m_lowBatCard->setValue(QString("<font color='white'>%1</font>").arg(lowBatteryShoes));

    // --- 更新 Qt Charts 环形图 ---
    // 清除旧数据并设置新值
    QList<QPieSlice*> slices = m_shoePieSeries->slices();
    if (slices.size() >= 4) {
        slices[0]->setValue(onlineShoes);
        slices[1]->setValue(inCabinetShoes);
        slices[2]->setValue(offlineShoes);
        slices[3]->setValue(lowBatteryShoes);
    }

    slices = m_cabinetPieSeries->slices();
    if (slices.size() >= 2) {
        slices[0]->setValue(onlineCabinets);
        slices[1]->setValue(offlineCabinets);
    }
}

void DataInventoryPage::dataUpdated(){
    QMap<quint16, std::shared_ptr<ShoeCabinet>> cabinetMap = DeviceManager::instance()->getCabinetMap();
    int totalCabinets = cabinetMap.size();
    int onlineCabinets = 0, offlineCabinets = 0;
    for (const auto& cabinet : cabinetMap) {
        if (!cabinet) continue;
        auto status = cabinet->GetCabinetData().byOnline;
        if (status == CabinetStatus::Online) onlineCabinets++;
        else if (status == CabinetStatus::Offline) offlineCabinets++;
    }

    QMap<quint16, std::shared_ptr<IconShoe>> shoeMap = DeviceManager::instance()->getShoeMap();
    int totalShoes = shoeMap.size();
    int onlineShoes = 0, offlineShoes = 0, inCabinetShoes = 0, lowBatteryShoes = 0;
    for (const auto& shoe : shoeMap) {
        if (!shoe) continue;
        auto status = shoe->GetShoeData().byOnline;
        if (status == ShoeStatus::Online) onlineShoes++;
        else if ( status == ShoeStatus::InCabinet) inCabinetShoes++;
        else if (status == ShoeStatus::Offline || status == ShoeStatus::Unregister) offlineShoes++;

        if (shoe->GetShoeData().byBatVal < 20 && status != ShoeStatus::Unregister) lowBatteryShoes++;
    }

    updateData(totalCabinets, onlineCabinets, offlineCabinets, totalShoes,
               onlineShoes, offlineShoes, inCabinetShoes, lowBatteryShoes);
}

void DataInventoryPage::handleDataInventoryConfig(const QString &path, const QTime &time){
    export_path_ = path;
    export_time_ = time;
    qDebug() << " DataInventoryPage: " << export_path_ << " " << export_time_.toString("HH:mm");
    m_exportTimeLabel->setText("数据导出时间: " + export_time_.toString("HH:mm"));
    m_exportPathLabel->setText("数据导出路径: " + export_path_);
}

void DataInventoryPage::exportData(){
    if (export_path_.isEmpty() || export_path_ == "未选择目录" || !export_time_.isValid()) {
        qDebug() << " 导出条件不成立 ";
        return;
    }

    QTime now = QTime::currentTime();
    QDate today = QDate::currentDate();

    // 判断是否到达目标分钟（小时和分钟匹配）
    if (now.hour() == export_time_.hour() &&
        now.minute() == export_time_.minute()) {

        // 确保今天还没导出过
        if (last_export_date_ != today) {
            // 获取文件所在的目录路径
            QFileInfo fileInfo(export_path_);
            QString dirPath = fileInfo.absolutePath();  // 例如: "C:/Users/86153/Desktop"
            QDir dir(dirPath);
            if (!dir.exists()) {
                if (!dir.mkpath(".")) {
                    qWarning() << "无法创建目录:" << dirPath;
                    return;
                }
            }

            performExport(); // export_path_ 作为文件名写入
            last_export_date_ = today; // 标记已导出
        }
    }
}

void DataInventoryPage::performExport(){
    if (export_path_.isEmpty()) {
        qDebug() << " export_path_ is empty ";
        return;
    }

    QFile file(export_path_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << export_path_ << "  open failed ";
        return;
    }

    // 获取当前时间
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    // 构造 CSV 内容（QString）
    QString csvContent;
    csvContent += "导出时间,总柜机数,在线柜机数,离线柜机数,总鞋数,在线鞋数,离线鞋数,柜内鞋数,低电量鞋数\n";
    csvContent += QString("%1,%2,%3,%4,%5,%6,%7,%8,%9\n")
                      .arg(currentTime)
                      .arg(m_totalCabinets)
                      .arg(m_onlineCabinets)
                      .arg(m_offlineCabinets)
                      .arg(m_totalShoes)
                      .arg(m_onlineShoes)
                      .arg(m_offlineShoes)
                      .arg(m_inCabinetShoes)
                      .arg(m_lowBatteryShoes);

    // 以 UTF-8 编码写入文件
    file.write("\xEF\xBB\xBF"); // UTF-8 BOM
    file.write(csvContent.toUtf8());
    file.close();
    qDebug() << " export data ";
}


void DataInventoryPage::showExportConfigDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("导出配置");
    dialog.setModal(true);
    dialog.setStyleSheet("background: #1a1a2e; color: white;");

    auto layout = new QVBoxLayout(&dialog);
    layout->setSpacing(15);
    layout->setContentsMargins(20, 20, 20, 20);

    // === 时间设置 ===
    auto timeLayout = new QHBoxLayout;
    auto timeLabel = new QLabel("导出时间:");
    timeLabel->setMinimumWidth(80);
    QTimeEdit* timeEdit = new QTimeEdit;
    timeEdit->setDisplayFormat("HH:mm");
    // timeEdit->setStyleSheet("background: #2d2d44; color: white; border: 1px solid #444; border-radius: 4px;");
    timeEdit->setStyleSheet(
        "QTimeEdit {"
        "   background: #1e293b; border: 1px solid #334155; border-radius: 6px;"
        "   padding: 6px 10px; color: white;"
        "}"
        "QTimeEdit::up-button, QTimeEdit::down-button { width: 16px; }"
        );
    timeEdit->setTime(export_time_.isValid() ? export_time_ : QTime(9, 0)); // 默认 09:00
    timeLayout->addWidget(timeLabel);
    timeLayout->addWidget(timeEdit);
    timeLayout->addStretch();
    layout->addLayout(timeLayout);

    // === 路径设置 ===
    auto pathLayout = new QHBoxLayout;
    auto pathLabel = new QLabel("导出路径:");
    pathLabel->setMinimumWidth(80);
    QLineEdit* pathEdit = new QLineEdit;
    pathEdit->setStyleSheet("background: #2d2d44; color: white; border: 1px solid #444; border-radius: 4px;");
    pathEdit->setText(export_path_.isEmpty() ? "未选择目录" : export_path_);
    QPushButton* browseBtn = new QPushButton("浏览...");
    browseBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #38bdf8; color: #0f172a; border: none; border-radius: 4px;"
        "   padding: 4px 12px; font-size: 12px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #0ea5e9; }"
        );
    connect(browseBtn, &QPushButton::clicked, this, [&]() {
        // 默认文件名
        QString defaultFileName = "data_inventory.csv";

        // 如果已有路径且不是默认提示，则尝试提取目录作为初始目录
        QString initialDir;
        QString currentText = m_exportPathLabel->text();
        if (currentText != "未选择目录") {
            QFileInfo fi(currentText);
            if (fi.exists()) {
                initialDir = fi.absolutePath();
            } else if (QDir(currentText).exists()) {
                initialDir = currentText;
            } else {
                initialDir = QFileInfo(currentText).absolutePath(); // 即使路径不存在也取目录部分
            }
        } else {
            initialDir = QDir::homePath();
        }

        // 弹出“另存为”对话框
        QString selectedFile = QFileDialog::getSaveFileName(
            this,
            "选择导出文件路径",
            initialDir + "/" + defaultFileName,   // 初始路径+文件名
            "CSV 文件 (*.csv)"     // 文件过滤器（可选）
            );

        if (!selectedFile.isEmpty()) {
            QFileInfo fileInfo(selectedFile);
            QString baseName = fileInfo.completeBaseName(); // 去掉所有扩展名
            QString dir = fileInfo.absolutePath();
            selectedFile = dir + "/" + baseName + ".csv";

            export_path_ = selectedFile;
            pathEdit->setText(selectedFile);
            // m_exportPathLabel->setText(selectedFile);
        }
    });
    pathLayout->addWidget(pathLabel);
    pathLayout->addWidget(pathEdit);
    pathLayout->addWidget(browseBtn);
    layout->addLayout(pathLayout);


    // === 按钮区 ===
    auto buttonLayout = new QHBoxLayout;
    QPushButton* saveBtn = new QPushButton("保存");
    QPushButton* cancelBtn = new QPushButton("取消");
    saveBtn->setStyleSheet("background: #00b894; color: white; border: none; border-radius: 4px; padding: 6px 12px;");
    cancelBtn->setStyleSheet("background: #7f8c8d; color: white; border: none; border-radius: 4px; padding: 6px 12px;");
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    layout->addLayout(buttonLayout);

    // 连接按钮
    connect(saveBtn, &QPushButton::clicked, [&]() {
        QTime newTime = timeEdit->time();
        // QString newPath = pathEdit->text().trimmed();

        if (export_path_.isEmpty() || export_path_ == "未选择目录") {
            QMessageBox::warning(&dialog, "输入错误", "请设置有效的导出路径。");
            return;
        }

        // 发送信号或直接更新成员变量
        export_time_ = newTime;
        // export_path_ = newPath;

        m_exportPathLabel->setText("数据导出路径: " + export_path_);
        m_exportTimeLabel->setText("数据导出时间: " + export_time_.toString("HH:mm"));


        // 发出信号给其他模块
        emit dataInventoryConfig(export_path_, export_time_);

        dialog.accept();
    });

    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    dialog.exec();
}
