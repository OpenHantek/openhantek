#pragma once

#include <QIcon>
#include <QObject>
#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QDiffuseMapMaterial>
#include <Qt3DExtras/QNormalDiffuseMapAlphaMaterial>
#include <Qt3DExtras/QPerVertexColorMaterial>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DInput/QMouseDevice>
#include <Qt3DInput/QMouseHandler>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QLayer>
#include <Qt3DRender/QPaintedTextureImage>
#include <Qt3DRender/QTexture>
#include <Qt3DRender/QTextureImage>

#include <memory>

#include "glmoveresizesnap.h"
#include "utils/observer.h"
#include "utils/scopecoordinates.h"

class PaintIcon;

class GlIcon : public Qt3DCore::QEntity, public GlMoveResizeSnap {
    Q_OBJECT
  public:
    /**
     * @brief Creates a new frame entity
     * @param m_mouse A mouse device, used for moving and resizing the frame
     * @param m_coordinates A coordinate system
     * @param normal The normal color
     * @param hover The hovered color
     * @param pressed The pressed color
     * @param colorObserver A color observer. Whenever the observer signals that a color changed, a redraw
     * with the color object pointers is performed.
     * @param background A background material. This object will takeover owenership.
     * @param layer A layer that this entity and all subentities will be part of or null.
     * @param parent A parent entity.
     */
    explicit GlIcon(GlMouseDevice *mouse, const ScopeCoordinates *m_coordinates, const QColor *normal,
                    const QColor *hover, const QColor *pressed, Observer *colorObserver, const QIcon &icon,
                    Qt3DRender::QLayer *layer = nullptr, Qt3DCore::QEntity *parent = nullptr);
    virtual ~GlIcon();

    /**
     * @brief Updates the icon position.
     * @param position Icon position in screen coordinates
     */
    void updatePosition(const QPointF &position);

    /**
     * @brief Updates the icon geometry.
     * @param size Icon geometry in screen pixels
     */
    void updateSize(QSizeF size = QSize());

    /**
     * @return Returns the current position and geometry
     */
    inline const QRectF *rect() const { return &m_rect; }

  protected:
    virtual void rectChanged() override;
    virtual void inputStateChanged() override;

  private:
    const ScopeCoordinates *m_coordinates;

    // Icon textures
    PaintIcon *m_iconTexture;

    // Background
    Qt3DExtras::QNormalDiffuseMapAlphaMaterial *m_material = new Qt3DExtras::QNormalDiffuseMapAlphaMaterial(this);
    Qt3DCore::QTransform *m_transform;
    const QColor *m_normal;
    const QColor *m_hover;
    const QColor *m_pressed;

    // State
    QRectF m_rect;
    QSizeF m_sizeInScreenPixels;
};
