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
#include <QThread>
#include <QMessageBox>
#include <QGraphicsSceneMouseEvent>
#include <QTimer>
#include <QGraphicsDropShadowEffect>  // 新增
#include <QRadialGradient>            // 新增
#include <QScrollBar>
#include "DeviceManager.h"

RailMapViewerWidget::RailMapViewerWidget(QWidget *parent)
    : QWidget(parent) {
    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene);
    view->setRenderHint(QPainter::Antialiasing, true);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setStyleSheet("background: transparent; border: none;"); // 深空黑背景
    // 隐藏滚动条
    // view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    lngInput = new QLineEdit;
    lngInput->setPlaceholderText("经度 (e.g., 120.718)");
    latInput = new QLineEdit;
    latInput->setPlaceholderText("纬度 (e.g., 30.759)");
    QPushButton *addBtn = new QPushButton("添加标记");
    coordLabel = new QLabel("点击地图查看坐标");
    coordLabel->setStyleSheet("background-color: #1a1a2e; color: #00ffcc; padding: 4px; border-radius: 3px;");

    connect(addBtn, &QPushButton::clicked, this, &RailMapViewerWidget::add_user_marker);

    rotateInput = new QLineEdit;
    rotateInput->setPlaceholderText("旋转角度 (e.g., -20)");
    QPushButton *rotateBtn = new QPushButton("旋转");
    connect(rotateBtn, &QPushButton::clicked, this, [this]{
        bool ok;
        double angle = rotateInput->text().toDouble(&ok);
        if (ok) {
            view->setTransform(QTransform().rotate(angle));
            view->fitInView(scene->sceneRect(), Qt::KeepAspectRatioByExpanding);
            view->verticalScrollBar()->setValue(view->verticalScrollBar()->maximum());
            rotateInput->clear();
        }
    });

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

    QLabel *titleLabel = new QLabel("石家庄车辆段铁鞋智能管理");
    titleLabel->setAlignment(Qt::AlignCenter); // 居中对齐
    titleLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #ffffff; padding: 10px; background: transparent;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(titleLabel);  // 添加标题
    mainLayout->addWidget(view);
    mainLayout->addWidget(controlWidget);

    loadConfig();
    drawAll();

    // 延迟执行 fitInView
    // QTimer::singleShot(0, this, [this]() {
    //     view->setTransform(QTransform().rotate(-24.5));
    //     view->fitInView(scene->sceneRect(), Qt::KeepAspectRatioByExpanding);
    //     // view->verticalScrollBar()->setValue(view->verticalScrollBar()->maximum());
    // });

    // connect(view, &QGraphicsView::resizeEvent, this, &RailMapViewerWidget::updateLegendPosition);
    connect(view->horizontalScrollBar(), &QAbstractSlider::valueChanged, this, &RailMapViewerWidget::updateLegendPosition);
    connect(view->verticalScrollBar(), &QAbstractSlider::valueChanged, this, &RailMapViewerWidget::updateLegendPosition);
}

RailMapViewerWidget::~RailMapViewerWidget() {
    // 在 view 被销毁前，断开滚动条信号
    if (view) {
        disconnect(view->horizontalScrollBar(), nullptr, this, nullptr);
        disconnect(view->verticalScrollBar(), nullptr, this, nullptr);
    }
}

void RailMapViewerWidget::updateLegendPosition() {
    if (!legendGroup) return;

    QSize vp = view->viewport()->size();
    qreal margin = 10;
    QPointF scenePos = view->mapToScene(
        vp.width() - legendGroup->boundingRect().width() - margin,
        vp.height() - legendGroup->boundingRect().height() - margin
        );
    legendGroup->setPos(scenePos);
}

void RailMapViewerWidget::loadGeoFence()
{
    qDebug() << "=============== RailMapViewerWidget::loadGeoFence ====================";
    emit getGeoFence();
    qDebug() << "===================================";
}

