#include "glmoveresizesnap.h"
#include <Qt3DInput/QMouseHandler>

static const unsigned invalidID = INT_MAX;

GlMoveResizeSnap::GlMoveResizeSnap(QRectF *rect, unsigned *id, GlMouseDevice *mouse,
                                   const ScopeCoordinates *coordinates, Qt3DCore::QEntity *parent)
    : m_coordinates(coordinates), m_id(id ? id : &invalidID), m_rect(rect), m_mouse(mouse) {

    qRegisterMetaType<EdgePositionFlags>("HoveredPosition");
    QObject::connect(mouse, &QObject::destroyed, &signalEmitter, [this]() { m_mouse = nullptr; });

    // Input handling. Create a mouseHandler. Disable it if another object has the focus
    if (!mouse) return;
    Qt3DInput::QMouseHandler *mouseHandler = new Qt3DInput::QMouseHandler(parent);
    mouseHandler->setSourceDevice(mouse);
    mouseHandler->setEnabled(true);
    QObject::connect(
        mouse, &GlMouseDevice::focusObjectChanged, &signalEmitter,
        [this, mouseHandler](void *focusObject) { mouseHandler->setEnabled(focusObject == this || !focusObject); });
    QObject::connect(mouse, &GlMouseDevice::focusStoolen, &signalEmitter, [this]() { this->resetState(); });
    parent->addComponent(mouseHandler);
    QObject::connect(mouseHandler, &Qt3DInput::QMouseHandler::positionChanged, &signalEmitter,
                     [this](Qt3DInput::QMouseEvent *e) { mouseMoved(e); });
    QObject::connect(mouseHandler, &Qt3DInput::QMouseHandler::pressed, &signalEmitter,
                     [this](Qt3DInput::QMouseEvent *e) { mouseClicked(e); });
    QObject::connect(mouseHandler, &Qt3DInput::QMouseHandler::released, &signalEmitter,
                     [this](Qt3DInput::QMouseEvent *e) { mouseClicked(e); });
}

GlMoveResizeSnap::~GlMoveResizeSnap() {
    if (m_mouse) m_mouse->unsetFocusObject(&signalEmitter);
}

void GlMoveResizeSnap::setAllowInteraction(bool enable) {
    m_isInteractive = enable;
    if (!enable) {
        m_isPressed = false;
        inputStateChanged();
    }
}

void GlMoveResizeSnap::rectChanged() {}

void GlMoveResizeSnap::inputStateChanged() {}

void GlMoveResizeSnap::mouseMoved(Qt3DInput::QMouseEvent *event) {
    if (!m_isInteractive) return;
    const float newX = m_coordinates->x(event->x());
    const float newY = m_coordinates->y(event->y());

    if (m_isPressed) {
        handleMoveResize(newX, newY);
        return;
    }

    if (newX < m_rect->left() - m_coordinates->ratioX() * 8 || newX > m_rect->right() + m_coordinates->ratioX() * 8 ||
        newY < m_rect->top() - m_coordinates->ratioY() * 8 || newY > m_rect->bottom() + m_coordinates->ratioX() * 8) {
        m_mouse->unsetFocusObject(&signalEmitter);
        resetState();
        return;
    }

    if (!m_mouse->grabFocus(&signalEmitter, m_inputPriority)) return;

    Qt::CursorShape cursorShape = Qt::ArrowCursor;
    EdgePositionFlags newFlag = EdgePositionFlags::None;
    if (std::abs(newX - m_rect->left()) < m_coordinates->ratioX() * 8) {
        newFlag |= EdgePositionFlags::Left;
        cursorShape = Qt::SizeHorCursor;
    }
    if (std::abs(newX - m_rect->right()) < m_coordinates->ratioX() * 8) {
        newFlag |= EdgePositionFlags::Right;
        cursorShape = Qt::SizeHorCursor;
    }
    if (std::abs(newY - m_rect->top()) < m_coordinates->ratioY() * 8) {
        newFlag |= EdgePositionFlags::Top;
        cursorShape = (cursorShape == Qt::SizeHorCursor) ? Qt::SizeBDiagCursor : Qt::SizeVerCursor;
    }
    if (std::abs(newY - m_rect->bottom()) < m_coordinates->ratioY() * 8) {
        newFlag |= EdgePositionFlags::Bottom;
        cursorShape = (cursorShape == Qt::SizeHorCursor) ? Qt::SizeBDiagCursor : Qt::SizeVerCursor;
    }
    if (newFlag == EdgePositionFlags::None) {
        newFlag = EdgePositionFlags::Middle;
        cursorShape = Qt::SizeAllCursor;
    }

    if (m_hoveredParts != newFlag) {
        m_hoveredParts = newFlag;
        emit signalEmitter.requestMouseCursor(cursorShape);
        inputStateChanged();
        emit signalEmitter.hovered(*m_id, m_hoveredParts);
    }
}

