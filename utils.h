#ifndef UTILS_H
#define UTILS_H

#include <QSvgRenderer>
#include <QPainter>
#include <QMessageBox>

namespace NonModalMessageBox {
inline void warning(QWidget *parent, const QString &title, const QString &text) {
    QMessageBox *msg = new QMessageBox(parent);
    msg->setWindowTitle(title);
    msg->setText(text);
    msg->setIcon(QMessageBox::Warning);
    msg->setStandardButtons(QMessageBox::Ok);
    msg->setWindowModality(Qt::NonModal);          // 关键：非模态
    msg->setAttribute(Qt::WA_DeleteOnClose);       // 自动销毁
    msg->show();
}

// 可按需添加 information / critical 等
}

namespace CustomColors {
    inline const QColor& offlineFillColor() {
        static const QColor color{ "#ffd166" };
        return color;
    }

    inline const QColor& offlineBorderColor() {
        static const QColor color{ "#ffaa33" };
        return color;
    }

    inline const QColor& unusualFillColor() {
        static const QColor color{ "#ff0000" };
        return color;
    }

    inline const QColor& unusualBorderColor() {
        static const QColor color{ "#ff4444" };
        return color;
    }

    inline const QColor& onlineFillColor() {
        static const QColor color{ "#00ffcc" };
        return color;
    }

    inline const QColor& onlineBorderColor() {
        static const QColor color{ "#44ff44" };
        return color;
    }

    inline const QColor& buildingBorderColor() {
        static const QColor color(180, 180, 190, 80);  // 灰色
        return color;
    }

    inline const QColor& cabinetColor() {
        static const QColor color(74, 106, 255, 255);
        return color;
    }




}

static QPixmap coloredSvg(const QString &path, const QColor &color, int w, int h)
{
    QSvgRenderer renderer(path);
    if (!renderer.isValid()) {
        qWarning() << "Invalid SVG:" << path;
        return QPixmap();
    }

    // 创建透明 pixmap
    QPixmap pixmap(w, h);
    pixmap.fill(Qt::transparent); // 透明底

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // 渲染 SVG 到 pixmap（此时是黑色，但保留 alpha）
    renderer.render(&painter);

    // 用 CompositionMode_SourceIn 填充颜色（只影响非透明部分）
    // 先用指定颜色填充整个区域（作为“画笔”）
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), color);

    return pixmap;
}

#endif // UTILS_H