void RailMapViewerWidget::loadConfig() {
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

    railTracks_.clear();
    const QJsonArray& tracksArray = root["tracks"].toArray();
    for (const QJsonValue &val : tracksArray) {
        if (!val.isObject()) continue;
        QJsonObject trackObj = val.toObject();
        RailTrack newTrack;
        newTrack.number = trackObj["number"].toInt(-1);
        newTrack.name = trackObj["number"].toString();

        QList<QPointF> masterPoints;
        if (trackObj.contains("master") && trackObj["master"].isArray()) {
            for (const QJsonValue &ptVal : trackObj["master"].toArray()) {
                if (ptVal.isArray()) {
                    QJsonArray ptArr = ptVal.toArray();
                    if (ptArr.size() >= 2) {
                        double lng = ptArr[0].toDouble();
                        double lat = ptArr[1].toDouble();
                        masterPoints.append(QPointF(lng, lat));
                    }
                }
            }
        }

        QList<QPointF> slavePoints;
        if (trackObj.contains("slave") && trackObj["slave"].isArray()) {
            for (const QJsonValue &ptVal : trackObj["slave"].toArray()) {
                if (ptVal.isArray()) {
                    QJsonArray ptArr = ptVal.toArray();
                    if (ptArr.size() >= 2) {
                        double lng = ptArr[0].toDouble();
                        double lat = ptArr[1].toDouble();
                        slavePoints.append(QPointF(lng, lat));
                    }
                }
            }
        }

        newTrack.masterPoints = masterPoints;
        newTrack.slavePoints = slavePoints;
        railTracks_.append(newTrack);
    }

    buildings_.clear();
    const QJsonArray& buildingsArray = root["buildings"].toArray();
    for (const QJsonValue &val : buildingsArray) {
        if (!val.isObject()) continue;
        QJsonObject bObj = val.toObject();
        Building newBuilding;
        newBuilding.name = bObj["name"].toString();

        if (bObj.contains("coordinates") && bObj["coordinates"].isArray()) {
            for (const QJsonValue &ptVal : bObj["coordinates"].toArray()) {
                if (ptVal.isArray()) {
                    QJsonArray ptArr = ptVal.toArray();
                    if (ptArr.size() >= 2) {
                        double lng = ptArr[0].toDouble();
                        double lat = ptArr[1].toDouble();
                        newBuilding.points.append(QPointF(lng, lat));
                    }
                }
            }
        }
        buildings_.append(newBuilding);
    }

    shoeCabinetPoints.clear();
    const QJsonArray& shoeCabinetArray = root["shoeCabinets"].toArray();
    for (const QJsonValue &val : shoeCabinetArray) {
        if (val.isObject()) {
            QJsonObject p = val.toObject();
            quint16 id = p["id"].toInt();
            double lng = p["lng"].toDouble();
            double lat = p["lat"].toDouble();
            shoeCabinetPoints[id] = QPointF(lat, lng);
        }
    }

    if (railTracks_.isEmpty() || buildings_.isEmpty()) return;

    minLat = maxLat = buildings_.first().points.first().y();
    minLng = maxLng = buildings_.first().points.first().x();

    auto updateBounds = [&](const QPointF &pt) {
        minLat = qMin(minLat, pt.y());
        maxLat = qMax(maxLat, pt.y());
        minLng = qMin(minLng, pt.x());
        maxLng = qMax(maxLng, pt.x());
    };

    for(const auto &track : railTracks_) {
        for (const QPointF &pt : track.masterPoints) updateBounds(pt);
        for (const QPointF &pt : track.slavePoints) updateBounds(pt);
    }

    for(const auto &build : buildings_) {
        for (const QPointF &pt : build.points) updateBounds(pt);
    }

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
    qDebug() << "------------- draw all ------------------------";
    shoeMap.clear();
    scene->clear();
    legendGroup = nullptr;
    fenceItem = nullptr;

    drawTracks();
    drawBuildings();
    drawFence();
    drawShoeCabinet();

    createLegend();

    // ✅ 关键：设置场景矩形为所有图元的包围盒
    QRectF bounds = scene->itemsBoundingRect();
    if (!bounds.isEmpty()) {
        scene->setSceneRect(bounds);

        // 👇 不再 fitInView，而是直接放大！
        view->resetTransform();           // 先清除之前的变换（包括旋转）
        view->rotate(-22.5);              // 应用旋转
        view->scale(1.5, 1.5);            // 放大 x 倍
        view->verticalScrollBar()->setValue(view->verticalScrollBar()->maximum() * 0.85);
    }
}

