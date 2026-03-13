#include "RailMapViewerWidget.h"
#include "database/DatabaseManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QPen>
#include <QBrush>
#include <QPainter>
#include <QDir>
#include <QJsonObject>
#include <QJsonArray>
#include <QMouseEvent>
#include <QMap>

#include <QMessageBox>
#include <QGraphicsSceneMouseEvent>
#include <QTimer>

RailMapViewerWidget::RailMapViewerWidget(QWidget *parent)
    : QWidget(parent) {
    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene);
    view->setRenderHint(QPainter::Antialiasing, true);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setStyleSheet("background: #000032;");
    // view->setTransform(QTransform().rotate(-20.0));

    lngInput = new QLineEdit;
    lngInput->setPlaceholderText("经度 (e.g., 120.718)");
    latInput = new QLineEdit;
    latInput->setPlaceholderText("纬度 (e.g., 30.759)");
    QPushButton *addBtn = new QPushButton("添加标记");
    coordLabel = new QLabel("点击地图查看坐标");
    coordLabel->setStyleSheet("background-color: #f0f0f0; padding: 4px;");

    connect(addBtn, &QPushButton::clicked, this, &RailMapViewerWidget::add_user_marker);

    rotateInput = new QLineEdit;
    rotateInput->setPlaceholderText("旋转角度 (e.g., -20)");
    QPushButton *rotateBtn = new QPushButton("旋转");
    connect(rotateBtn, &QPushButton::clicked, this, [this]{
            bool ok;
            double angle = rotateInput->text().toDouble(&ok);
            if (ok) {
                view->setTransform(QTransform().rotate(angle));
                rotateInput->clear();
            }
        }
    );

    // 电子围栏按钮
    startFenceBtn = new QPushButton("开始围栏");
    finishFenceBtn = new QPushButton("完成围栏");
    clearFenceBtn = new QPushButton("清除围栏");

    connect(startFenceBtn, &QPushButton::clicked, this, [this]() {
        if (!savedFencePoints.isEmpty()) {
            QMessageBox::StandardButton reply = QMessageBox::question(this, "确认", "已有围栏，是否重新绘制？");
            if (reply != QMessageBox::Yes) return;
        }
        isDrawingFence = true;
        currentFencePoints.clear();
        if (fenceItem) {
            scene->removeItem(fenceItem);
            delete fenceItem;
            fenceItem = nullptr;
        }
        coordLabel->setText("点击地图添加围栏顶点（至少3个）");
    });

    connect(finishFenceBtn, &QPushButton::clicked, this, &RailMapViewerWidget::finishFenceDrawing);
    connect(clearFenceBtn, &QPushButton::clicked, this, &RailMapViewerWidget::clearFence);

    QHBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->addWidget(new QLabel("经度:"));
    controlLayout->addWidget(lngInput);
    controlLayout->addWidget(new QLabel("纬度:"));
    controlLayout->addWidget(latInput);
    controlLayout->addWidget(addBtn);

    controlLayout->addWidget(rotateInput);
    controlLayout->addWidget(rotateBtn);

    controlLayout->addWidget(coordLabel);
    controlLayout->addWidget(startFenceBtn);
    controlLayout->addWidget(finishFenceBtn);
    controlLayout->addWidget(clearFenceBtn);

    QWidget *controlWidget = new QWidget;
    controlWidget->setLayout(controlLayout);
    controlWidget->setFixedHeight(70);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(view);
    mainLayout->addWidget(controlWidget);

    loadConfig();

    // 加载电子围栏
    // savedFencePoints = DatabaseManager::loadGeoFence();

    drawAll();
}

 void RailMapViewerWidget::loadGeoFence()
{
     qDebug() << "=============== RailMapViewerWidget::loadGeoFence ====================";
     emit getGeoFence();
     qDebug() << "===================================";
}

