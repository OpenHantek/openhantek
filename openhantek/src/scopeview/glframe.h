#pragma once

#include <QObject>
#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QPerVertexColorMaterial>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QLayer>

#include <memory>

#include "glmoveresizesnap.h"
#include "utils/observer.h"

class GlFrame : public Qt3DCore::QEntity, public GlMoveResizeSnap {
    Q_OBJECT
  public:
    /**
     * @brief Creates a new frame entity
     * @param m_mouse A mouse device, used for moving and resizing the frame
     * @param normal The normal color
     * @param hover The hovered color
     * @param pressed The pressed color
     * @param active The actived color
     * @param colorObserver A color observer. Whenever the observer signals that a color changed, a redraw
     * with the color object pointers is performed.
     * @param m_coordinates A coordinate system
     * @param background A background material. This object will takeover owenership.
     * @param layer A layer that this entity and all subentities will be part of or null.
     * @param parent A parent entity.
     */
    explicit GlFrame(GlMouseDevice *mouse, const ScopeCoordinates *m_coordinates, const QColor *normal,
                     const QColor *hover, const QColor *pressed, const QColor *active, Observer *colorObserver,
                     Qt3DRender::QMaterial *background = nullptr, Qt3DRender::QLayer *layer = nullptr,
                     Qt3DCore::QEntity *parent = nullptr);
    virtual ~GlFrame();

    /**
     * @brief (De-)Activate this frame. An activated frame is drawn with different colors
     * @param enable
     */
    void setActive(bool enable);

    /**
     * @brief Make this frame activatable or not.
     * @param enable
     */
    void setActivatable(bool enable);

    /**
     * @brief Updates the frame position and geometry. This will not emit a frameChanged() signal.
     * @param rect Frame position and geometry
     */
    void updateRectangle(const QRectF &rect);

    /**
     * @return Returns the current position and geometry
     */
    inline const QRectF *rect() const { return &m_rect; }

    /**
     * @brief Sets a frame index. This is not necessary for the frame to function but may help
     * in differentiating between multiple frames. The frame index is attachted to all signals.
     * @param frameIndex
     */
    inline void setFrameIndex(unsigned frameIndex) { m_frameIndex = frameIndex; }

    /**
     * @brief Return the frame index. {@see setFrameIndex}
     */
    inline unsigned frameIndex() { return m_frameIndex; }

  private:
    // User provided data
    Qt3DRender::QMaterial *m_background;
    const QColor *m_normal;
    const QColor *m_hover;
    const QColor *m_pressed;
    const QColor *m_active;

    // Qt3D
    Qt3DCore::QTransform *m_transform;
    Qt3DRender::QBuffer *m_colorBuffer;

    // State
    QRectF m_rect;
    bool m_isActive = false;
    bool m_isActivatable = true;
    unsigned m_frameIndex = 0;

    void createBackground();

  protected:
    virtual void rectChanged() override;
    virtual void inputStateChanged() override;

  signals:
    /**
     * @brief Will be emitted when the user clicked on the frame, without resizing or moving it
     * @param frameIndex The frame index
     */
    void activated(unsigned frameIndex);
};
Q_DECLARE_METATYPE(EdgePositionFlags)