void RailMapViewerWidget::drawTracks() {
    QPen masterPen(QColor("#00ffff"), 1.2);
    masterPen.setCapStyle(Qt::RoundCap);
    masterPen.setJoinStyle(Qt::RoundJoin);

    QPen slavePen(QColor("#00cccc"), 1.2);
    slavePen.setCapStyle(Qt::RoundCap);
    slavePen.setJoinStyle(Qt::RoundJoin);

    for (const RailTrack &track : railTracks_) {
        for (int i = 0; i < track.masterPoints.size() - 1; ++i) {
            QPointF p1 = geoToPixel(track.masterPoints[i].x(), track.masterPoints[i].y());
            QPointF p2 = geoToPixel(track.masterPoints[i+1].x(), track.masterPoints[i+1].y());
            QGraphicsLineItem* line = scene->addLine(p1.x(), p1.y(), p2.x(), p2.y(), masterPen);

            // QGraphicsDropShadowEffect* glow = new QGraphicsDropShadowEffect();
            // glow->setColor(QColor(0, 255, 255, 60));
            // glow->setBlurRadius(4);
            // glow->setOffset(0, 0);
            // line->setGraphicsEffect(glow);
        }

        for (int i = 0; i < track.slavePoints.size() - 1; ++i) {
            QPointF p1 = geoToPixel(track.slavePoints[i].x(), track.slavePoints[i].y());
            QPointF p2 = geoToPixel(track.slavePoints[i+1].x(), track.slavePoints[i+1].y());
            scene->addLine(p1.x(), p1.y(), p2.x(), p2.y(), slavePen);
        }
    }
}

void RailMapViewerWidget::drawBuildings() {
    QBrush brush(QColor(20, 20, 40, 180));
    QPen pen(QColor("#4a6aff"), 2.0);
    pen.setJoinStyle(Qt::MiterJoin);

    QFont font("Consolas", 10, QFont::Bold);
    font.setLetterSpacing(QFont::PercentageSpacing, 110);

    for (const Building &building : buildings_) {
        QPolygonF poly;
        for (const QPointF &geo : building.points) {
            QPointF px = geoToPixel(geo.x(), geo.y());
            poly << px;
        }
        if (poly.size() < 3) continue;

        QGraphicsPolygonItem* polygon = scene->addPolygon(poly, pen, brush);

        QGraphicsDropShadowEffect* innerGlow = new QGraphicsDropShadowEffect();
        innerGlow->setColor(QColor(74, 106, 255, 40));
        innerGlow->setBlurRadius(8);
        innerGlow->setOffset(0, 0);
        polygon->setGraphicsEffect(innerGlow);

        qreal cx = 0, cy = 0;
        for (const QPointF &p : poly) { cx += p.x(); cy += p.y(); }
        cx /= poly.size(); cy /= poly.size();

        QGraphicsTextItem *text = scene->addText(building.name, font);
        text->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        text->setDefaultTextColor(QColor("#ffffff"));
        text->setPos(cx - text->boundingRect().width()/2, cy - text->boundingRect().height()/2);
    }
}

void RailMapViewerWidget::drawFence() {
    if (savedFencePoints.size() < 3) return;

    QPolygonF poly;
    for (const QPointF& geo : savedFencePoints) {
        poly << geoToPixel(geo.x(), geo.y());
    }
    poly << geoToPixel(savedFencePoints.first().x(), savedFencePoints.first().y());

    fenceItem = scene->addPolygon(poly, QPen(QColor("#ff0000"), 2.5, Qt::DashLine));
    fenceItem->setZValue(100);
}

void RailMapViewerWidget::drawShoeCabinet(){
    shoeCabinet.clear();
    for (auto it = shoeCabinetPoints.constBegin(); it != shoeCabinetPoints.constEnd(); ++it) {
        QString name = QString("鞋柜%1").arg(it.key());
        QPointF pt = it.value();
        ShoeCabinetItem *marker = new ShoeCabinetItem(name, pt.x(), pt.y(), scene, this);
        marker->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        shoeCabinet[it.key()] = marker;
        scene->addItem(marker);
    }
    dataUpdated();
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

    QColor yellowColor = Qt::yellow;
    yellowColor.setAlpha(50);
    fenceItem = scene->addPolygon(poly, QPen(Qt::yellow, 2), QBrush(yellowColor));
}

