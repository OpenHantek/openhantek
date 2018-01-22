// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <deque>
#include <list>
#include <memory>

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QPointer>
#include <QWindow>

#include <Qt3DCore/QAspectEngine>
#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>

#include <Qt3DInput/QInputAspect>
#include <Qt3DInput/QInputSettings>
#include <Qt3DInput/QMouseDevice>
#include <Qt3DInput/QMouseHandler>

#include <Qt3DLogic/QFrameAction>
#include <Qt3DLogic/QLogicAspect>

#include <Qt3DRender/QCamera>
#include <Qt3DRender/QCameraSelector>
#include <Qt3DRender/QClearBuffers>
#include <Qt3DRender/QFilterKey>
#include <Qt3DRender/QLayer>
#include <Qt3DRender/QLayerFilter>
#include <Qt3DRender/QRenderAspect>
#include <Qt3DRender/QRenderCapture>
#include <Qt3DRender/QRenderCaptureReply>
#include <Qt3DRender/QRenderSettings>
#include <Qt3DRender/QRenderSurfaceSelector>
#include <Qt3DRender/QRenderTarget>
#include <Qt3DRender/QRenderTargetOutput>
#include <Qt3DRender/QRenderTargetSelector>
#include <Qt3DRender/QTechniqueFilter>
#include <Qt3DRender/QTexture>
#include <Qt3DRender/QViewport>

#include "glmousedevice.h"
#include "glscopegraph.h"
#include "glscopegrid.h"
#include "glscopezoomviewport.h"
#include "hantekdso/enums.h"
#include "hantekprotocol/types.h"
#include "utils/scopecoordinates.h"

namespace Settings {
class View;
struct Colors;
struct MarkerAndZoom;
class ZoomViewSettings;
}
class PPresult;

/**
 * Because we do not want GlScope to inherit from a QObject (to make it inheritable by a QObject class like QWindow)
 * but still want to send signals, we use a delegate object.
 */
class GlScopeSignalEmitter : public QObject {
    Q_OBJECT
  signals:
    /**
     * @brief Request a fitting mouse cursor shape.
     * @param shape The cursor shape
     */
    void requestMouseCursor(Qt::CursorShape shape);
    /**
     * @brief Request to show a status text message
     * @param text The status text
     */
    void requestStatusText(const QString &text);
};

/**
 * The GlScope class is responsible for drawing the oscilloscope screen, markers
 * and zoom views and interacting with mouse events. It can also be used as an
 * offscreen renderer to export an image of the scope. You need to either call
 * initWithWindow() or initWithoutWindow() to finialize the initialisation.
 */
class GlScope {
    friend class MarkerAndZoom;

  public:
    /// \brief Initializes the scope window.
    /// It is required that you call one of the init*() methods as well to create the frame graph.
    ///
    /// \param markers The marker settings that should be used. Can be null.
    /// \param view The view settings that should be used (phosphor depth, etc)
    /// \param colors The colors that will be used
    /// \param renderSize The initial window/backbuffer size. You need to syncronize the window size to the
    ///  coords object for further render size adjustments.
    GlScope(Settings::ZoomViewSettings *markers, const Settings::View *view, const Settings::Colors *colors, QSize renderSize);
    virtual ~GlScope();
    GlScope(const GlScope &) = delete;

    /**
     * Call this method if you are using a QSurface backed object like a QWindow to draw the scope on.
     */
    void initWithWindow(QObject *eventSource);
    /**
     * Call this method if you want to draw to an offscreen buffer. Retrieve the image via the capture()
     * method.
     */
    void initWithoutWindow();

    /**
     * Show new post processed data
     * @param data
     */
    void showData(std::shared_ptr<PPresult> data);

    /**
     * Captures the rendered scene. Use QRenderCaptureReply::image
     * to retrieve the image. Usually the very first capture is invalid.
     */
    inline QPointer<Qt3DRender::QRenderCaptureReply> capture() {
        return QPointer<Qt3DRender::QRenderCaptureReply>(m_renderCapture->requestCapture());
    }