void RailMapViewerWidget::loadConfig() {
    // QString configPath = QDir::currentPath() + "/config.json";
    QString configPath = QDir::currentPath() + "/rail_config.json";
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "错误", "配置文件未找到：" + configPath);
        return;
    }

    QByteArray data = file.readAll();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        QMessageBox::critical(this, "错误", "配置文件格式错误");
        return;
    }
    QJsonObject root = doc.object();

    // 用于计算边界的临时变量
    // bool hasValidCoord = false;
    // double minLatVal = 90.0, maxLatVal = -90.0;
    // double minLngVal = 180.0, maxLngVal = -180.0;

    // auto updateBounds = [&](double lng, double lat) {
    //     hasValidCoord = true;
    //     if (lat < minLatVal) minLatVal = lat;
    //     if (lat > maxLatVal) maxLatVal = lat;
    //     if (lng < minLngVal) minLngVal = lng;
    //     if (lng > maxLngVal) maxLngVal = lng;
    // };


    railTracks_.clear();
    const QJsonArray& tracksArray = root["tracks"].toArray();
    for (const QJsonValue &val : tracksArray) {
        if (!val.isObject()) continue;
        QJsonObject trackObj = val.toObject();
        RailTrack newTrack;

        // 提取基础信息
        newTrack.number = trackObj["number"].toInt(-1);
        newTrack.name = trackObj["number"].toString();

        // 解析 Master 轨道
        QList<QPointF> masterPoints;
        if (trackObj.contains("master") && trackObj["master"].isArray()) {
            for (const QJsonValue &ptVal : trackObj["master"].toArray()) {
                if (ptVal.isArray()) {
                    QJsonArray ptArr = ptVal.toArray();
                    if (ptArr.size() >= 2) {
                        double lng = ptArr[0].toDouble();
                        double lat = ptArr[1].toDouble();

                        masterPoints.append(QPointF(lng, lat));
                        // updateBounds(lng, lat);
                    }
                }
            }
        }

        // 解析 Slave 轨道
        QList<QPointF> slavePoints;
        if (trackObj.contains("slave") && trackObj["slave"].isArray()) {
            for (const QJsonValue &ptVal : trackObj["slave"].toArray()) {
                if (ptVal.isArray()) {
                    QJsonArray ptArr = ptVal.toArray();
                    if (ptArr.size() >= 2) {
                        double lng = ptArr[0].toDouble();
                        double lat = ptArr[1].toDouble();
                        slavePoints.append(QPointF(lng, lat));
                        // updateBounds(lng, lat);
                    }
                }
            }
        }

        newTrack.masterPoints = masterPoints;
        newTrack.slavePoints = slavePoints;
        railTracks_.append(newTrack);
    }

    tracks.clear();
    buildings.clear();
    buildings_.clear();

    const QJsonArray& buildingsArray = root["buildings"].toArray();
    for (const QJsonValue &val : buildingsArray) {
        if (!val.isObject()) continue;
        QJsonObject bObj = val.toObject();
        Building newBuilding;
        newBuilding.name = bObj["name"].toString();

        // buildings.append(bObj.toVariantMap());

        if (bObj.contains("coordinates") && bObj["coordinates"].isArray()) {
            for (const QJsonValue &ptVal : bObj["coordinates"].toArray()) {
                if (ptVal.isArray()) {
                    QJsonArray ptArr = ptVal.toArray();
                    if (ptArr.size() >= 2) {
                        double lng = ptArr[0].toDouble();
                        double lat = ptArr[1].toDouble();
                        newBuilding.points.append(QPointF(lng, lat));
                        // updateBounds(lng, lat);
                    }
                }
            }
        }
        buildings_.append(newBuilding);
    }

    // const QJsonArray& shoeCabinetArray = root["shoeCabinets"].toArray();
    // for (const QJsonValue &val : shoeCabinetArray) {
    //     if (val.isObject()) {
    //         QJsonObject p = val.toObject();
    //         QString name = p["name"].toString();
    //         double lng = p["lng"].toDouble();
    //         double lat = p["lat"].toDouble();
    //         shoeCabinetPoints[name] = QPointF(lat,lng);
    //     }
    // }

    // 4. 更新全局边界变量
    // if (hasValidCoord) {
    //     minLat = minLatVal;
    //     maxLat = maxLatVal;
    //     minLng = minLngVal;
    //     maxLng = maxLngVal;

    //     // 防止跨度为0导致除零错误或缩放异常
    //     latSpan = qMax(maxLat - minLat, 1e-8);
    //     lngSpan = qMax(maxLng - minLng, 1e-8);
    // } else {
    //     // 默认值处理
    //     minLat = maxLat = 0.0;
    //     minLng = maxLng = 0.0;
    //     latSpan = lngSpan = 1.0;
    // }
    // return;

    if (railTracks_.isEmpty() || buildings_.isEmpty()) return;

    minLat = maxLat = buildings_.first().points.first().y();
    minLng = maxLng = buildings_.first().points.first().x();


    auto updateBounds = [&](const QPointF &pt) {
        minLat = qMin(minLat, pt.y());
        maxLat = qMax(maxLat, pt.y());
        minLng = qMin(minLng, pt.x());
        maxLng = qMax(maxLng, pt.x());
    };

    for(const auto &track :railTracks_){
        for (const QPointF &pt : track.masterPoints) updateBounds(pt);
        for (const QPointF &pt : track.slavePoints) updateBounds(pt);
    }

    for(const auto &build :buildings_){
        for (const QPointF &pt : build.points) updateBounds(pt);
    }

    // for (const QPointF &pt : viaPoints) updateBounds(pt);
    // for (const QPointF &pt : buildPoints) updateBounds(pt);

    latSpan = qMax(maxLat - minLat, 1e-8);
    lngSpan = qMax(maxLng - minLng, 1e-8);
}

