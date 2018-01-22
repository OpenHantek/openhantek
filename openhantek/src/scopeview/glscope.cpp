// SPDX-License-Identifier: GPL-2.0+

#include "glscope.h"

#include "post/graphgenerator.h"
#include "post/ppresult.h"
#include "scopesettings.h"
#include "viewconstants.h"
#include "viewsettings.h"

#include <cmath>
#include <iostream>

#include <QColor>
#include <QCoreApplication>
#include <QDebug>

#include <QFirstPersonCameraController>
#include <QSharedPointer>

using namespace Qt3DRender;
using namespace Qt3DCore;

GlScope::GlScope(Settings::ZoomViewSettings *markers, const Settings::View *view, const Settings::Colors *colors,
                 QSize renderSize)
    : m_view(view), m_colors(colors), m_markers(markers), coords(renderSize),
      m_grid(new GlScopeGrid(colors, m_zoomViewLayer, m_scene.data())) {
    if (markers)
        QObject::connect(markers, &Settings::ZoomViewSettings::markerChanged, m_grid,
                         [this](int activeMarker) { updateMarkers(activeMarker); });
}

void GlScope::init() {

    ///// Setup frame graph
    Qt3DLogic::QLogicAspect *m_logicAspect = new Qt3DLogic::QLogicAspect;
    m_logicAspect->setObjectName("logicAspect");
    m_aspectEngine->registerAspect(m_logicAspect);

    QRenderAspect *m_renderAspect = new QRenderAspect;
    m_renderAspect->setObjectName("renderAspect");
    m_aspectEngine->registerAspect(m_renderAspect);

    // The pipeline chain will destroy itself, no delete required or allowed

    m_techniqueFilter = QPointer<QTechniqueFilter>(new QTechniqueFilter);

    QFilterKey *forwardRenderingStyle = new QFilterKey;
    forwardRenderingStyle->setName(QStringLiteral("renderingStyle"));
    forwardRenderingStyle->setValue(QStringLiteral("forward"));
    m_techniqueFilter->addMatch(forwardRenderingStyle);

    m_containerViewport->setNormalizedRect(QRectF(0, 0, 1, 1));
    m_mainViewport->setNormalizedRect(QRectF(0, 0, 1, 1));

    m_clearbuffers = QPointer<QClearBuffers>(new QClearBuffers);
    m_clearbuffers->setClearColor(m_colors->background());
    m_clearbuffers->setBuffers(QClearBuffers::ColorDepthBuffer);
    m_clearbuffers->setEnabled(true);

    QMatrix4x4 pmvMatrix;
    pmvMatrix.ortho(-(DIVS_TIME / 2.0f), (DIVS_TIME / 2.0f), -(DIVS_VOLTAGE / 2.0f), (DIVS_VOLTAGE / 2.0f), -1.0f,
                    1.0f);

    m_defaultCamera->setProjectionType(QCameraLens::PerspectiveProjection);
    m_defaultCamera->setProjectionMatrix(pmvMatrix);
    m_defaultCamera->setFieldOfView(500);
    m_defaultCamera->setNearPlane(-1000);
    m_defaultCamera->setFarPlane(1000);
    m_defaultCamera->setUpVector(QVector3D(0, 1.0, 0));
    // m_defaultCamera->setViewCenter(QVector3D(0.0f, 3.5f, 0.0f));
    // m_defaultCamera->setPosition(QVector3D(0.0f, 3.5f, 25.0f));
    m_defaultCamera->lens()->setProjectionMatrix(pmvMatrix);
    m_defaultCamera->setAspectRatio(float(coords.screenSize().width()) / float(coords.screenSize().height()));

    QCameraSelector *cameraSelector = new QCameraSelector;
    cameraSelector->setCamera(m_defaultCamera.data());

    m_renderSettings->setActiveFrameGraph(m_techniqueFilter.data());

    // Qt3d will create a render-pass for each leaf of the framegraph

    // First branch: Main view
    m_clearbuffers->setParent(m_containerViewport);
    m_mainViewport->setParent(m_clearbuffers.data());
    cameraSelector->setParent(m_mainViewport);
    m_renderCapture->setParent(cameraSelector);

    // Every zoomed view is another branch, that adds to the container view

    ///// Setup scene graph

    m_scene->addComponent(m_renderSettings.data());
    m_scene->addComponent(m_zoomViewLayer);
    m_grid->addComponent(m_zoomViewLayer);
    m_grid->setParent(m_scene.data());

    // Last init steps
    m_aspectEngine->setRootEntity(QEntityPtr(m_scene.data()));
}

