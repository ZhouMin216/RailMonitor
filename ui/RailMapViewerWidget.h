#pragma once
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QLineEdit>
#include <QLabel>
#include <QVariantMap>
#include <QPointF>
#include <QFile>
#include <QJsonDocument>
#include <QMessageBox>
#include <QGraphicsItemGroup>

#include "protocol/DeviceParser.h"

struct RailTrack {
    int number;
    QString name;
    QList<QPointF> masterPoints;
    QList<QPointF> slavePoints;
};

struct Building {
    QString name;
    QList<QPointF> points;
};

class ShoeCabinetItem;
class DeviceMarkerItem;
class RailMapViewerWidget : public QWidget {
    Q_OBJECT
public:
    explicit RailMapViewerWidget(QWidget *parent = nullptr);
    void addDeviceMarker(const QString &name, double lat, double lon);
    QPointF geoToPixel(double lng, double lat);
    void loadGeoFence();

public slots:
    // 接收外部数据
    void handleIncomingFencePoint(const QList<QPointF>& data);
    void updateCabinets(const QList<CabinetData>& data);
    void updateShoes(const QList<ShoeData>& data);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void add_user_marker();
    void finishFenceDrawing();
    void clearFence();


signals:
    void clearGeoFence();
    void getGeoFence();
    void saveGeoFence(const QList<QPointF>& data);

private:
    void loadConfig();
    void drawAll();
    void drawTracks();
    void drawBuildings();
    void drawViaPoints();
    void drawFence();
    void drawShoeCabinet();
    void updateFencePreview();

    QPointF pixelToGeo(qreal x, qreal y);
    bool pointInPolygon(const QPointF& point, const QList<QPointF>& polygon);

    QGraphicsScene *scene;
    QGraphicsView *view;
    QLineEdit *lngInput, *latInput, *rotateInput;
    QLabel *coordLabel;

    QMap<QString, QPointF> viaPoints;
    QMap<QString, QPointF> buildPoints;
    QMap<quint16, QPointF> shoeCabinetPoints;
    QList<QVariantMap> tracks;
    QList<QVariantMap> buildings;
    QMap<quint16,ShoeCabinetItem*> shoeCabinet;
    QMap<quint16, DeviceMarkerItem*> shoeMap;

    QList<RailTrack> railTracks_;
    QList<Building> buildings_;

    double minLat = 0, maxLat = 0, minLng = 0, maxLng = 0;
    double latSpan = 1e-8, lngSpan = 1e-8;
    int h_pixel = 1200;
    int v_pixel = 700;
    int marign = 50;

private:
    QList<QPointF> currentFencePoints;      // 正在绘制的围栏点（经纬度）
    QList<QPointF> savedFencePoints;        // 已保存的围栏（从DB加载）
    QGraphicsPolygonItem* fenceItem = nullptr;
    bool isDrawingFence = false;

    QPushButton* startFenceBtn;
    QPushButton* finishFenceBtn;
    QPushButton* clearFenceBtn;
};

class DeviceMarkerItem : public QGraphicsItemGroup {
public:
    explicit DeviceMarkerItem(const QString &name, double lat, double lon, QGraphicsScene *scene, RailMapViewerWidget *parent);
    explicit DeviceMarkerItem(const ShoeData &data, QGraphicsScene *scene, RailMapViewerWidget *parent);

    // 重写 mousePressEvent 以响应点击
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

    void updateData(const ShoeData& data);
    quint16 getDevID(){ return devID; }
    DeviceStatus getPrevStatus() { return prevStatus_;}
    bool isInPolygon() { return inPolygon_;}
    void setInPolygon(bool in) { inPolygon_ = in; }

private:
    void setupUI();

private:
    quint16 devID;
    QString deviceName;
    double latitude;
    double longitude;
    RailMapViewerWidget *parentWidget; // 用于调用父窗口的槽函数
    ShoeData shoeData;
    DeviceStatus prevStatus_{DeviceStatus::Unregister};
    bool inPolygon_{true};

    QMap<QString, QVariant> simulatedParams;
private:
    bool isShowingDetails = false;
};

class ShoeCabinetItem : public QGraphicsItemGroup {
public:
    explicit ShoeCabinetItem(const QString &name, double lat, double lon, QGraphicsScene *scene, RailMapViewerWidget *parent);

    // 重写 mousePressEvent 以响应点击
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

    void updateData(const CabinetData& data);
    const quint16& getDevID(){return cabinetData.wDevID; }

private:
    void setupUI();

private:
    QString deviceName;
    double latitude;
    double longitude;
    RailMapViewerWidget *parentWidget; // 用于调用父窗口的槽函数
    CabinetData cabinetData;

    // 模拟参数 (可以扩展)
    QMap<QString, QVariant> simulatedParams;
private:
    bool isShowingDetails = false;
};

class DeviceDetailsDialog : public QDialog {
    Q_OBJECT

public:
    explicit DeviceDetailsDialog(const QString &name, double lat, double lon,
                                 const QMap<QString, QVariant> &params, QWidget *parent = nullptr);

private slots:
    void onOkClicked();

private:
    QLabel *infoLabel;
};
