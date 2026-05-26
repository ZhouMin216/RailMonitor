#ifndef EVENTPAGE_H
#define EVENTPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QDateTime>

#include "protocol/ProtocolPacket.h"

class DatabaseManager;

class EventPage : public QWidget
{
    Q_OBJECT
public:
    explicit EventPage(QWidget *parent = nullptr);

    enum class Column {
        EventTime = 0,
        Level,
        Details,
        Count
    };

    static QString columnHeader(Column col) {
        switch (col) {
        case Column::EventTime: return "时间";
        case Column::Level:     return "等级";
        case Column::Details:   return "详情";
        default:                return "";
        }
    }

    static int columnIndex(Column col) {
        return static_cast<int>(col);
    }

    void getTotalEventCnt(){
        emit getTotalEventCount();
    }

signals:
    void getEventLogs(int limit, int offset);
    void getTotalEventCount();
public slots:
    void onEventLogsLoaded(const QList<EventLogEntry>& logs);
    void onTotalEventCountLoaded(int totalCount);

private slots:
    void onNextPage();
    void onPrevPage();
    void onResize();

private:
    void setupTable();
    void setupPagination();
    void loadCurrentPage();
    QString formatTimestamp(const QDateTime& dt) const;
    QColor levelColor(const QString& level) const;
    int calculateRowsPerPage() const;

private:
    QTableWidget *table;

    QHBoxLayout* paginationLayout = nullptr;
    QLabel *pageLabel = nullptr;
    QPushButton *prevButton = nullptr;
    QPushButton *nextButton = nullptr;

    int currentPage = 1;      // 从 1 开始
    int totalPages = 1;
    int currentRowsPerPage = 0;
    int totalEventCount = 0;
};

#endif // EVENTPAGE_H
