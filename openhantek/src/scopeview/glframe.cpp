#include "glframe.h"

using namespace Qt3DRender;
using namespace Qt3DExtras;
using namespace Qt3DCore;

GlFrame::GlFrame(GlMouseDevice *mouse, const ScopeCoordinates *coordinates, const QColor *normal, const QColor *hover,
                 const QColor *pressed, const QColor *active, Observer *colorObserver, QMaterial *background,
                 QLayer *layer, Qt3DCore::QEntity *parent)
    : Qt3DCore::QEntity(parent), GlMoveResizeSnap(&m_rect, &m_frameIndex, mouse, coordinates, this),
      m_background(background), m_normal(normal), m_hover(hover), m_pressed(pressed), m_active(active) {

    static const float frameV[] = {0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0,  // lines
                                   1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0,  // lines
                                   0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,  // normales
                                   0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1}; // normales

    QByteArray frameGeometryTemplate;
    frameGeometryTemplate.setRawData((char *)frameV, sizeof(frameV));

    auto material = new Qt3DExtras::QPerVertexColorMaterial(this);
    QObject::connect(colorObserver, &Observer::changed, this, &GlFrame::inputStateChanged);
    addComponent(material);

    m_colorBuffer = new QBuffer(QBuffer::VertexBuffer, material);
    m_colorBuffer->setUsage(QBuffer::DynamicDraw);
    QByteArray t;
    t.fill(0, sizeof(QVector4D) * 8);
    m_colorBuffer->setData(t);

    const int numV = 8;
    QBuffer *dataBuffer = new QBuffer(QBuffer::VertexBuffer, this);
    dataBuffer->setData(frameGeometryTemplate);
    const auto namePosition = QAttribute::defaultPositionAttributeName();
    auto attr = new QAttribute(dataBuffer, namePosition, QAttribute::Float, 3, numV, 0, 0);
    const auto nameNormal = QAttribute::defaultNormalAttributeName();
    auto attrN = new QAttribute(dataBuffer, nameNormal, QAttribute::Float, 3, numV, numV * sizeof(QVector3D));
    const auto nameColor = QAttribute::defaultColorAttributeName();
    auto attrC = new QAttribute(m_colorBuffer, nameColor, QAttribute::Float, 4, numV);
    auto mesh = new QGeometryRenderer(this);
    auto geometry = new QGeometry(mesh);
    geometry->addAttribute(attr);
    geometry->addAttribute(attrN);
    geometry->addAttribute(attrC);
    mesh->setVertexCount((int)numV);
    mesh->setPrimitiveType(QGeometryRenderer::Lines);
    mesh->setGeometry(geometry);
    addComponent(mesh);

    if (background) createBackground();

    m_transform = new Qt3DCore::QTransform(this);
    addComponent(m_transform);
    if (layer) addComponent(layer);

    connect(moveResizeSignals(), &GlMoveResizeSnapSignals::clicked, this, [this]() {
        if (m_isActivatable) {
            m_isActive = true;
            emit activated(frameIndex());
        }
    });
}

GlFrame::~GlFrame() {}

void GlFrame::setActive(bool enable) {
    if (!m_isActivatable && enable) return;
    m_isActive = enable;
    inputStateChanged();
}

void GlFrame::setActivatable(bool enable) {
    m_isActivatable = enable;
    if (!enable) setActive(false);
}

void GlFrame::updateRectangle(const QRectF &rect) {
    m_rect = rect;

    const QRectF m = m_coordinates->scopeRect();
    if (m_rect.left() <= m.left()) m_rect.setLeft(m.left());
    if (m_rect.right() >= m.right()) m_rect.setRight(m.right());

    if (m_rect.top() <= m.top()) m_rect.setTop(m.top());
    if (m_rect.bottom() >= m.bottom()) m_rect.setBottom(m.bottom());

    if (m_rect.height() < 0.3) m_rect.setHeight(0.3);
    if (m_rect.width() < 0.3) m_rect.setWidth(0.3);

    rectChanged();
    inputStateChanged();
    updateSnap();
}

inline QVector4D color2vec(const QColor &c) {
    return QVector4D((float)c.redF(), (float)c.greenF(), (float)c.blueF(), (float)c.alphaF());
}
void GlFrame::createBackground() {
    Qt3DCore::QEntity *bgEntity = new QEntity(this);

    bgEntity->addComponent(m_background);

    static const QVector3D frameV[] = {QVector3D(0, 1, 0.1f), QVector3D(0, 0, 0.1f), QVector3D(1, 1, 0.1f),
                                       QVector3D(1, 0, 0.1f), QVector3D(0, 0, 0.1f), // vertex; lines
                                       QVector3D(0, 0, 1),    QVector3D(0, 0, 1),    QVector3D(0, 0, 1),
                                       QVector3D(0, 0, 1),    QVector3D(0, 0, 1)}; // normales

    QByteArray frameBgGeometry;
    frameBgGeometry.setRawData((char *)frameV, sizeof(frameV));

    const int numV = 5;
    QBuffer *dataBuffer = new QBuffer(QBuffer::VertexBuffer, this);
    dataBuffer->setData(frameBgGeometry);
    const auto namePosition = QAttribute::defaultPositionAttributeName();
    auto attr = new QAttribute(dataBuffer, namePosition, QAttribute::Float, 3, numV, 0, 0);
    const auto nameNormal = QAttribute::defaultNormalAttributeName();
    auto attrN = new QAttribute(dataBuffer, nameNormal, QAttribute::Float, 3, numV, numV * sizeof(QVector3D));
    auto mesh = new QGeometryRenderer(this);
    auto geometry = new QGeometry(mesh);
    geometry->addAttribute(attr);
    geometry->addAttribute(attrN);
    mesh->setVertexCount((int)numV);
    mesh->setPrimitiveType(QGeometryRenderer::TriangleStrip);
    mesh->setGeometry(geometry);
    bgEntity->addComponent(mesh);
    bgEntity->setParent(this);
}

void GlFrame::rectChanged() {
    m_transform->setScale3D(QVector3D((float)m_rect.width(), (float)m_rect.height(), 0));
    m_transform->setTranslation(QVector3D((float)m_rect.left(), (float)m_rect.top(), 0));
}

void GlFrame::inputStateChanged() {
    const QVector4D color = color2vec(m_isActive ? *m_active : *m_normal);
    const QVector4D colorHover = color2vec(m_isPressed ? *m_pressed : *m_hover);

    QByteArray colorB;
    colorB.resize(sizeof(QVector4D) * 8);
    QVector4D *colorBp = reinterpret_cast<QVector4D *>(colorB.data());
    *(colorBp++) = m_hoveredParts / EdgePositionFlags::Top ? colorHover : color;
    *(colorBp++) = m_hoveredParts / EdgePositionFlags::Top ? colorHover : color;

    *(colorBp++) = m_hoveredParts / EdgePositionFlags::Right ? colorHover : color;
    *(colorBp++) = m_hoveredParts / EdgePositionFlags::Right ? colorHover : color;

    *(colorBp++) = m_hoveredParts / EdgePositionFlags::Bottom ? colorHover : color;
    *(colorBp++) = m_hoveredParts / EdgePositionFlags::Bottom ? colorHover : color;

    *(colorBp++) = m_hoveredParts / EdgePositionFlags::Left ? colorHover : color;
    *(colorBp++) = m_hoveredParts / EdgePositionFlags::Left ? colorHover : color;

    m_colorBuffer->updateData(0, colorB);
}