void RailMapViewerWidget::finishFenceDrawing() {
    if (currentFencePoints.size() < 3) {
        QMessageBox::warning(this, "警告", "围栏至少需要3个点！");
        return;
    }

    savedFencePoints = currentFencePoints;
    isDrawingFence = false;
    emit saveGeoFence(savedFencePoints);

    drawAll(); // 会调用 drawFence()

    coordLabel->setText("电子围栏已设置");
}

void RailMapViewerWidget::clearFence() {
    emit clearGeoFence();
    savedFencePoints.clear();
    currentFencePoints.clear();
    isDrawingFence = false;
    if (fenceItem) {
        scene->removeItem(fenceItem);
        delete fenceItem;
        fenceItem = nullptr;
    }
    drawAll();
    coordLabel->setText("电子围栏已清除");
}

void RailMapViewerWidget::updateCabinets(const QList<CabinetData>& data)
{
    for(const auto& cabinet : data) {
        if (shoeCabinet.contains(cabinet.wDevID)) {
            shoeCabinet[cabinet.wDevID]->updateData(cabinet);
        }
    }
}

void RailMapViewerWidget::dataUpdated(){
    QMap<quint16, std::shared_ptr<IconShoe>> shoeMap = DeviceManager::instance()->getShoeMap();
    if (!shoeMap.isEmpty()) {
        QList<ShoeData> data_list;
        for (auto it = shoeMap.constBegin(); it != shoeMap.constEnd(); ++it) {
            quint16 key = it.key();
            std::shared_ptr<IconShoe> shoe = it.value();

            if (!shoe) continue; // 安全检查

            ShoeData data = shoe->GetShoeData();
            if (data.byOnline != ShoeStatus::Unregister && data.byOnline != ShoeStatus::NoEnter)
                data_list.push_back(data);
        }
        updateShoes(data_list);
    }

    QMap<quint16, std::shared_ptr<ShoeCabinet>> cabinetMap = DeviceManager::instance()->getCabinetMap();
    if (!cabinetMap.isEmpty()) {
        QList<CabinetData> data_list;
        for (auto it = cabinetMap.constBegin(); it != cabinetMap.constEnd(); ++it) {
            quint16 key = it.key();
            std::shared_ptr<ShoeCabinet> cabinet = it.value();

            if (!cabinet) continue; // 安全检查

            CabinetData data = cabinet->GetCabinetData();
            data_list.push_back(data);
        }
        updateCabinets(data_list);
    }
}

void RailMapViewerWidget::updateShoes(const QList<ShoeData>& data)
{
    QStringList outOfFenceIds;
    for(auto shoe : data) {
        bool inPolygon = true;
        if (!savedFencePoints.isEmpty()) {
            shoe.lng = qBound(minLng, shoe.lng, maxLng);
            shoe.lat = qBound(minLat, shoe.lat, maxLat);
            QPointF pt(shoe.lng, shoe.lat);
            if (!pointInPolygon(pt, savedFencePoints)) {
                inPolygon = false;
            }
        }

        DeviceMarkerItem *marker = nullptr;
        if (shoeMap.contains(shoe.wDevID)) {
            marker = shoeMap[shoe.wDevID];
            marker->updateData(shoe);
        } else {
            marker = new DeviceMarkerItem(shoe, scene, this);
            shoeMap[shoe.wDevID] = marker;
            scene->addItem(marker);
        }

        if (marker) {
            if (!inPolygon && marker->isInPolygon()) {
                outOfFenceIds << QString::number(shoe.wDevID);
            }
            marker->setInPolygon(inPolygon);
        }
    }

    if (!outOfFenceIds.isEmpty()) {
        QMessageBox::warning(this, "警告",
                             QString("以下铁鞋已超出电子围栏：%1").arg(outOfFenceIds.join(", ")));
    }
}

void RailMapViewerWidget::handleIncomingFencePoint(const QList<QPointF>& data)
{
    savedFencePoints = data;
    drawAll();
}

