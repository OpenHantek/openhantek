#include "glscopezoomviewport.h"
#include "glscope.h"
#include "iconfont/QtAwesome.h"
#include "settings/colorsettings.h"
#include "settings/scopesettings.h"
#include "settings/viewsettings.h"

#include <QCoreApplication>
#include <Qt3DCore/QTransform>
#include <Qt3DRender/QLayerFilter>

using namespace Qt3DRender;
using namespace Qt3DExtras;
using namespace Qt3DCore;

MarkerAndZoom::MarkerAndZoom(unsigned markerID, Settings::MarkerAndZoom *markerSetting, GlScope *scope)
    : m_markerSetting(markerSetting), m_markerID(markerID), m_scope(scope),
      mouseHandler(&m_zoomviewPosition, &m_markerID, scope->mouseDevice(), new ScopeCoordinates(scope->coords),
                   scope->m_scene.data()) {
    ///// Setup camera with matching settings to the mainview /////
    zoomCamera->setFieldOfView(1000);
    zoomCamera->setNearPlane(0.1f);
    zoomCamera->setFarPlane(1000);
    zoomCamera->setViewCenter(QVector3D(0, 0, 0));
    zoomCamera->setUpVector(QVector3D(0, 1.0, 0));

    ///// Setup viewport with camera and layerfilter (to not show the marker frame) /////

    QLayerFilter *layerFilter = new QLayerFilter(viewport);
    layerFilter->addLayer(scope->m_zoomViewLayer);

    Qt3DRender::QCameraSelector *cameraSelector = new Qt3DRender::QCameraSelector(viewport);
    cameraSelector->setCamera(zoomCamera);

    cameraSelector->setParent(layerFilter);
    layerFilter->setParent(viewport);
    viewport->setParent(scope->m_containerViewport);

    const Settings::Colors *colors = scope->colors();
    marker = new GlFrame(scope->mouseDevice(), &scope->coords, &colors->markers(), &colors->markerHover(),
                         &colors->markerSelected(), &colors->markerActive(), colors->observer(), nullptr,
                         scope->m_zoomViewLayer, scope->m_scene.data());
    marker->setFrameIndex(markerID);

    m_removeBtn = new GlIcon(scope->mouseDevice(), &scope->coords, &colors->zoomBackground(), &colors->zoomHover(),
                             &colors->zoomSelected(), colors->observer(), iconFont->icon(fa::trash), nullptr,
                             scope->m_scene.data());

    GlMoveResizeSnapSignals *zvSignals = mouseHandler.moveResizeSignals();

    connect(zvSignals, &GlMoveResizeSnapSignals::frameChanged, viewport, [this, scope](unsigned, const QRectF &rect) {
        m_markerSetting->zoomRect = rect;
        moveZoomview(rect);
    });

    connect(zvSignals, &GlMoveResizeSnapSignals::requestMouseCursor, &scope->m_signalEmitter,
            &GlScopeSignalEmitter::requestMouseCursor);

    connect(zvSignals, &GlMoveResizeSnapSignals::snapChanged, this, [this]() { m_scope->updateZoomViewsSnap(); });

    connect(marker->moveResizeSignals(), &GlMoveResizeSnapSignals::frameChanged, marker,
            [scope, this](unsigned, const QRectF &rect) {
                m_markerSetting->markerRect = rect;
                m_removeBtn->updatePosition(QPointF(rect.x(), rect.y()));
                updateZoomRegion(rect);
                emit userChangedGeometry();
            });

    connect(marker->moveResizeSignals(), &GlMoveResizeSnapSignals::requestMouseCursor, scope->signalEmitter(),
            &GlScopeSignalEmitter::requestMouseCursor);

    connect(marker, &GlFrame::activated, this, &MarkerAndZoom::requestActive);

    connect(m_removeBtn->moveResizeSignals(), &GlMoveResizeSnapSignals::hovered, m_removeBtn,
            [scope]() { emit scope->signalEmitter()->requestStatusText(QCoreApplication::tr("Remove zoom view")); });

    connect(m_removeBtn->moveResizeSignals(), &GlMoveResizeSnapSignals::clicked, this,
            [this]() { emit requestRemove(m_markerID); });
}

MarkerAndZoom::~MarkerAndZoom() { delete mouseHandler.coordinateSystem(); }

void MarkerAndZoom::destroy() {
    delete marker;
    delete m_removeBtn;
    delete viewport;
    viewport = nullptr;
    m_removeBtn = nullptr;
    marker = nullptr;
}

void MarkerAndZoom::updateZoomRegion(const QRectF &marker) {
    QMatrix4x4 zoomViewMatrix;
    zoomViewMatrix.ortho(marker.left(), marker.right(), marker.top(), marker.bottom(), -1.0f, +1.0f);
    zoomCamera->setProjectionMatrix(zoomViewMatrix);
}

bool MarkerAndZoom::moveZoomview(const QRectF &position) {
    const QRectF m = mouseHandler.coordinateSystem()->scopeRect();

    if (position.height() > m.height() || position.width() > m.width()) return false;
    if (position.top() < m.top() || position.bottom() > m.bottom()) return false;
    if (position.right() > m.right() || position.left() < m.left()) return false;
    if (position.height() < 0.01 || position.width() < 0.01) return false;

    m_zoomviewPosition = position;
    viewport->setNormalizedRect(ScopeCoordinates::computeNormalizedRect(m_zoomviewPosition, m));
    mouseHandler.updateSnap();
    return true;
}

void MarkerAndZoom::update(unsigned positionIndexIfNoSavedPos, int activeMarker) {
    // A new zoom view should default to 1/4 width of the scope screen and 1/5 height
    // and be placed in the right bottom corner.
    const float w = DIVS_TIME / 4;
    const float h = DIVS_VOLTAGE / 5;

    // Move the viewport and update the shown region
    if (m_markerSetting->zoomRect.isNull() || !moveZoomview(m_markerSetting->zoomRect))
        moveZoomview(QRectF(DIVS_TIME / 2 - w, -DIVS_VOLTAGE / 2 + h * positionIndexIfNoSavedPos, w, h));
    updateZoomRegion(m_markerSetting->markerRect);
    marker->updateRectangle(m_markerSetting->markerRect);
    m_removeBtn->updatePosition(QPointF(marker->rect()->x(), marker->rect()->y()));
    updateActive(activeMarker);
}

void MarkerAndZoom::updateActive(int activeMarker) { marker->setActive(activeMarker == (int)m_markerID); }
