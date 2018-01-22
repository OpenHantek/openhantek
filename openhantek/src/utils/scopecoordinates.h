#pragma once

#include "viewconstants.h"
#include <QDebug>
#include <QObject>
#include <QRectF>

/**
 * Helper class to compute scope coordinates from screen coordinates. It is usually
 * enough to have one instance, update it in your main window and pass it around.
 */
class ScopeCoordinates : public QObject {
    Q_OBJECT
  public:
    ScopeCoordinates(const QSize &r) { updateScreenSize(r); }
    /// Copy constructor: Connect to the original scope coordinates change event
    ScopeCoordinates(const ScopeCoordinates &orig) {
        updateScreenSize(orig.m_screenSize);
        connect(&orig, &ScopeCoordinates::rectChanged, this, [this](const QSize &r) { updateScreenSize(r); });
    }

    /// Call this in the resize method of your widget/window
    inline void updateScreenSize(const QSize &screenSize) {
        m_screenSize = screenSize;
        m_ratioX = float(m_scopeRect.width() / m_screenSize.width());
        m_ratioY = float(m_scopeRect.height() / m_screenSize.height());
        emit rectChanged(screenSize);
    }
    inline float x(float screenX) const { return screenX * m_ratioX + (float)m_scopeRect.left(); }
    inline float y(float screenY) const { return (float)m_scopeRect.bottom() - screenY * m_ratioY; }
    inline float width(float screenWidth) const { return screenWidth * m_ratioX; }
    inline float height(float screenHeight) const { return screenHeight * m_ratioY; }
    inline float ratioX() const { return m_ratioX; }
    inline float ratioY() const { return m_ratioY; }
    inline QSize screenSize() const { return m_screenSize; }
    inline const QRectF &scopeRect() const { return m_scopeRect; }
    inline const QRectF &fixedScopeRect() const { return m_fixedScopeRect; }
    inline void setScopeRect(const QRectF &scopeRect) {
        m_scopeRect = scopeRect;
        updateScreenSize(m_screenSize);
    }

    /**
     * A viewport doesn't want the absolute world-position (dependant on the current camera view),
     * but a normalized position rectangle (x,y,w,h) with x,y,w,h ∈ [0,1]
     * @param position World coordinates
     * @return Normalized coordinates
     */
    static QRectF computeNormalizedRect(QRectF viewScopeRect, const QRectF &fullScopeRect) {
        viewScopeRect.translate(fullScopeRect.width() / 2, fullScopeRect.height() / 2);
        return QRectF(viewScopeRect.left() / fullScopeRect.width(),        // left
                      1 - viewScopeRect.bottom() / fullScopeRect.height(), // top
                      viewScopeRect.width() / fullScopeRect.width(),       // width
                      viewScopeRect.height() / fullScopeRect.height());    // height
    }

  private:
    QSize m_screenSize;
    const QRectF m_fixedScopeRect = QRectF(-DIVS_TIME / 2, -DIVS_VOLTAGE / 2, DIVS_TIME, DIVS_VOLTAGE);
    QRectF m_scopeRect = m_fixedScopeRect;
    float m_ratioX = 1.0f;
    float m_ratioY = 1.0f;
  signals:
    void rectChanged(const QSize &r);
};