void RailMapViewerWidget::add_user_marker() {
    bool ok1, ok2;
    double lng = lngInput->text().toDouble(&ok1);
    double lat = latInput->text().toDouble(&ok2);

    lng = qBound(minLng, lng, maxLng);
    lat = qBound(minLat, lat, maxLat);

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
            QMessageBox::warning(this, "警告", QString("设备超出电子围栏！"));
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

void RailMapViewerWidget::createLegend() {
    if (legendGroup) {
        if (legendGroup->scene() == scene)
            scene->removeItem(legendGroup);
        delete legendGroup;
        legendGroup = nullptr;
    }

    legendGroup = new QGraphicsItemGroup;

    // 设置图例背景（半透明深色面板）
    QRectF bgRect(0, 0, 220, 200);
    QPainterPath path;
    path.addRoundedRect(bgRect, 6, 6);
    QGraphicsPathItem  *bg = new QGraphicsPathItem(path);
    bg->setBrush(QColor(20, 20, 30, 200));
    bg->setPen(QPen(QColor(60, 60, 80), 1));
    // bg->setRadius(6); // 圆角
    legendGroup->addToGroup(bg);

    int y = 10; // 起始 Y 坐标
    const int lineHeight = 25;
    const int iconX = 20;
    const int textX = 70;

    QFont font("Consolas", 14);
    font.setBold(false);

    // 1. 电子围栏：红色虚线
    {
        QPen pen(QColor("#ff0000"), 3.5, Qt::DashLine);
        QGraphicsLineItem *line = new QGraphicsLineItem(iconX, y + 10, iconX + 30, y + 10);
        line->setPen(pen);
        legendGroup->addToGroup(line);

        QGraphicsTextItem *text = new QGraphicsTextItem("电子围栏");
        text->setFont(font);
        text->setDefaultTextColor(Qt::white);
        text->setPos(textX, y);
        legendGroup->addToGroup(text);
        y += lineHeight;
    }

    // 2. 轨道：青绿色实线（master）
    {
        QPen pen(QColor("#00ffff"), 3.5);
        QGraphicsLineItem *line = new QGraphicsLineItem(iconX, y + 12, iconX + 30, y + 12);
        line->setPen(pen);
        legendGroup->addToGroup(line);

        QGraphicsTextItem *text = new QGraphicsTextItem("轨道");
        text->setFont(font);
        text->setDefaultTextColor(Qt::white);
        text->setPos(textX, y);
        legendGroup->addToGroup(text);
        y += lineHeight;
    }

    // 3. 建筑物：蓝紫色填充多边形（简化为矩形）
    {
        QBrush brush(QColor(20, 20, 40, 180));
        QPen pen(QColor("#4a6aff"), 2.0);
        QGraphicsRectItem *rect = new QGraphicsRectItem(iconX, y + 9, 30, 12);
        rect->setPen(pen);
        rect->setBrush(brush);
        legendGroup->addToGroup(rect);

        QGraphicsTextItem *text = new QGraphicsTextItem("建筑物");
        text->setFont(font);
        text->setDefaultTextColor(Qt::white);
        text->setPos(textX, y);
        legendGroup->addToGroup(text);
        y += lineHeight;
    }

    // 4. 鞋柜：紫色小矩形
    {
        QColor fillColor = QColor("#b967ff");
        QColor borderColor = QColor("#9955dd");
        QRadialGradient grad(iconX, y + 6, 12);
        grad.setColorAt(0, QColor("#ffffff"));
        grad.setColorAt(1, fillColor);

        QGraphicsRectItem *rect = new QGraphicsRectItem(iconX, y + 8, 12, 12);
        rect->setPen(QPen(borderColor, 1.5));
        rect->setBrush(grad);
        legendGroup->addToGroup(rect);

        QGraphicsTextItem *text = new QGraphicsTextItem("鞋柜");
        text->setFont(font);
        text->setDefaultTextColor(Qt::white);
        text->setPos(textX, y);
        legendGroup->addToGroup(text);
        y += lineHeight;
    }

    // 5. 铁鞋：圆形（在线状态：青绿色）
    {
        QColor fillColor = QColor("#00ffcc");
        QColor borderColor = QColor("#00ccaa");
        QRadialGradient grad(iconX, y + 6, 15);
        grad.setColorAt(0, fillColor.lighter(130));
        grad.setColorAt(1, fillColor);

        QGraphicsEllipseItem *ellipse = new QGraphicsEllipseItem(iconX, y + 8, 12, 12);
        ellipse->setPen(QPen(borderColor, 1.5));
        ellipse->setBrush(grad);
        legendGroup->addToGroup(ellipse);

        QGraphicsTextItem *text = new QGraphicsTextItem("铁鞋在线");
        text->setFont(font);
        text->setDefaultTextColor(Qt::white);
        text->setPos(textX, y);
        legendGroup->addToGroup(text);
        y += lineHeight;
    }

    {
        QColor fillColor = QColor("#ffd166");
        QColor borderColor = QColor("#ffaa33");
        QRadialGradient grad(iconX, y + 6, 15);
        grad.setColorAt(0, fillColor.lighter(130));
        grad.setColorAt(1, fillColor);

        QGraphicsEllipseItem *ellipse = new QGraphicsEllipseItem(iconX, y + 8, 12, 12);
        ellipse->setPen(QPen(borderColor, 1.5));
        ellipse->setBrush(grad);
        legendGroup->addToGroup(ellipse);

        QGraphicsTextItem *text = new QGraphicsTextItem("铁鞋离线");
        text->setFont(font);
        text->setDefaultTextColor(Qt::white);
        text->setPos(textX, y);
        legendGroup->addToGroup(text);
        y += lineHeight;
    }

    {
        QColor fillColor = QColor("#ff0000");
        QColor borderColor = QColor("#ff4444");
        QRadialGradient grad(iconX, y + 6, 15);
        grad.setColorAt(0, fillColor.lighter(130));
        grad.setColorAt(1, fillColor);

        QGraphicsEllipseItem *ellipse = new QGraphicsEllipseItem(iconX, y + 8, 12, 12);
        ellipse->setPen(QPen(borderColor, 1.5));
        ellipse->setBrush(grad);
        legendGroup->addToGroup(ellipse);

        QGraphicsTextItem *text = new QGraphicsTextItem("铁鞋异常");
        text->setFont(font);
        text->setDefaultTextColor(Qt::white);
        text->setPos(textX, y);
        legendGroup->addToGroup(text);
    }

    // 关键：让图例不随地图变换（缩放/旋转）而变化
    legendGroup->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    legendGroup->setZValue(1000); // 确保在最上层

    scene->addItem(legendGroup);

    QSize vp = view->viewport()->size();
    qreal margin = 10;
    QPointF scenePos = view->mapToScene(
        vp.width() - legendGroup->boundingRect().width() - margin,
        vp.height() - legendGroup->boundingRect().height() - margin
        );
    legendGroup->setPos(scenePos);

    // QSize vp = view->size();
    // qreal legendWidth = legendGroup->boundingRect().width(); // 获取图例的实际宽度
    // qreal legendHeight = legendGroup->boundingRect().height(); // 获取图例的实际高度

    // legendGroup->setPos(
    //     std::max(0.0, vp.width() - legendWidth * 2 ),
    //     std::max(0.0,  vp.height() + legendHeight * 1.5)
    //     );
}

// =============== DeviceMarkerItem ===============
DeviceMarkerItem::DeviceMarkerItem(const QString &name, double lat, double lon, QGraphicsScene *scene, RailMapViewerWidget *parent)
    : deviceName(name), latitude(lat), longitude(lon), parentWidget(parent) {
    devID = 0;
    setupUI();
    QPointF px = parentWidget->geoToPixel(lon, lat);
    setPos(px.x(), px.y());
    simulatedParams["设备ID"] = devID;
    simulatedParams["电量值"] = 100;
    simulatedParams["位置质量"] = 5;
    simulatedParams["卫星数量"] = 2;
}

DeviceMarkerItem::DeviceMarkerItem(const ShoeData &data, QGraphicsScene *scene, RailMapViewerWidget *parent)
    : shoeData(data), parentWidget(parent)
{
    devID = shoeData.wDevID;
    deviceName = QString("铁鞋 %1").arg(DeviceManager::instance()->getPaintedID(shoeData.wDevID));
    latitude = shoeData.lat;
    longitude = shoeData.lng;
    setupUI();
    QPointF px = parentWidget->geoToPixel(shoeData.lng, shoeData.lat);
    setPos(px.x(), px.y());
    simulatedParams["铁鞋序列号"] = shoeData.wDevID;
    simulatedParams["在线状态"] = EnumtoString(shoeData.byOnline);
    simulatedParams["电量值"] = shoeData.byBatVal;
    simulatedParams["位置质量"] = EnumtoString(shoeData.byPosQuality);
    simulatedParams["卫星数量"] = shoeData.byStarNum;
    setVisible(shoeData.byOnline != ShoeStatus::InCabinet);
}

void DeviceMarkerItem::setupUI()
{
    QColor fillColor, borderColor;
    if (shoeData.byOnline == ShoeStatus::Online) {
        fillColor = QColor("#00ffcc");
        borderColor = QColor("#00ccaa");
    } else if (shoeData.byOnline == ShoeStatus::Offline) {
        fillColor = QColor("#ffd166");
        borderColor = QColor("#ffaa33");
    } else {
        fillColor = QColor("#ff0000");
        borderColor = QColor("#ff4444");
    }

    QGraphicsEllipseItem *ellipse = new QGraphicsEllipseItem(-5, -5, 10, 10);
    ellipse->setPen(QPen(borderColor, 1.5));
    QRadialGradient grad(-5, -5, 15);
    grad.setColorAt(0, fillColor.lighter(130));
    grad.setColorAt(1, fillColor);
    ellipse->setBrush(grad);
    ellipse->setFlag(QGraphicsItem::ItemIsSelectable);
    ellipse->setFlag(QGraphicsItem::ItemIsFocusable);
    ellipse->setFlag(QGraphicsItem::ItemIgnoresTransformations);

    addToGroup(ellipse);
}

void DeviceMarkerItem::updateData(const ShoeData& data)
{
    latitude = data.lat;
    longitude = data.lng;
    shoeData = data;

    simulatedParams.clear();
    simulatedParams["设备ID"] = data.wDevID;
    simulatedParams["在线状态"] = EnumtoString(data.byOnline);
    simulatedParams["电量值"] = data.byBatVal;
    simulatedParams["位置质量"] = EnumtoString(data.byPosQuality);
    simulatedParams["卫星数量"] = data.byStarNum;

    if (data.byOnline == ShoeStatus::InCabinet){
        setVisible(false);
        return;
    }
    setVisible(true);

    QPointF px = parentWidget->geoToPixel(data.lng, data.lat);
    setPos(px.x(), px.y());

    // 更新颜色
    if (childItems().size() >= 1) {
        QGraphicsEllipseItem* ellipse = dynamic_cast<QGraphicsEllipseItem*>(childItems()[0]);
        if (ellipse) {
            QColor fillColor, borderColor;
            if (data.byOnline == ShoeStatus::Online) {
                fillColor = QColor("#00ffcc");
                borderColor = QColor("#00ccaa");
            } else if (data.byOnline == ShoeStatus::Offline) {
                fillColor = QColor("#ffd166");
                borderColor = QColor("#ffaa33");
            } else {
                fillColor = QColor("#ff0000");
                borderColor = QColor("#ff4444");
            }
            ellipse->setPen(QPen(borderColor, 1.5));
            QRadialGradient grad(-5, -5, 15);
            grad.setColorAt(0, fillColor.lighter(130));
            grad.setColorAt(1, fillColor);
            ellipse->setBrush(grad);
        }
    }
}

void DeviceMarkerItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if(isShowingDetails) return;
    QGraphicsItemGroup::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        isShowingDetails = true;
        QTimer::singleShot(0, [this]() {
            DeviceDetailsDialog dialog(deviceName, latitude, longitude, simulatedParams, nullptr);
            dialog.exec();
            isShowingDetails = false;
        });
    }
}