QPointF RailMapViewerWidget::geoToPixel(double lng, double lat) {
    qreal x = (lng - minLng) / lngSpan * h_pixel + marign;
    qreal y = (maxLat - lat) / latSpan * v_pixel + marign;
    return QPointF(x, y);
}

QPointF RailMapViewerWidget::pixelToGeo(qreal x, qreal y) {
    double lng = minLng + (x - marign) / h_pixel * lngSpan;
    double lat = maxLat - (y - marign) / v_pixel * latSpan;
    return QPointF(lng, lat);
}

void RailMapViewerWidget::drawAll() {
    scene->clear();
    drawTracks();
    drawBuildings();
    // drawViaPoints();
    // drawFence();
    // drawShoeCabinet();

    return;
    // 旋转视图
    QRectF sceneRect = scene->itemsBoundingRect();

    if (sceneRect.isEmpty()) {
        return; // 如果没有内容，直接返回
    }

    // B. 定义旋转角度
    double angle = -24.0;

    // C. 创建变换矩阵 (QTransform)
    QTransform transform;

    // 核心技巧：绕着场景的中心点旋转，而不是绕着 (0,0)
    QPointF center = sceneRect.center();

    // 1. 将中心点移到原点
    transform.translate(center.x(), center.y());
    // 2. 执行旋转
    transform.rotate(angle);
    // 3. 移回原位置 (注意：这里移回的是原来的中心坐标)
    transform.translate(-center.x(), -center.y());

    // D. 应用变换到 View
    view->setTransform(transform);

    // E. 【最重要的一步】重新计算旋转后的实际包围盒，并强制缩放视图
    // 当应用了 setTransform 后，scene->itemsBoundingRect() 返回的仍然是逻辑坐标（未旋转的）
    // 我们需要用 transform.mapRect() 获取旋转后的物理包围盒
    QRectF rotatedRect = transform.mapRect(sceneRect);

    // // 强制视图适应这个旋转后的新矩形，这会自动去除滚动条
    // view->fitInView(rotatedRect, Qt::KeepAspectRatio);

    // 可选：设置一点边距，防止内容紧贴边缘
    // view->fitInView(rotatedRect.adjusted(-10, -10, 10, 10), Qt::KeepAspectRatio);

    // plan b
    // A. 获取视图当前的大小 (可视区域)
    QSize viewSize = view->viewport()->size();

    // B. 计算原始内容的大小 (未旋转前的宽高)
    // qreal contentWidth = sceneRect.width();
    // qreal contentHeight = sceneRect.height();

    // C. 设定一个“填充系数” (Padding Factor)
    // 0.9 表示内容占视图的 90%，留 10% 边距
    // 如果觉得还是太小，可以调大到 0.95 或 0.98
    double paddingFactor = 1.9;

    // D. 计算需要的缩放比例 (Scale)
    // 逻辑：让内容的宽/高 (取较小比例的那个) 刚好占满视图宽/高的 paddingFactor
    // qreal scaleX = (viewSize.width() * paddingFactor) / contentWidth;
    // qreal scaleY = (viewSize.height() * paddingFactor) / contentHeight;

    qreal scaleX = (viewSize.width() * paddingFactor) / rotatedRect.width();
    qreal scaleY = (viewSize.height() * paddingFactor) / rotatedRect.height();

    // 选择较小的缩放比例，确保内容完全在视野内 (Keep Aspect Ratio)
    qreal finalScale = std::min(scaleX, scaleY);

    // E. 应用缩放
    // 注意：setTransform 会覆盖之前的变换，所以要把 旋转 + 缩放 组合起来
    QTransform finalTransform = transform;
    finalTransform.scale(finalScale, finalScale);

    // 重新应用组合后的变换 (先旋转，再缩放)
    // 注意：矩阵乘法顺序很重要。通常我们希望先旋转，再以新的尺度显示。
    // 但在这里，我们是在 view 层级操作。
    // 更稳妥的方式是：先设旋转，再 scale view 的 matrix

    view->setTransform(transform); // 先恢复纯旋转状态
    view->scale(finalScale, finalScale); // 在此基础上缩放

    // F. 居中视图
    // 现在内容可能不在中心，因为缩放和旋转改变了坐标映射
    // 我们直接将视图的中心点设置场景的中心点
    view->centerOn(center);
}

