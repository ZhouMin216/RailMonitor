#include "DataInventoryPage.h"

#include <QFontDatabase>
#include <QGraphicsDropShadowEffect>
#include <QDir>
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
    m_pieSeries = new QPieSeries();
    m_pieSeries->setHoleSize(0.00); // 设置为环形图 (0.0 = 饼图, 1.0 = 空心)

    // 添加切片并设置初始颜色
    m_pieSeries->append("在线", 0)->setColor(QColor("#00ffcc"));
    m_pieSeries->append("在柜", 0)->setColor(QColor("#4a6aff"));
    m_pieSeries->append("离线", 0)->setColor(QColor("#ffd166"));
    m_pieSeries->append("低电量", 0)->setColor(QColor("#ff0000"));

    // ---隐藏饼图上的所有标签 ---
    m_pieSeries->setLabelsVisible(false);

    // 配置切片标签和交互
    for (auto slice : m_pieSeries->slices()) {
        slice->setLabelVisible(false);
        slice->setLabelPosition(QPieSlice::LabelOutside);
        slice->setBorderColor(Qt::transparent);
        slice->setLabelBrush(QBrush(Qt::white)); // <-- 关键修改
        slice->setBorderWidth(0);
        // 可选：鼠标悬停时高亮
        // slice->setExploded(true);
    }

    QChart *chart = new QChart();
    chart->addSeries(m_pieSeries);
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

    m_chartView = new QChartView(chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumSize(160, 160);
    centerLayout->addWidget(m_chartView);

    m_mainLayout->addLayout(centerLayout);
    // updateData(0, 0, 0, 0, 0, 0, 0, 0);

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
    m_cabinetCard->setValue(QString("<font color='white'>%1</font>\n<font color='#99ccff'>在线: %2 | 离线: %3</font>")
                                .arg(totalCabinets).arg(onlineCabinets).arg(offlineCabinets));
    m_shoeCard->setValue(QString("<font color='white'>%1</font>\n<font color='#99ffcc'>在线: %2 | 离线: %3 | 在柜: %4</font>")
                             .arg(totalShoes).arg(onlineShoes).arg(offlineShoes).arg(inCabinetShoes));
    m_lowBatCard->setValue(QString("<font color='white'>%1</font>").arg(lowBatteryShoes));

    // --- 更新 Qt Charts 环形图 ---
    // 清除旧数据并设置新值
    QList<QPieSlice*> slices = m_pieSeries->slices();
    if (slices.size() >= 4) {
        slices[0]->setValue(onlineShoes);
        slices[1]->setValue(inCabinetShoes);
        slices[2]->setValue(offlineShoes);
        slices[3]->setValue(lowBatteryShoes);
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