// =============== ShoeCabinetItem ===============
ShoeCabinetItem::ShoeCabinetItem(const QString &name, double lat, double lon, QGraphicsScene *scene, RailMapViewerWidget *parent)
    : deviceName(name), latitude(lat), longitude(lon), parentWidget(parent) {
    cabinetData.wDevID = 0;
    setupUI();
    QPointF px = parentWidget->geoToPixel(lon, lat);
    setPos(px.x(), px.y());
    simulatedParams["设备ID"] = cabinetData.wDevID;
    simulatedParams["仓位数"] = DeviceManager::instance()->getCabinetBindShoes(cabinetData.wDevID).size();
    // simulatedParams["仓位状态"] = "未知";
    simulatedParams["在线状态"] = "未知";
}

void ShoeCabinetItem::updateData(const CabinetData& data)
{
    cabinetData = data;
    simulatedParams.clear();
    simulatedParams["设备ID"] = data.wDevID;
    simulatedParams["仓位数"] = data.byStoreNum;
    simulatedParams["在线状态"] = EnumtoString(data.byOnline);
    // if (data.byStoreNum > 0 && data.abyStatus.size() >= data.byStoreNum) {
    //     for (int i = 0; i < data.byStoreNum; ++i) {
    //         StorageStatus value = static_cast<StorageStatus>(data.abyStatus.at(i));
    //         QString key = QString("仓位%1状态").arg(i+1);
    //         simulatedParams[key] = EnumtoString(value);
    //     }
    // }
    // 更新颜色（如果需要根据状态变化）
}

