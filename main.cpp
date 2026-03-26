#include <QApplication>
#include <QFont>
#include "ui/MainWindow.h"
#include "database/DatabaseManager.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setFont(QFont("Microsoft YaHei", 10));

    MainWindow w;
    w.showFullScreen();
    return app.exec();
}
