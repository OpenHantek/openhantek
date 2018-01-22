#pragma once

#include "glmousedevice.h"
#include "glscopehover.h"
#include "utils/scopecoordinates.h"

#include <Qt3DCore/QEntity>
#include <Qt3DInput/QMouseDevice>
#include <Qt3DInput/QMouseHandler>

/**
 * Because we do not want GlMoveResizeSnap to inherit from a QObject
 * but still want to send signals, we use a delegate object.
 */
class GlMoveResizeSnapSignals : public QObject {
    Q_OBJECT
  signals:

    /**
     * @brief Emitted whenever the position or geometry of the frame changed
     * @param id The frame index
     * @param rect The new size and position
     */
    void frameChanged(unsigned id, const QRectF &rect);

    /**
     * @brief Emitted when clicked
     * @param id The frame index
     */
    void clicked(unsigned id);

    /**
     * @brief This signal is emitted whenever the hover status changed
     * @param id The frame index
     * @param hoveredAreas Test for a left hover with `hoveredAreas / EdgePositionFlags::Left` for example.
     */
    void hovered(unsigned id, EdgePositionFlags hoveredAreas);

    /**
     * @brief This signal is emitted whenever the frame snapped to a border of the containing view
     * @param id The frame index
     */
    void snapChanged(unsigned id);

    /**
     * @brief If the mouse hovers over the frame and it is movable or resizable, it will emit this signal
     * and request a fitting mouse cursor shape.
     * @param shape The cursor shape
     */
    void requestMouseCursor(Qt::CursorShape shape);
};

/**
 * Inherit from this class to gain mouse interactive abilities like resizing, moving, snapping, mouse clicks
 * and hover states.
 */
class GlMoveResizeSnap {
  public:
    GlMoveResizeSnap(QRectF *rect, unsigned *id, GlMouseDevice *mouse, const ScopeCoordinates *coordinates,
                     Qt3DCore::QEntity *parent);
    virtual ~GlMoveResizeSnap();

    /**
     * @brief Allows or disallows interactions (move/resize/activate).
     * @param enable
     */
    void setAllowInteraction(bool enable);

    /**
     * @brief Allow or disallow this frame to be resizable
     * @param enable
     */
    inline void setResizable(bool enable) { m_isResizable = enable; }

    /**
     * @brief Allow or disallow this frame to be movable
     * @param enable
     */
    inline void setMovable(bool enable) { m_isMovable = enable; }

    /**
     * @return Returns the snapped borders.
     */
    inline EdgePositionFlags snapState() const { return m_snappedParts; }

    inline GlMoveResizeSnapSignals *moveResizeSignals() { return &signalEmitter; }

    inline const ScopeCoordinates *coordinateSystem() const { return m_coordinates; }

    /**
     * Call this method in your class, when the position/geometry rectangle was updated.
     */
    void updateSnap();

  protected:
    /**
     * You will be informed, whenever the position/geometry rectangle changes.
     */
    virtual void rectChanged();
    /**
     * Implement this to receive a call whenever the input state (hover state, pressed state) changed.
     */
    virtual void inputStateChanged();

    // Input state
    EdgePositionFlags m_hoveredParts;
    bool m_isPressed = false;
    int m_inputPriority = 0;

    // Position/Geometry state
    const ScopeCoordinates *m_coordinates;

    const unsigned *m_id;

  protected:
    // Position/Geometry state
    QRectF *m_rect;

  private:
    // Input state
    EdgePositionFlags m_snappedParts = EdgePositionFlags::None;
    QVector2D m_localGrabPos;
    bool m_isInteractive = true;
    bool m_isResizable = true;
    bool m_isMovable = true;

    GlMouseDevice *m_mouse;
    GlMoveResizeSnapSignals signalEmitter;

    void mouseMoved(Qt3DInput::QMouseEvent *event);
    void mouseClicked(Qt3DInput::QMouseEvent *event);
    void handleMoveResize(float newX, float newY);
    void resetState();
};
