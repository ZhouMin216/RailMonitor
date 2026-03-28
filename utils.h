#ifndef UTILS_H
#define UTILS_H

#include <QSvgRenderer>

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