void ShoeCabinetItem::setupUI()
{
    QColor fillColor = QColor("#b967ff");
    QColor borderColor = QColor("#9955dd");

    QGraphicsRectItem *rect = new QGraphicsRectItem(-4, -4, 8, 8);
    rect->setPen(QPen(borderColor, 1.5));
    QRadialGradient grad(-4, -4, 12);
    grad.setColorAt(0, QColor("#ffffff"));
    grad.setColorAt(1, fillColor);
    rect->setBrush(grad);
    rect->setFlag(QGraphicsItem::ItemIsSelectable);
    rect->setFlag(QGraphicsItem::ItemIsFocusable);

    addToGroup(rect);
}

void ShoeCabinetItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if(isShowingDetails) return;
    QGraphicsItemGroup::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        isShowingDetails = true;
        QTimer::singleShot(0, [this]() {
            DeviceDetailsDialog dialog(deviceName, latitude, longitude, simulatedParams, nullptr);
            dialog.exec();
            isShowingDetails = false;
        });
    }
}

DeviceDetailsDialog::DeviceDetailsDialog(const QString &name, double lat, double lon,
                                         const QMap<QString, QVariant> &params, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("设备属性");
    setModal(true);
    setStyleSheet(R"(
        QDialog {
            background-color: #1a1a2e;
            color: #e0e0ff;
            border-radius: 10px;         /* 可选：保持圆角 */
            padding: 16px;               /* 让内容内边距统一 */
        }
    )");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(0);

    // 添加设备名称（突出显示）
    QLabel *titleLabel = new QLabel(name);
    titleLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #00ffcc; margin-bottom: 8px; margin: 0; padding: 0;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // 基础信息：经纬度
    addKeyValueRow(mainLayout, "经度", QString::number(lon, 'f', 6));
    addKeyValueRow(mainLayout, "纬度", QString::number(lat, 'f', 6));

    // 自定义参数
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        addKeyValueRow(mainLayout, it.key(), it.value().toString());
    }

    // === 按钮 ===
    QPushButton *okButton = new QPushButton("确定");
    okButton->setStyleSheet(R"(
        QPushButton {
            background-color: #4a6aff;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 8px 16px;
            font-size: 11pt;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #5a7aff;
        }
        QPushButton:pressed {
            background-color: #3a5aff;
        }
    )");
    connect(okButton, &QPushButton::clicked, this, &DeviceDetailsDialog::accept);
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
    resize(400, 300); // 设置合理默认大小
}

// 辅助函数：添加一行“键: 值”
void DeviceDetailsDialog::addKeyValueRow(QVBoxLayout *layout, const QString &key, const QString &value) {
    QHBoxLayout *row = new QHBoxLayout;
    row->setSpacing(8); // 适度间距（比 0 更美观），可调为 4 或 6
    row->setContentsMargins(0, 0, 0, 0);

    // 左对齐：键 + 冒号 + 值，全部左对齐
    QLabel *label = new QLabel(QString("<b>%1:</b> %2").arg(key).arg(value));
    label->setStyleSheet(R"(
        color: #e0e0ff;
        font-size: 10pt;
        margin: 0;
        padding: 0;
    )");
    label->setIndent(0);
    label->setWordWrap(false);
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    row->addWidget(label);
    layout->addLayout(row);
}