    /**
     * GlScope does not inherit from QObject, so can't emit signals. Use this signal delegate object
     * to connect to the signals instead.
     */
    inline GlScopeSignalEmitter *signalEmitter() { return &m_signalEmitter; }

    /**
     * @return Return the mouse device
     */
    inline GlMouseDevice *mouseDevice() { return m_mouseDevice.data(); }

    /**
     * @return Return the associated color palette
     */
    inline const Settings::Colors *colors() const { return m_colors; }

    /**
     * @return Return the zoom view settings
     */
    Settings::ZoomViewSettings *zoomViewSettings();

    /**
     * Zoom views can snap to the edges of the scope screen. A zoom view will call this
     * method as soon as it's snap property changed to update the geometry of the main scope view.
     */
    void updateZoomViewsSnap();

  protected:
    // User settings
    const Settings::View *m_view;
    const Settings::Colors *m_colors;
    Settings::ZoomViewSettings *m_markers;
    ScopeCoordinates coords; ///< Screen to scope coordinate system converter

    // Signal delegate
    GlScopeSignalEmitter m_signalEmitter;

    // Aspects
    QPointer<Qt3DCore::QAspectEngine> m_aspectEngine = QPointer<Qt3DCore::QAspectEngine>(new Qt3DCore::QAspectEngine);

    // Renderer configuration
    QPointer<Qt3DRender::QRenderSettings> m_renderSettings =
        QPointer<Qt3DRender::QRenderSettings>(new Qt3DRender::QRenderSettings);
    QPointer<Qt3DRender::QCamera> m_defaultCamera = QPointer<Qt3DRender::QCamera>(new Qt3DRender::QCamera);
    QPointer<Qt3DRender::QRenderCapture> m_renderCapture =
        QPointer<Qt3DRender::QRenderCapture>(new Qt3DRender::QRenderCapture);
    QPointer<Qt3DRender::QTechniqueFilter> m_techniqueFilter;
    QPointer<Qt3DRender::QClearBuffers> m_clearbuffers;

    // Viewports and layers
    Qt3DRender::QViewport *m_containerViewport = new Qt3DRender::QViewport;
    Qt3DRender::QViewport *m_mainViewport = new Qt3DRender::QViewport;
    Qt3DRender::QLayer *m_zoomViewLayer = new Qt3DRender::QLayer;

    // Input
    QPointer<GlMouseDevice> m_mouseDevice = nullptr;
    QPointer<Qt3DInput::QInputSettings> m_inputSettings = nullptr;

    // Scene graph
    QPointer<Qt3DCore::QEntity> m_scene =
        QPointer<Qt3DCore::QEntity>(new Qt3DCore::QEntity); ///< root scene graph object
    GlScopeGrid *m_grid;

    MarkerAndZoomMap m_markerEntities;
    std::list<GlScopeGraph *> m_GraphHistory;

    // Offscreen objects
    QPointer<QOpenGLContext> m_offscreenContext;
    QPointer<QOffscreenSurface> m_offscreenSurface;

    /// Initializes the frame graph objects, camera and scene graph. The frame graph itself
    /// is not tied together and no input init takes place.
    void init();
    /**
     * @brief A scope view can have an arbitrary number of embedded zoomed views.
     * A zoom view will be created for each marker rectangle. This method synchronizes
     * scope->markers with m_zoomEntities.
     */
    void updateMarkers(int activeMarker);
};

/// \brief Qt3D accelerated window that displays the oscilloscope screen.
/// Hint for Qt4 developers: Use a QWindow in a legacy QWidget application,
/// by the static wrapper funtion: QWidget::createWindowContainer(window).
class GlScopeWindow : public QWindow, public GlScope {
    Q_OBJECT
  public:
    GlScopeWindow(Settings::ZoomViewSettings *markers, const Settings::View *view, const Settings::Colors *colors);

  protected:
    void resizeEvent(QResizeEvent *) override;
};
