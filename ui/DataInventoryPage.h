#ifndef DATAINVENTORYPAGE_H
#define DATAINVENTORYPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QMap>
#include <QObject>

// --- 引入 Qt Charts 相关头文件 ---
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>

#include <QTextDocument>
#include <QTime>

// 卡片类（复用）
class StatCard : public QWidget {
    Q_OBJECT
public:
    StatCard(const QString &title, const QString &icon, const QColor &borderColor,
             QWidget *parent = nullptr)
        : QWidget(parent), m_title(title), m_icon(icon), m_borderColor(borderColor) {
        setFixedHeight(100);
        setStyleSheet("background: #0d152a; border-radius: 12px;");
    }

    void setValue(const QString &value) {
        m_value = value;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // 边框（渐变描边）
        QPen pen;
        pen.setColor(m_borderColor);
        pen.setWidth(2);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 12, 12);

        // 图标（左上）
        QFont iconFont = font();
        iconFont.setPixelSize(24);
        painter.setFont(iconFont);
        painter.setPen(Qt::white);
        painter.drawText(QRectF(16, 16, 32, 32), Qt::AlignCenter, m_icon);

        QRectF titleRect(56, 16, width()-72, 20);
        QFont titleFont = font();
        titleFont.setPixelSize(16);
        titleFont.setBold(false);

        // 主值和子信息区域
        QRectF valueRect(56, 36, width()-72, 48);

        // 创建 QTextDocument 来处理 HTML
        QTextDocument doc;
        doc.setHtml(m_value); // m_value 现在可以包含 HTML

        // 设置文档的默认字体和宽度以适应布局
        doc.setDefaultFont(titleFont); // 默认字体用于未指定样式的文本
        doc.setTextWidth(valueRect.width());

        // 先绘制纯标题（如果需要分离标题和数值，可以保留这部分）
        painter.setFont(titleFont);
        painter.setPen(QColor(150, 170, 200));
        painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignTop, m_title);

        // 将整个 m_value (包含主数值和子信息) 绘制在 valueRect 区域
        // 并手动设置其顶部偏移以模拟原来的位置
        painter.translate(valueRect.topLeft());
        doc.drawContents(&painter);
        painter.translate(-valueRect.topLeft());
    }

private:
    QString m_title, m_icon, m_value;
    QColor m_borderColor;
};

class DataInventoryPage : public QWidget
{
    Q_OBJECT

public:
    explicit DataInventoryPage(QWidget *parent = nullptr);

    // 数据更新接口（外部调用）
    void updateData(int totalCabinets, int onlineCabinets, int offlineCabinets,
                    int totalShoes, int onlineShoes, int offlineShoes, int inCabinetShoes,
                    int lowBatteryShoes);

public slots:
    void dataUpdated();
    void handleDataInventoryConfig(const QString &path, const QTime &time);
    void exportData();

signals:
    void dataChanged();

private:
    void performExport();

private:
    // 子控件
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_topRow;
    StatCard *m_cabinetCard;
    StatCard *m_shoeCard;
    StatCard *m_lowBatCard;

    // --- 使用 Qt Charts 替换 DonutChart ---
    QChartView *m_chartView;
    QPieSeries *m_pieSeries;

    QLabel *m_legend;

    // 数据缓存
    int m_totalCabinets = 0, m_onlineCabinets = 0, m_offlineCabinets = 0;
    int m_totalShoes = 0, m_onlineShoes = 0, m_offlineShoes = 0, m_inCabinetShoes = 0, m_lowBatteryShoes = 0;

    QString export_path_;
    QTime export_time_;
    QDate last_export_date_;   // 记录上次成功导出的日期
};

#endif // DATAINVENTORYPAGE_H