void GlScope::initWithWindow(QObject *eventSource) {
    Qt3DInput::QInputAspect *m_inputAspect = new Qt3DInput::QInputAspect;
    m_inputAspect->setObjectName("inputAspect");
    m_aspectEngine->registerAspect(m_inputAspect);

    m_mouseDevice = QPointer<GlMouseDevice>(new GlMouseDevice);
    m_inputSettings = QPointer<Qt3DInput::QInputSettings>(new Qt3DInput::QInputSettings);
    m_inputSettings->setEventSource(eventSource);
    m_scene->addComponent(m_inputSettings.data());

    QRenderSurfaceSelector *renderSurfaceSelector = new QRenderSurfaceSelector;
    renderSurfaceSelector->setSurface(eventSource);

    init();

    // First create the common path (TechniqueFilter+MouseDevice+RenderSurface+FrustumCulling+Viewport)
    m_mouseDevice->setParent(m_techniqueFilter.data());
    renderSurfaceSelector->setParent(m_mouseDevice.data());
    m_containerViewport->setParent(renderSurfaceSelector);

    QObject::connect(m_colors->observer(), &Observer::changed, &m_signalEmitter,
                     [this]() { m_clearbuffers->setClearColor(m_colors->background()); });
    QObject::connect(&coords, &ScopeCoordinates::rectChanged, &m_signalEmitter, [this]() {
        m_defaultCamera->setAspectRatio(float(coords.screenSize().width()) / float(coords.screenSize().height()));
    });
    if (m_markers) updateMarkers(m_markers->activeMarker());
}

void GlScope::initWithoutWindow() {

    QRenderSurfaceSelector *renderSurfaceSelector = new QRenderSurfaceSelector;
    QRenderTargetSelector *renderTargetSelector = new QRenderTargetSelector;
    auto renderTarget = new QRenderTarget(renderTargetSelector);
    renderTargetSelector->setTarget(renderTarget);

    auto texture = new QTexture2D(renderTarget);
    texture->setSize(coords.screenSize().width(), coords.screenSize().height());
    texture->setFormat(QAbstractTexture::RGBA8_UNorm);
    texture->setMinificationFilter(QAbstractTexture::Linear);
    texture->setMagnificationFilter(QAbstractTexture::Linear);

    auto renderTargetOutput = new QRenderTargetOutput(renderTarget);
    renderTargetOutput->setAttachmentPoint(QRenderTargetOutput::Color0);
    renderTargetOutput->setTexture(texture);
    renderTarget->addOutput(renderTargetOutput);

    auto textureDep = new QTexture2D(renderTarget);
    textureDep->setSize(coords.screenSize().width(), coords.screenSize().height());
    textureDep->setFormat(QAbstractTexture::D24);
    textureDep->setMinificationFilter(QAbstractTexture::Linear);
    textureDep->setMagnificationFilter(QAbstractTexture::Linear);
    textureDep->setComparisonFunction(QAbstractTexture::CompareLessEqual);
    textureDep->setComparisonMode(QAbstractTexture::CompareRefToTexture);

    auto renderTargetOutputDep = new QRenderTargetOutput(renderTarget);
    renderTargetOutputDep->setAttachmentPoint(QRenderTargetOutput::Depth);
    renderTargetOutputDep->setTexture(textureDep);
    renderTarget->addOutput(renderTargetOutputDep);

    m_offscreenContext = QPointer<QOpenGLContext>(new QOpenGLContext);
    m_offscreenContext->setFormat(QSurfaceFormat::defaultFormat());
    m_offscreenContext->create();

    m_offscreenSurface = QPointer<QOffscreenSurface>(new QOffscreenSurface);
    m_offscreenSurface->setFormat(QSurfaceFormat::defaultFormat());
    m_offscreenSurface->create();
    renderSurfaceSelector->setSurface(m_offscreenSurface.data());
    renderSurfaceSelector->setExternalRenderTargetSize(coords.screenSize());

    m_offscreenContext->makeCurrent(m_offscreenSurface.data());

    init();

    // First create the common path (TechniqueFilter+RenderSurface+FrustumCulling+Viewport)
    renderTargetSelector->setParent(m_techniqueFilter.data());
    renderSurfaceSelector->setParent(renderTargetSelector);
    m_containerViewport->setParent(renderSurfaceSelector);
    if (m_markers) updateMarkers(m_markers->activeMarker());
}