void RailMapViewerWidget::drawTracks() {
    QPen pen(QColor(180, 255, 255), 1);

    for (const RailTrack &track : railTracks_) {
        // QPolygonF poly;
        for (int i = 0; i < track.masterPoints.size() - 1; ++i)
        // for (const QPointF &geo : track.masterPoints)
        {
            QPointF p1 = geoToPixel(track.masterPoints[i].x(), track.masterPoints[i].y());
            QPointF p2 = geoToPixel(track.masterPoints[i+1].x(), track.masterPoints[i+1].y());
            scene->addLine(p1.x(), p1.y(), p2.x(), p2.y(), pen);
        }

        for (int i = 0; i < track.slavePoints.size() - 1; ++i)
        // for (const QPointF &geo : track.masterPoints)
        {
            QPointF p1 = geoToPixel(track.slavePoints[i].x(), track.slavePoints[i].y());
            QPointF p2 = geoToPixel(track.slavePoints[i+1].x(), track.slavePoints[i+1].y());
            scene->addLine(p1.x(), p1.y(), p2.x(), p2.y(), pen);
        }
    }
}

void RailMapViewerWidget::drawBuildings() {
    QPen pen(QColor(165, 42, 42), 2);
    QBrush brush(QColor(245, 222, 179, 80));
    QFont font;
    font.setPointSize(10);

    for (const Building &building : buildings_) {
        QPolygonF poly;
        for (const QPointF &geo : building.points) {
                QPointF px = geoToPixel(geo.x(), geo.y());
                poly << px;
        }
        if (poly.size() < 3) continue;
        scene->addPolygon(poly, pen, brush);

        qreal cx = 0, cy = 0;
        for (const QPointF &p : poly) { cx += p.x(); cy += p.y(); }
        cx /= poly.size(); cy /= poly.size();
        QGraphicsTextItem *text = scene->addText(building.name, font);
        text->setPos(cx - text->boundingRect().width()/2, cy - text->boundingRect().height()/2);
    }
    return;
    for (const QVariant &bldVar : buildings) {
        QVariantMap bld = bldVar.toMap();
        QStringList corners = bld["corner"].toStringList();
        QPolygonF poly;
        for (const QString &cid : corners) {
            if (buildPoints.contains(cid)) {
                QPointF geo = buildPoints[cid];
                QPointF px = geoToPixel(geo.x(), geo.y());
                poly << px;
            }
        }
        if (poly.size() < 3) continue;
        scene->addPolygon(poly, pen, brush);

        qreal cx = 0, cy = 0;
        for (const QPointF &p : poly) { cx += p.x(); cy += p.y(); }
        cx /= poly.size(); cy /= poly.size();
        QGraphicsTextItem *text = scene->addText(bld["name"].toString(), font);
        text->setPos(cx - text->boundingRect().width()/2, cy - text->boundingRect().height()/2);
    }
}

