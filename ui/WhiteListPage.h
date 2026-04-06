// WhiteListPage.h

#ifndef WHITELISTPAGE_H
#define WHITELISTPAGE_H

#include <QWidget>
#include <QMap>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QIcon>
#include <QTimeEdit>

struct WhitelistEntry {
    quint32 uid;
    QString name;
    QString department;
};

using WhitelistMap = QMap<quint32, WhitelistEntry>;

// ===== 新增：表格列枚举 =====
enum class Column {
    Uid = 0,
    Name,
    Department,
    Actions,
    Count  // 必须是最后一项
};

namespace Columns {
inline QString header(Column col) {
    switch (col) {
    case Column::Uid:         return "授权ID";
    case Column::Name:        return "姓名";
    case Column::Department:  return "所属部门";
    case Column::Actions:     return "操作";
    default:                  return "";
    }
}

inline int index(Column col) {
    return static_cast<int>(col);
}
}

class WhiteListPage : public QWidget
{
    Q_OBJECT
public:
    explicit WhiteListPage(QWidget *parent = nullptr);

signals:
    void entryAdded(quint32 uid, const QString &name);
    void entryUpdated(quint32 uid, const QString &name);
    void entryRemoved(quint32 uid);

public slots:
    void refreshTable();
    void onAddClicked();
    void onEditClicked(int row);
    void onDeleteClicked(int row);
    void onSaveGlobalConfig();

private:
    void setupUI();
    void populateTable(const WhitelistMap &entries);

    // UI 控件
    QTableWidget *m_table;
    QLineEdit *m_lineName;
    QLineEdit *m_lineUid;
    QLineEdit *m_lineDept;
    QPushButton *m_btnAdd;
    QPushButton *m_btnReset;

    QCheckBox *m_cbOfflineLock;
    QCheckBox *m_cbAlertPush;
    QCheckBox *m_cbAnonymize;
    QPushButton *m_btnSaveGlobal;

    QString m_editingUidStr; // 临时存储正在编辑的 UID 字符串（用于按钮文本切换）

    WhitelistMap m_whitelist;

    QLabel* m_exportPathLabel = nullptr;
    QTimeEdit* m_timeEdit = nullptr;
};

#endif // WHITELISTPAGE_H