GlScope::~GlScope() {
    // Clean up offscreen buffers
    if (m_offscreenContext) {
        m_offscreenContext->doneCurrent();
        delete m_offscreenContext;
        delete m_offscreenSurface;
    }
    // Detach scene graph from frame graph
    m_aspectEngine->setRootEntity(nullptr);

    // Clean up scene graph. All children including graphs, the grid, markers, zoom views will be destroyed
    delete m_scene;

    // Clean up frame graph (camera, viewports, clearbuffer)
    // delete m_zoomViewLayer;
    // delete m_mainViewport;
    // delete m_containerViewport;

    delete m_renderCapture;
    delete m_defaultCamera;

    delete m_clearbuffers;
    delete m_mouseDevice;
    delete m_techniqueFilter;
    delete m_renderSettings;

    delete m_inputSettings;
    delete m_aspectEngine;
}

void GlScope::updateMarkers(int activeMarker) {
    // Delete entity entries of not existing markers
    auto markerEntityIterator = m_markerEntities.begin();
    while (markerEntityIterator != m_markerEntities.end()) {
        if (!m_markers->contains(markerEntityIterator->first)) {
            markerEntityIterator->second->destroy();
            markerEntityIterator = m_markerEntities.erase(markerEntityIterator);
        } else {
            ++markerEntityIterator;
        }
    }

    // Create new entities
    for (auto markerIterator = m_markers->begin(); markerIterator != m_markers->end(); ++markerIterator) {
        markerEntityIterator = m_markerEntities.find(markerIterator->first);
        if (markerEntityIterator != m_markerEntities.end()) continue;

        // If not marker+zoomview with matching unique id found: Create new entry. The entry
        // contains the Qt3d entities for the marker frame and for the zoom view viewport.
        const unsigned markerID = markerIterator->first;
        MarkerAndZoom *localMarkerAndZoom = new MarkerAndZoom(markerID, &markerIterator->second, this);
        QObject::connect(localMarkerAndZoom, &MarkerAndZoom::requestActive, m_markers,
                         &Settings::ZoomViewSettings::setActiveMarker);
        QObject::connect(localMarkerAndZoom, &MarkerAndZoom::requestRemove, m_markers,
                         &Settings::ZoomViewSettings::removeMarker);
        QObject::connect(localMarkerAndZoom, &MarkerAndZoom::userChangedGeometry, m_markers, [this, markerID]() {
            if ((int)markerID == m_markers->activeMarker()) m_markers->notifyDataChanged();
        });
        QObject::connect(m_markers, &Settings::ZoomViewSettings::activeMarkerChanged, localMarkerAndZoom,
                         &MarkerAndZoom::updateActive);
        m_markerEntities.put(markerID, localMarkerAndZoom);
    }

    /// Used to assign zoomviews to a position, different to other default-placed zoomviews
    unsigned countZoomViews = 0;
    /// For every zoom view / marker: Synchronize position, geometry, shown region etc.
    for (MarkerAndZoomMap::value_type &entry : m_markerEntities) {
        entry.second->update(countZoomViews++, activeMarker);
    }
    updateZoomViewsSnap();
}

