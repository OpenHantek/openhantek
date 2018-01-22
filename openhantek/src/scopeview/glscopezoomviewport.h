#pragma once

#include <QSharedPointer>

#include <Qt3DRender/QCamera>
#include <Qt3DRender/QCameraSelector>
#include <Qt3DRender/QViewport>

#include "glframe.h"
#include "glicon.h"
#include "glmoveresizesnap.h"
#include "utils/scopecoordinates.h"

namespace Settings {
class View;
struct Colors;
struct MarkerAndZoom;
class ZoomViewSettings;
}
class GlMouseDevice;
class GlScope;

class MarkerAndZoom : public QObject {
    Q_OBJECT
  public:
    MarkerAndZoom(unsigned markerID, Settings::MarkerAndZoom *markerSetting, GlScope *scope);
    MarkerAndZoom(const MarkerAndZoom &) = delete;
    virtual ~MarkerAndZoom();

    /// Updates the positions and geometry of the zoomview and the marker frame.
    /// This is necessary after creating a new MarkerAndZoom object.
    void update(unsigned positionIndexIfNoSavedPos = 0, int activeMarker = -1);
    void updateActive(int activeMarker);

    /// Return the coordinates and geometry of the viewport
    inline const QRectF zoomviewPosition() const { return m_zoomviewPosition; }

    inline EdgePositionFlags snapState() const { return mouseHandler.snapState(); }

    /// Usually the resources are free'd due to parent/child relationship. This is not the case if
    /// the zoomview/marker is deleted by the user. Call destroy in this case.
    void destroy();

  private:
    /**
     * Updates the shown region of this zoom view
     * @param marker The marker region in scope coordinates
     */
    void updateZoomRegion(const QRectF &marker);

    /**
     * Move/resize this zoom view
     * @param position Position and geometry in scope coordinates
     * @return Return true if move was successful, false otherwise. If the geometry or coodinates
     *  are out of the scope coordinate boundaries, this function returns false.
     */
    bool moveZoomview(const QRectF &position);

    // State: marker ID and settings marker pointer
    Settings::MarkerAndZoom *m_markerSetting;
    unsigned m_markerID;

    // Widgets
    GlScope *m_scope;
    GlFrame *marker = nullptr;
    GlIcon *m_removeBtn = nullptr;

    QRectF m_zoomviewPosition;

    // 3D viewport and input
    Qt3DRender::QViewport *viewport = new Qt3DRender::QViewport; // will be deleted by parent
    Qt3DRender::QCamera *zoomCamera = new Qt3DRender::QCamera;   // will be deleted by parent
    GlMoveResizeSnap mouseHandler;
  signals:
    void requestRemove(unsigned markerId);
    void requestActive(unsigned markerId);
    void userChangedGeometry();
};

struct MarkerAndZoomMap : public std::map<unsigned, std::unique_ptr<MarkerAndZoom>> {
    inline void put(unsigned markerID, MarkerAndZoom *mz) {
        insert(MarkerAndZoomMap::value_type(markerID, std::unique_ptr<MarkerAndZoom>(mz)));
    }
};
