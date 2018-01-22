#include "glicon.h"

#include <QCuboidMesh>
#include <QPainter>
#include <QPlaneMesh>
#include <QTextureWrapMode>

using namespace Qt3DRender;
using namespace Qt3DExtras;
using namespace Qt3DCore;

#define MAX_WIDTH DIVS_TIME / 2
#define MAX_HEIGHT DIVS_VOLTAGE / 2

class PaintIcon : public QPaintedTextureImage {
    Q_OBJECT
  public:
    PaintIcon(const QIcon &icon, QIcon::Mode mode, Qt3DCore::QNode *parent = nullptr)
        : QPaintedTextureImage(parent), m_icon(icon), m_mode(mode) {
        setSize(QSize(256, 256));
        update();
    }
    virtual ~PaintIcon() {}
    void setMode(QIcon::Mode mode) {
        m_mode = mode;
        update();
    }

  private:
    const QIcon m_icon;
    QColor border;
    QIcon::Mode m_mode = QIcon::Normal;

  protected:
    virtual void paint(QPainter *painter) override {
        int w = painter->device()->width();
        int h = painter->device()->height();

        painter->setPen(QPen(QBrush(QColor(Qt::black)), 10));
        painter->setBrush(QBrush());

        painter->drawEllipse(0, 0, w, h);
        m_icon.paint(painter, 0, 0, w, h, Qt::AlignCenter, m_mode, QIcon::On);
    }
};
#include "glicon.moc"

GlIcon::GlIcon(GlMouseDevice *mouse, const ScopeCoordinates *coordinates, const QColor *normal, const QColor *hover,
               const QColor *pressed, Observer *colorObserver, const QIcon &icon, QLayer *layer,
               Qt3DCore::QEntity *parent)
    : Qt3DCore::QEntity(parent), GlMoveResizeSnap(&m_rect, 0, mouse, coordinates, this), m_coordinates(coordinates),
      m_normal(normal), m_hover(hover), m_pressed(pressed) {

    // Move/Resize init
    setResizable(false);
    setMovable(false);
    m_inputPriority = 1;

    auto te = new QEntity;
    te->setParent(this);
    if (parent) this->setParent(parent);

    QObject::connect(colorObserver, &Observer::changed, this, &GlIcon::inputStateChanged);
    QObject::connect(m_coordinates, &ScopeCoordinates::rectChanged, this, [this]() { updateSize(); });

    m_iconTexture = new PaintIcon(icon, QIcon::Normal);
    m_material->diffuse()->addTextureImage(m_iconTexture);

    // We need a sub-entity, because QPlaneMesh is on the yz-axes, center-aligned
    // and we need to rotate it to be on the xy-axes and translate it have its origin left/bottom
    auto m = new QPlaneMesh;
    m->setWidth(1);
    m->setHeight(1);
    te->addComponent(m);
    te->addComponent(m_material);
    auto tr = new Qt3DCore::QTransform(this);
    tr->setRotationX(45);                       ///< Make it a xy plane
    tr->setTranslation(QVector3D(0.5, 0.5, 0)); ///<Translate so that the left/bottom corner is in the origin
    te->addComponent(tr);

    m_transform = new Qt3DCore::QTransform(this);
    addComponent(m_transform);
    if (layer) addComponent(layer);
    updateSize(QSizeF(32, 32)); ///< 32x32px icon
    inputStateChanged();
}

GlIcon::~GlIcon() {
    m_material->diffuse()->removeTextureImage(m_iconTexture);
    delete m_iconTexture;
}

void GlIcon::updatePosition(const QPointF &position) {
    m_rect.moveTo(position);
    rectChanged();
}

void GlIcon::updateSize(QSizeF size) {
    if (size.height() > 0.0f) m_sizeInScreenPixels = size;
    m_rect.setWidth(m_coordinates->width((float)m_sizeInScreenPixels.width()));
    m_rect.setHeight(m_coordinates->height((float)m_sizeInScreenPixels.height()));
    m_transform->setScale3D(QVector3D((float)m_rect.width(), (float)m_rect.height(), 0.5));
}

void GlIcon::rectChanged() { m_transform->setTranslation(QVector3D((float)m_rect.left(), (float)m_rect.top(), 0)); }

void GlIcon::inputStateChanged() {
    const QColor *color = m_hoveredParts != EdgePositionFlags::None ? m_hover : (m_isPressed ? m_pressed : m_normal);
    // m_iconTexture->setMode(m_isHovered ? QIcon::Selected : (m_isPressed ? QIcon::Active : QIcon::Normal));
    m_material->setAmbient(*color);
}