void RailMapViewerWidget::drawViaPoints() {
    for (const QPointF &pt : viaPoints) {
        QPointF px = geoToPixel(pt.x(), pt.y());
        scene->addEllipse(px.x()-2, px.y()-2, 4, 4, QPen(Qt::black), Qt::red);
    }
}

void RailMapViewerWidget::drawFence() {
    if (savedFencePoints.size() < 3) return;

    QPolygonF poly;
    for (const QPointF& geo : savedFencePoints) {
        poly << geoToPixel(geo.x(), geo.y());
    }
    poly << geoToPixel(savedFencePoints.first().x(), savedFencePoints.first().y());

    scene->addPolygon(poly, QPen(Qt::red, 3), QBrush(QColor(255, 0, 0, 30)));
}

void RailMapViewerWidget::drawShoeCabinet(){
    shoeCabinet.clear();
    for (QMap<QString, QPointF>::const_iterator it = shoeCabinetPoints.constBegin();
         it != shoeCabinetPoints.constEnd(); ++it) {
        QString name = it.key();
        QPointF pt = it.value();
        ShoeCabinetItem *marker = new ShoeCabinetItem(name, pt.x(), pt.y(), scene, this);
        shoeCabinet[name] = marker;
        scene->addItem(marker);
    }
}

void RailMapViewerWidget::updateFencePreview() {
    if (fenceItem) {
        scene->removeItem(fenceItem);
        delete fenceItem;
        fenceItem = nullptr;
    }

    if (currentFencePoints.size() < 2) return;

    QPolygonF poly;
    for (const QPointF& geo : currentFencePoints) {
        poly << geoToPixel(geo.x(), geo.y());
    }
    if (currentFencePoints.size() >= 3) {
        poly << geoToPixel(currentFencePoints.first().x(), currentFencePoints.first().y());
    }

    // fenceItem = scene->addPolygon(poly, QPen(Qt::yellow, 2), QBrush(Qt::yellow, 50));
    QColor yellowColor = Qt::yellow;
    yellowColor.setAlpha(50); // 半透明
    fenceItem = scene->addPolygon(poly, QPen(Qt::yellow, 2), QBrush(yellowColor));
}

void RailMapViewerWidget::finishFenceDrawing() {
    // test
    for (QMap<QString, ShoeCabinetItem*>::const_iterator it = shoeCabinet.constBegin();
         it != shoeCabinet.constEnd(); ++it) {
        QString name = it.key();
        ShoeCabinetItem* Cabinet = it.value();
        qDebug() << name << " " << Cabinet->Name();
    }
    //

    if (currentFencePoints.size() < 3) {
        QMessageBox::warning(this, "警告", "围栏至少需要3个点！");
        return;
    }

    savedFencePoints = currentFencePoints;
    isDrawingFence = false;

    // DatabaseManager::saveGeoFence(savedFencePoints);
    emit saveGeoFence(savedFencePoints);

    drawAll();
    coordLabel->setText("电子围栏已设置");
}

void RailMapViewerWidget::clearFence() {
    savedFencePoints.clear();
    currentFencePoints.clear();
    isDrawingFence = false;

    QSqlQuery query;
    query.exec("DELETE FROM geo_fence WHERE id = 1");

    drawAll();
    coordLabel->setText("电子围栏已清除");
}

void RailMapViewerWidget::handleIncomingFencePoint(const QList<QPointF>& data)
{
    savedFencePoints = data;
}

void RailMapViewerWidget::add_user_marker() {
    bool ok1, ok2;
    double lng = lngInput->text().toDouble(&ok1);
    double lat = latInput->text().toDouble(&ok2);
    if (!ok1 || !ok2) {
        QMessageBox::critical(this, "错误", "请输入有效的经纬度！");
        return;
    }

    addDeviceMarker("用户设备", lat, lng);
    lngInput->clear();
    latInput->clear();

    if (!savedFencePoints.isEmpty()) {
        QPointF pt(lng, lat);
        if (!pointInPolygon(pt, savedFencePoints)) {
            QMessageBox::warning(this, "警告", "该坐标不在电子围栏范围内！");
            // return;
        }
    }
}