void GlScope::showData(std::shared_ptr<PPresult> data) {
    // Remove too much entries
    const unsigned history = m_view->digitalPhosphorDraws();
    while (history < m_GraphHistory.size()) {
        delete m_GraphHistory.back();
        m_GraphHistory.pop_back();
    }

    // Add if missing
    if (history > m_GraphHistory.size()) {
        m_GraphHistory.push_front(new GlScopeGraph(m_scene, m_colors, m_view, m_zoomViewLayer));
    }

    // Move last entry to front
    if (history > 1) {
        m_GraphHistory.push_front(m_GraphHistory.back());
        m_GraphHistory.pop_back();
    }

    // Apply new data to first graph
    m_GraphHistory.front()->writeData(data.get());

    int index = 0;
    for (GlScopeGraph *g : m_GraphHistory) g->setColorAlpha(1.0f - (index++) * 0.2f);
}

Settings::ZoomViewSettings *GlScope::zoomViewSettings() { return m_markers; }

void GlScope::updateZoomViewsSnap() {
    double shrinkLeft = 0;
    double shrinkTop = 0;
    double shrinkRight = 0;
    double shrinkBottom = 0;

    QRectF viewPortRect = coords.fixedScopeRect();
    QRectF mouseAdjustRect = viewPortRect;

    for (MarkerAndZoomMap::value_type &entry : m_markerEntities) {
        const QRectF &rect = entry.second->zoomviewPosition();
        double h = rect.height();
        double w = rect.width();
        EdgePositionFlags snaps = entry.second->snapState();
        if (snaps / EdgePositionFlags::Left) {
            if (shrinkLeft < w) shrinkLeft = w;
        } else if (snaps / EdgePositionFlags::Right) {
            if (shrinkRight < w) shrinkRight = w;
        } else if (snaps / EdgePositionFlags::Top) {
            if (shrinkTop < h) shrinkTop = h;
        } else if (snaps / EdgePositionFlags::Bottom) {
            if (shrinkBottom < h) shrinkBottom = h;
        }
    }

    // Marker zoom views may be snapped to one or more edges. Make the main viewport smaller now.
    viewPortRect.setLeft(viewPortRect.left() + shrinkLeft);
    viewPortRect.setRight(viewPortRect.right() - shrinkRight);
    viewPortRect.setTop(viewPortRect.top() + shrinkTop);
    viewPortRect.setBottom(viewPortRect.bottom() - shrinkBottom);

    m_mainViewport->setNormalizedRect(ScopeCoordinates::computeNormalizedRect(viewPortRect, coords.fixedScopeRect()));

    // The viewport changed its size, therefore we need to adjust the mainview coordinate system
    // by virtually extending its size, so that the mouse positions within the actual smaller area
    // will still be translated to correct coordinates for mouse hover/click events.
    double xR = mouseAdjustRect.width() / viewPortRect.width();
    double yR = mouseAdjustRect.height() / viewPortRect.height();

    shrinkLeft *= xR;
    shrinkRight *= xR;
    shrinkTop *= yR;
    shrinkBottom *= yR;

    // Compute
    mouseAdjustRect.setLeft(mouseAdjustRect.left() - shrinkLeft);
    mouseAdjustRect.setWidth(mouseAdjustRect.width() + shrinkRight);
    mouseAdjustRect.setTop(mouseAdjustRect.top() - shrinkTop);
    mouseAdjustRect.setBottom(mouseAdjustRect.bottom() + shrinkBottom);
    coords.setScopeRect(mouseAdjustRect);
}

GlScopeWindow::GlScopeWindow(Settings::ZoomViewSettings *markers, const Settings::View *view,
                             const Settings::Colors *colors)
    : GlScope(markers, view, colors, size()) {
    setSurfaceType(QSurface::OpenGLSurface);
    initWithWindow(this);

    connect(signalEmitter(), &GlScopeSignalEmitter::requestMouseCursor, this,
            [this](Qt::CursorShape shape) { setCursor(QCursor(shape)); });

    //    Qt3DExtras::QFirstPersonCameraController *p = new Qt3DExtras::QFirstPersonCameraController(m_scene);
    //    p->setCamera(m_defaultCamera);
    //    p->setParent(m_scene);
}

void GlScopeWindow::resizeEvent(QResizeEvent *) { coords.updateScreenSize(size()); }