void GlMoveResizeSnap::mouseClicked(Qt3DInput::QMouseEvent *event) {
    bool pressed = (unsigned)event->buttons() & Qt::LeftButton;
    if (!pressed && m_isPressed) {
        m_isPressed = false;
        const float newX = m_coordinates->x(event->x());
        const float newY = m_coordinates->y(event->y());
        // Just a click
        if (m_localGrabPos.x() == newX - (float)m_rect->x() && m_localGrabPos.y() == newY - (float)m_rect->y()) {
            emit signalEmitter.clicked(*m_id);
        }
        inputStateChanged();
    } else if (pressed && m_hoveredParts != EdgePositionFlags::None) {
        const float newX = m_coordinates->x(event->x());
        const float newY = m_coordinates->y(event->y());
        m_localGrabPos = QVector2D(newX - (float)m_rect->x(), newY - (float)m_rect->y());
        m_isPressed = true;
    }
}

void GlMoveResizeSnap::handleMoveResize(float newX, float newY) {
    const QRectF &borderRect = m_coordinates->fixedScopeRect();
    // Handle move
    if (m_isMovable && EdgePositionFlags::Middle == m_hoveredParts) {
        float x = newX - m_localGrabPos.x();
        x = std::min(x, (float)borderRect.right() - (float)m_rect->width()); // c++17 cramp
        x = std::max(x, (float)borderRect.left());

        float y = newY - m_localGrabPos.y();
        y = std::min(y, (float)borderRect.bottom() - (float)m_rect->height()); // c++17 cramp
        y = std::max(y, (float)borderRect.top());

        m_rect->moveTo(x, y);
        rectChanged();
        updateSnap();
        emit signalEmitter.frameChanged(*m_id, *m_rect);
    } else if (m_isResizable) {
        if (m_hoveredParts / EdgePositionFlags::Left) {
            float x = std::min(newX, (float)m_rect->right() - 0.2f); // don't go over the other side of the rect
            x = std::max(x, (float)borderRect.left());               // don't go beyond the border
            m_rect->setLeft(x);
        }
        if (m_hoveredParts / EdgePositionFlags::Right) {
            float x = std::max(newX, (float)m_rect->left() + 0.2f);
            x = std::min(x, (float)borderRect.right()); // don't go beyond the border
            m_rect->setRight(x);
        }
        if (m_hoveredParts / EdgePositionFlags::Top) {
            float y = std::min(newY, (float)m_rect->bottom() - 0.2f); // don't go over the other side of the rect
            y = std::max(y, (float)borderRect.top());                 // don't go beyond the border
            m_rect->setTop(y);
        }
        if (m_hoveredParts / EdgePositionFlags::Bottom) {
            float y = std::max(newY, (float)m_rect->top() + 0.2f); // don't go over the other side of the rect
            y = std::min(y, (float)borderRect.bottom());           // don't go beyond the border
            m_rect->setBottom(y);
        }
        rectChanged();
        m_snappedParts = EdgePositionFlags::None;
        updateSnap();
        emit signalEmitter.frameChanged(*m_id, *m_rect);
    }
}

void GlMoveResizeSnap::updateSnap() {
    const QRectF &m = m_coordinates->scopeRect();
    auto snapped = EdgePositionFlags::None;
    if (m_rect->left() <= (float)m.left()) {
        snapped |= EdgePositionFlags::Left;
    } else if (m_rect->right() >= (float)m.right()) {
        snapped |= EdgePositionFlags::Right;
    } else if (m_rect->top() <= (float)m.top()) {
        snapped |= EdgePositionFlags::Top;
    } else if (m_rect->bottom() >= (float)m.bottom()) {
        snapped |= EdgePositionFlags::Bottom;
    }

    if (m_snappedParts == snapped) return;
    m_snappedParts = snapped;
    emit signalEmitter.snapChanged(*m_id);
}

void GlMoveResizeSnap::resetState() {
    if (m_hoveredParts != EdgePositionFlags::None) {
        m_hoveredParts = EdgePositionFlags::None;
        emit signalEmitter.requestMouseCursor(Qt::ArrowCursor);
        inputStateChanged();
    }
}
