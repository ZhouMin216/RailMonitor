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

// 全局围栏脉冲定时器（避免重复创建）
static QTimer* g_fencePulseTimer = nullptr;

RailMapViewerWidget::RailMapViewerWidget(QWidget *parent)
    : QWidget(parent) {
    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene);
    view->setRenderHint(QPainter::Antialiasing, true);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setStyleSheet("background: #000010; border: none;"); // 深空黑背景

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
    drawAll();
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
    scene->clear();
    drawTracks();
    drawBuildings();
    drawFence();
    drawShoeCabinet();
}

void RailMapViewerWidget::drawTracks() {
    QPen masterPen(QColor("#00ffff"), 1.8);
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

            QGraphicsDropShadowEffect* glow = new QGraphicsDropShadowEffect();
            glow->setColor(QColor(0, 255, 255, 60));
            glow->setBlurRadius(4);
            glow->setOffset(0, 0);
            line->setGraphicsEffect(glow);
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

    // 初始化全局脉冲定时器（只创建一次）
    if (!g_fencePulseTimer) {
        g_fencePulseTimer = new QTimer(qApp);
        connect(g_fencePulseTimer, &QTimer::timeout, []() {
            static qreal offset = 0;
            offset += 2;
            // 注意：此处无法直接访问 fenceItem，需在 RailMapViewerWidget 中处理
            // 因此我们在 finishFenceDrawing 中启动局部动画
        });
        // 实际动画在 finishFenceDrawing 中启动（见下文）
    }
}

void RailMapViewerWidget::drawShoeCabinet(){
    shoeCabinet.clear();
    for (auto it = shoeCabinetPoints.constBegin(); it != shoeCabinetPoints.constEnd(); ++it) {
        QString name = QString("鞋柜%1").arg(it.key());
        QPointF pt = it.value();
        ShoeCabinetItem *marker = new ShoeCabinetItem(name, pt.x(), pt.y(), scene, this);
        shoeCabinet[it.key()] = marker;
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

    // 启动脉冲动画（局部，避免全局依赖）
    if (fenceItem) {
        QTimer* pulseTimer = new QTimer(this);
        pulseTimer->setSingleShot(false);
        connect(pulseTimer, &QTimer::timeout, [this]() {
            if (!fenceItem) return;
            QPen pen = fenceItem->pen();
            static qreal offset = 0;
            offset += 2;
            pen.setDashOffset(offset);
            fenceItem->setPen(pen);
        });
        pulseTimer->start(100);
    }

    coordLabel->setText("电子围栏已设置");
}

void RailMapViewerWidget::clearFence() {
    emit clearGeoFence();
    savedFencePoints.clear();
    currentFencePoints.clear();
    isDrawingFence = false;
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
    deviceName = QString("铁鞋%1").arg(shoeData.wDevID);
    latitude = shoeData.lat;
    longitude = shoeData.lng;
    setupUI();
    QPointF px = parentWidget->geoToPixel(shoeData.lng, shoeData.lat);
    setPos(px.x(), px.y());
    simulatedParams["设备ID"] = shoeData.wDevID;
    simulatedParams["在线状态"] = EnumtoString(shoeData.byOnline);
    simulatedParams["电量值"] = shoeData.byBatVal;
    simulatedParams["位置质量"] = EnumtoString(shoeData.byPosQuality);
    simulatedParams["卫星数量"] = shoeData.byStarNum;
    setVisible(shoeData.byOnline != DeviceStatus::InCabinet);
}

void DeviceMarkerItem::setupUI()
{
    QColor fillColor, borderColor;
    if (shoeData.byOnline == DeviceStatus::Online) {
        fillColor = QColor("#00ffcc");
        borderColor = QColor("#00ccaa");
    } else if (shoeData.byOnline == DeviceStatus::Offline) {
        fillColor = QColor("#aa5555");
        borderColor = QColor("#884444");
    } else {
        fillColor = QColor("#555555");
        borderColor = QColor("#444444");
    }

    QGraphicsEllipseItem *ellipse = new QGraphicsEllipseItem(-5, -5, 10, 10);
    ellipse->setPen(QPen(borderColor, 1.5));
    QRadialGradient grad(-5, -5, 15);
    grad.setColorAt(0, fillColor.lighter(130));
    grad.setColorAt(1, fillColor);
    ellipse->setBrush(grad);
    ellipse->setFlag(QGraphicsItem::ItemIsSelectable);
    ellipse->setFlag(QGraphicsItem::ItemIsFocusable);

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

    setVisible(data.byOnline != DeviceStatus::InCabinet);

    QPointF px = parentWidget->geoToPixel(data.lng, data.lat);
    setPos(px.x(), px.y());

    // 更新颜色
    if (childItems().size() >= 1) {
        QGraphicsEllipseItem* ellipse = dynamic_cast<QGraphicsEllipseItem*>(childItems()[0]);
        if (ellipse) {
            QColor fillColor, borderColor;
            if (data.byOnline == DeviceStatus::Online) {
                fillColor = QColor("#00ffcc");
                borderColor = QColor("#00ccaa");
            } else if (data.byOnline == DeviceStatus::Offline) {
                fillColor = QColor("#aa5555");
                borderColor = QColor("#884444");
            } else {
                fillColor = QColor("#555555");
                borderColor = QColor("#444444");
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
    simulatedParams["仓位数"] = 6;
    simulatedParams["仓位状态"] = "未知";
    simulatedParams["在线状态"] = "未知";
}

void ShoeCabinetItem::updateData(const CabinetData& data)
{
    cabinetData = data;
    simulatedParams.clear();
    simulatedParams["设备ID"] = data.wDevID;
    simulatedParams["仓位数"] = data.byStoreNum;
    simulatedParams["在线状态"] = EnumtoString(data.byOnline);
    if (data.byStoreNum > 0 && data.abyStatus.size() >= data.byStoreNum) {
        for (int i = 0; i < data.byStoreNum; ++i) {
            StorageStatus value = static_cast<StorageStatus>(data.abyStatus.at(i));
            QString key = QString("仓位%1状态").arg(i+1);
            simulatedParams[key] = EnumtoString(value);
        }
    }
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

// =============== DeviceDetailsDialog ===============
DeviceDetailsDialog::DeviceDetailsDialog(const QString &name, double lat, double lon,
                                         const QMap<QString, QVariant> &params, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("设备属性");
    setModal(true);
    setStyleSheet("background-color: #0f0f1f; color: #00ffcc;");

    QVBoxLayout *layout = new QVBoxLayout(this);

    QString info = QString("<b>设备名称:</b> %1<br>").arg(name);
    info += QString("<b>经度:</b> %1<br>").arg(lon, 0, 'f', 6);
    info += QString("<b>纬度:</b> %1<br>").arg(lat, 0, 'f', 6);

    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        info += QString("<b>%1:</b> %2<br>").arg(it.key()).arg(it.value().toString());
    }

    QLabel *infoLabel = new QLabel(info);
    infoLabel->setTextFormat(Qt::RichText);
    infoLabel->setStyleSheet("background-color: #1a1a2e; padding: 10px; border-radius: 5px;");
    layout->addWidget(infoLabel);

    QPushButton *okButton = new QPushButton("确定");
    okButton->setStyleSheet("background-color: #4a6aff; color: white; padding: 5px;");
    connect(okButton, &QPushButton::clicked, this, &DeviceDetailsDialog::accept);
    layout->addWidget(okButton);

    setLayout(layout);
}
