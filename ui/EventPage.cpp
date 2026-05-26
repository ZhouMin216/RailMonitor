#include "EventPage.h"
#include "database/DatabaseManager.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QApplication>
#include <QDebug>

EventPage::EventPage(QWidget *parent)
    : QWidget(parent)
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(8);

    // 标题
    QLabel *titleLabel = new QLabel("事件列表");
    titleLabel->setStyleSheet(R"(
        font-size: 24px;
        font-weight: bold;
        color: white;
        border: none;
        background: transparent;
        margin-bottom: 4px;
    )");
    mainLayout->addWidget(titleLabel);

    // 表格
    setupTable();
    mainLayout->addWidget(table);

    // 翻页控件
    setupPagination();
    mainLayout->addLayout(paginationLayout);

    // onResize();

    // 监听表格尺寸变化以调整每页行数
    // connect(table->verticalScrollBar(), &QScrollBar::rangeChanged,
    //         this, &EventPage::onResize);
    // connect(table, &QTableWidget::viewportEntered, this, &EventPage::onResize);
}

void EventPage::setupTable()
{
    table = new QTableWidget(0, static_cast<int>(Column::Count));
    QStringList headers;
    for (int i = 0; i < static_cast<int>(Column::Count); ++i) {
        headers << columnHeader(static_cast<Column>(i));
    }
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setDefaultSectionSize(36);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);

    table->setStyleSheet(R"(
        QTableWidget {
            background-color: #1e1e2e;
            gridline-color: #2d2d44;
            color: #e0e0ff;
            border: none;
            font-size: 13px;
        }
        QHeaderView::section {
            background-color: #252535;
            color: #a0a0c0;
            padding: 8px;
            border: none;
            font-weight: bold;
            font-size: 14px;
        }
        QTableWidget::item {
            padding: 8px;
            border-bottom: 1px solid #2d2d44;
        }
        QTableWidget::item:selected {
            background-color: #2a2a40;
        }
    )");
}

void EventPage::setupPagination()
{
    paginationLayout = new QHBoxLayout;

    prevButton = new QPushButton("上一页");
    nextButton = new QPushButton("下一页");
    pageLabel = new QLabel("第 1 页，共 1 页");

    prevButton->setEnabled(false);
    nextButton->setEnabled(false);

    connect(prevButton, &QPushButton::clicked, this, &EventPage::onPrevPage);
    connect(nextButton, &QPushButton::clicked, this, &EventPage::onNextPage);

    paginationLayout->addWidget(prevButton);
    paginationLayout->addStretch();
    paginationLayout->addWidget(pageLabel);
    paginationLayout->addStretch();
    paginationLayout->addWidget(nextButton);
}

int EventPage::calculateRowsPerPage() const
{
    int rowHeight = table->rowHeight(0);
    if (rowHeight <= 0) rowHeight = 36;
    int viewportHeight = table->viewport()->height();
    int rows = qMax(1, viewportHeight / rowHeight);
    return rows;
}

void EventPage::onResize()
{
    int newRows = calculateRowsPerPage();
    if (newRows != currentRowsPerPage) {
        currentRowsPerPage = newRows;
        if (totalEventCount > 0) {
            // 重新计算总页数并加载当前页
            totalPages = (totalEventCount + currentRowsPerPage - 1) / currentRowsPerPage;
            currentPage = qBound(1, currentPage, totalPages);
            pageLabel->setText(QString("第 %1 页，共 %2 页").arg(currentPage).arg(totalPages));
            prevButton->setEnabled(currentPage > 1);
            nextButton->setEnabled(currentPage < totalPages);
            loadCurrentPage();
        }
    }
}

void EventPage::loadCurrentPage()
{
    int offset = (currentPage - 1) * currentRowsPerPage;
    emit getEventLogs(currentRowsPerPage, offset);
}

void EventPage::onTotalEventCountLoaded(int totalCount)
{
    totalEventCount = totalCount;
    currentRowsPerPage = calculateRowsPerPage(); // 确保使用最新行数
    totalPages = (totalCount == 0) ? 1 : (totalCount + currentRowsPerPage - 1) / currentRowsPerPage;
    currentPage = qBound(1, currentPage, totalPages);

    pageLabel->setText(QString("第 %1 页，共 %2 页").arg(currentPage).arg(totalPages));
    prevButton->setEnabled(currentPage > 1);
    nextButton->setEnabled(currentPage < totalPages);

    loadCurrentPage();
}

void EventPage::onEventLogsLoaded(const QList<EventLogEntry>& logs)
{
    table->setRowCount(logs.size());
    for (int row = 0; row < logs.size(); ++row) {
        const auto& entry = logs[row];

        // 时间列
        auto timeItem = new QTableWidgetItem(formatTimestamp(entry.timestamp));
        timeItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, columnIndex(Column::EventTime), timeItem);

        // 等级列
        auto levelItem = new QTableWidgetItem(entry.level);
        levelItem->setTextAlignment(Qt::AlignCenter);
        levelItem->setForeground(levelColor(entry.level));
        levelItem->setFont(QFont("Consolas", 10, QFont::Bold));
        table->setItem(row, columnIndex(Column::Level), levelItem);

        // 详情列
        auto detailItem = new QTableWidgetItem(entry.message);
        detailItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, columnIndex(Column::Details), detailItem);
    }

    // 调整列宽
    for (int col = 0; col < table->columnCount(); ++col) {
        table->resizeColumnToContents(col);
        if (table->columnWidth(col) > 200) {
            table->setColumnWidth(col, 200);
        }
    }
}

void EventPage::onPrevPage()
{
    if (currentPage > 1) {
        currentPage--;
    }
}

void EventPage::onNextPage()
{
    if (currentPage < totalPages) {
        currentPage++;
    }
}

QString EventPage::formatTimestamp(const QDateTime& dt) const
{
    return dt.toString("yyyy-MM-dd HH:mm:ss");
}

QColor EventPage::levelColor(const QString& level) const
{
    if (level == "critical") return QColor("#ff6b6b"); // 红
    if (level == "warning")  return QColor("#feca57"); // 橙黄
    return QColor("#48dbfb"); // info: 青蓝
}