void RailMapViewerWidget::addDeviceMarker(const QString &name, double lat, double lon) {
    DeviceMarkerItem *marker = new DeviceMarkerItem(name, lat, lon, scene, this);
    scene->addItem(marker);
}

void RailMapViewerWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        QPoint viewPos = view->mapFromParent(event->pos());
        QRect viewportRect = view->viewport()->rect();
        if (!viewportRect.contains(viewPos)) {
            QWidget::mousePressEvent(event);
            return;
        }

        QPointF scenePos = view->mapToScene(viewPos);
        QPointF geo = pixelToGeo(scenePos.x(), scenePos.y());

        if (isDrawingFence) {
            currentFencePoints.append(geo);
            updateFencePreview();
            coordLabel->setText(QString("已添加 %1 个点").arg(currentFencePoints.size()));
        } else {
            coordLabel->setText(QString("经度: %1, 纬度: %2")
                                    .arg(geo.x(), 0, 'f', 6)
                                    .arg(geo.y(), 0, 'f', 6));
        }
    }
    QWidget::mousePressEvent(event);
}

void RailMapViewerWidget::resizeEvent(QResizeEvent *event) {
    QSize vpSize = view->size();
    h_pixel = vpSize.width() - 2 * marign;
    v_pixel = vpSize.height() - 2 * marign;
    if (h_pixel < 100) h_pixel = 100;
    if (v_pixel < 100) v_pixel = 100;

    drawAll();
    QWidget::resizeEvent(event);
}

bool RailMapViewerWidget::pointInPolygon(const QPointF& point, const QList<QPointF>& polygon) {
    int n = polygon.size();
    if (n < 3) return false;

    bool inside = false;
    for (int i = 0, j = n - 1; i < n; j = i++) {
        if (((polygon[i].y() > point.y()) != (polygon[j].y() > point.y())) &&
            (point.x() < (polygon[j].x() - polygon[i].x()) * (point.y() - polygon[i].y()) / (polygon[j].y() - polygon[i].y()) + polygon[i].x())) {
            inside = !inside;
        }
    }
    return inside;
}

DeviceMarkerItem::DeviceMarkerItem(const QString &name, double lat, double lon, QGraphicsScene *scene, RailMapViewerWidget *parent)
    : deviceName(name), latitude(lat), longitude(lon), parentWidget(parent) {

    // 创建椭圆
    QGraphicsEllipseItem *ellipse = new QGraphicsEllipseItem(-4, -4, 8, 8);
    ellipse->setPen(QPen(Qt::darkGreen, 2));
    ellipse->setBrush(Qt::green);
    ellipse->setFlag(QGraphicsItem::ItemIsSelectable); // 启用选择
    ellipse->setFlag(QGraphicsItem::ItemIsFocusable); // 启用焦点

    // 创建文本
    QGraphicsTextItem *text = new QGraphicsTextItem(name);
    text->setDefaultTextColor(Qt::black); // 可选：设置文字颜色
    text->setPos(-5, -5); // 相对于椭圆的位置

    // 将子项添加到组中
    addToGroup(ellipse);
    addToGroup(text);

    // 设置组的坐标 (相对于场景)
    QPointF px = parentWidget->geoToPixel(lon, lat);
    setPos(px.x(), px.y());

    // 初始化模拟参数 (示例数据)
    simulatedParams["温度"] = 25.5;
    simulatedParams["电压"] = 12.3;
    simulatedParams["状态"] = "运行中";
    simulatedParams["信号强度"] = 85;
}

void DeviceMarkerItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    qDebug() << "DeviceMarkerItem clicked:" << deviceName;
    if(isShowingDetails) return;
    // 调用父类处理默认行为 (如选择)
    QGraphicsItemGroup::mousePressEvent(event);

    // 显示设备属性信息
    if (event->button() == Qt::LeftButton) {
        // 使用 QTimer::singleShot 延迟显示对话框
        isShowingDetails = true;
        QTimer::singleShot(0, [this]() {
            /*
            // 构建信息字符串
            QString info = QString("设备名称: %1\n").arg(deviceName);
            info += QString("经度: %1\n").arg(longitude, 0, 'f', 6);
            info += QString("纬度: %1\n").arg(latitude, 0, 'f', 6);
            info += "\n模拟参数:\n";

            for (auto it = simulatedParams.constBegin(); it != simulatedParams.constEnd(); ++it) {
                info += QString("%1: %2\n").arg(it.key()).arg(it.value().toString());
            }

            // 显示对话框
            QMessageBox::information(nullptr, "设备属性", info);
            */
            DeviceDetailsDialog dialog(deviceName, latitude, longitude, simulatedParams, nullptr);
            dialog.exec(); // 模态显示，无系统声音

            // 对话框关闭后，重置标志位
            isShowingDetails = false;
            qDebug() << "DeviceMarkerItem exit";
        });
    }
}

ShoeCabinetItem::ShoeCabinetItem(const QString &name, double lat, double lon, QGraphicsScene *scene, RailMapViewerWidget *parent)
    : deviceName(name), latitude(lat), longitude(lon), parentWidget(parent) {

    // 创建椭圆
    QGraphicsRectItem *rect = new QGraphicsRectItem(-4, -4, 16, 16);
    rect->setPen(QPen(Qt::darkGreen, 2));
    rect->setBrush(Qt::green);
    rect->setFlag(QGraphicsItem::ItemIsSelectable); // 启用选择
    rect->setFlag(QGraphicsItem::ItemIsFocusable); // 启用焦点

    // 创建文本
    QGraphicsTextItem *text = new QGraphicsTextItem(name);
    text->setDefaultTextColor(Qt::black); // 可选：设置文字颜色
    text->setPos(5, -5); // 相对于矩形的位置

    // 将子项添加到组中
    addToGroup(rect);
    addToGroup(text);

    // 设置组的坐标 (相对于场景)
    QPointF px = parentWidget->geoToPixel(lon, lat);
    setPos(px.x(), px.y());

    // 初始化模拟参数 (示例数据)
    simulatedParams["温度"] = 25.5;
    simulatedParams["铁鞋容量"] = 8;
    simulatedParams["状态"] = "运行中";
    simulatedParams["信号强度"] = 85;
}

void ShoeCabinetItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    qDebug() << "ShoeCabinetItem clicked:" << deviceName;
    if(isShowingDetails) return;
    // 调用父类处理默认行为 (如选择)
    QGraphicsItemGroup::mousePressEvent(event);

    // 显示设备属性信息
    if (event->button() == Qt::LeftButton) {
        // 使用 QTimer::singleShot 延迟显示对话框
        isShowingDetails = true;
        QTimer::singleShot(0, [this]() {
            DeviceDetailsDialog dialog(deviceName, latitude, longitude, simulatedParams, nullptr);
            dialog.exec(); // 模态显示，无系统声音

            // 对话框关闭后，重置标志位
            isShowingDetails = false;
            qDebug() << "DeviceMarkerItem exit";
        });
    }
}

DeviceDetailsDialog::DeviceDetailsDialog(const QString &name, double lat, double lon,
                                         const QMap<QString, QVariant> &params, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("设备属性");
    setModal(true); // 设置为模态对话框

    QVBoxLayout *layout = new QVBoxLayout(this);

    QString info = QString("<b>设备名称:</b> %1<br>").arg(name);
    info += QString("<b>经度:</b> %1<br>").arg(lon, 0, 'f', 6);
    info += QString("<b>纬度:</b> %1<br><br>").arg(lat, 0, 'f', 6);
    info += "<b>模拟参数:</b><br>";

    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        info += QString("%1: %2<br>").arg(it.key()).arg(it.value().toString());
    }

    infoLabel = new QLabel(info);
    infoLabel->setTextFormat(Qt::RichText); // 支持 HTML 格式
    layout->addWidget(infoLabel);

    QPushButton *okButton = new QPushButton("确定");
    connect(okButton, &QPushButton::clicked, this, &DeviceDetailsDialog::onOkClicked);
    layout->addWidget(okButton);

    setLayout(layout);
}

void DeviceDetailsDialog::onOkClicked() {
    accept(); // 关闭对话框并返回 Accepted
}
