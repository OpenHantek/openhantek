// SPDX-License-Identifier: GPL-2.0+

#include <cmath>

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QWidget>

#include "levelslider.h"

LevelSlider::LevelSlider(Qt::ArrowType direction, QWidget *parent) : QWidget(parent) {
    QFont font = this->font();
    font.setPointSizeF(font.pointSize() * 0.8);
    this->setFont(font);

    setDirection(direction);
    calculateWidth();
}

int LevelSlider::preMargin() const { return this->_preMargin; }

int LevelSlider::postMargin() const { return this->_postMargin; }

LevelSlider::IndexType LevelSlider::addSlider(IndexType index, const QString &text) {
    if (index == INVALID) // No index given by the user: choose the next free one
        index = newItemIndex();
    else if (slider.find(index) != slider.end()) // index already known? exit now
        return INVALID;

    LevelSliderParameters *parameters = new LevelSliderParameters;
    parameters->color = Qt::white;
    parameters->minimum = 0x00;
    parameters->maximum = 0xff;
    parameters->value = 0x00;
    parameters->visible = false;

    slider.insert(std::make_pair(index, parameters));
    setText(index, text);
    return index;
}

void LevelSlider::removeSlider(IndexType index) {
    if (_pressedSlider == index) _pressedSlider = INVALID;
    slider.erase(index);
    this->calculateWidth();
}

void LevelSlider::removeAll() {
    _pressedSlider = INVALID;
    slider.clear();
    this->calculateWidth();
}

QSize LevelSlider::sizeHint() const {
    if (_direction == Qt::RightArrow || _direction == Qt::LeftArrow)
        return QSize(_sliderWidth, 16);
    else
        return QSize(16, _sliderWidth);
}

const QColor LevelSlider::color(IndexType index) const {
    auto findIt = slider.find(index);
    if (findIt == slider.end()) return Qt::black;
    return findIt->second->color;
}

void LevelSlider::setColor(IndexType index, QColor color) {
    auto findIt = slider.find(index);
    if (findIt == slider.end()) return;
    findIt->second->color = color;
    repaint();
}

const QString LevelSlider::text(IndexType index) const {
    auto findIt = slider.find(index);
    if (findIt == slider.end()) return QString();
    return findIt->second->text;
}

void LevelSlider::setText(IndexType index, const QString &text) {
    auto findIt = slider.find(index);
    if (findIt == slider.end()) return;
    findIt->second->text = text;
    calculateWidth();
}

bool LevelSlider::visible(IndexType index) const {
    auto findIt = slider.find(index);
    if (findIt == slider.end()) return false;
    return findIt->second->visible;
}

void LevelSlider::setIndexVisible(IndexType index, bool visible) {
    auto findIt = slider.find(index);
    if (findIt == slider.end()) return;
    findIt->second->visible = visible;
    repaint();
}

double LevelSlider::minimum(IndexType index) const {
    auto findIt = slider.find(index);
    if (findIt == slider.end()) return -1;
    return findIt->second->minimum;
}

double LevelSlider::maximum(IndexType index) const {
    auto findIt = slider.find(index);
    if (findIt == slider.end()) return -1;
    return findIt->second->maximum;
}

void LevelSlider::setLimits(IndexType index, double minimum, double maximum) {
    auto findIt = slider.find(index);
    if (findIt == slider.end()) return;
    findIt->second->minimum = minimum;
    findIt->second->maximum = maximum;
    fixValue(findIt->second.get());
    calculateRect(findIt->second.get());
    repaint();
}

double LevelSlider::step(IndexType index) const {
    auto findIt = slider.find(index);
    if (findIt == slider.end()) return -1;
    return findIt->second->step;
}

double LevelSlider::setStep(IndexType index, double step) {
    auto findIt = slider.find(index);
    if (findIt == slider.end()) return -1;
    if (step > 0) findIt->second->step = step;
    return findIt->second->step;
}

double LevelSlider::value(IndexType index) const {
    auto findIt = slider.find(index);
    if (findIt == slider.end()) return -1;
    return findIt->second->value;
}

void LevelSlider::setValue(LevelSlider::IndexType index, double value) {
    auto findIt = slider.find(index);
    if (findIt == slider.end()) return;
    setValue(findIt->second.get(), value);
    if (_pressedSlider == INVALID) emit valueChanged(index, value);
}

void LevelSlider::setValue(LevelSliderParameters *parameter, double value) {
    parameter->value = value;
    fixValue(parameter);
    calculateRect(parameter);
    repaint();
}

Qt::ArrowType LevelSlider::direction() const { return _direction; }

void LevelSlider::setDirection(Qt::ArrowType direction) {
    if (direction < Qt::UpArrow || direction > Qt::RightArrow) return;

    _direction = direction;

    if (_direction == Qt::RightArrow || _direction == Qt::LeftArrow) {
        _preMargin = fontMetrics().lineSpacing();
        _postMargin = 3;
    } else {
        _preMargin = fontMetrics().averageCharWidth() * 3;
        _postMargin = 3;
    }
}

void LevelSlider::mouseMoveEvent(QMouseEvent *event) {
    if (_pressedSlider == INVALID) {
        event->ignore();
        return;
    }

    // Get new value
    double value;
    if (_direction == Qt::RightArrow || _direction == Qt::LeftArrow)
        value = _pressedSliderParams->maximum -
                (_pressedSliderParams->maximum - _pressedSliderParams->minimum) *
                    ((double)event->y() - this->_preMargin + 0.5) /
                    (height() - this->_preMargin - this->_postMargin - 1);
    else
        value = _pressedSliderParams->minimum +
                (_pressedSliderParams->maximum - _pressedSliderParams->minimum) *
                    ((double)event->x() - this->_preMargin + 0.5) /
                    (width() - this->_preMargin - this->_postMargin - 1);

    // Move the slider
    if (event->modifiers() & Qt::AltModifier)
        setValue(_pressedSliderParams, value); ///< Alt allows every position
    else {
        /// Set to nearest possible position
        const double vQ = std::trunc(value / _pressedSliderParams->step) * _pressedSliderParams->step;
        setValue(_pressedSliderParams, vQ);
    }

    emit valueChanged(_pressedSlider, _pressedSliderParams->value);
    event->accept();
}

void LevelSlider::mousePressEvent(QMouseEvent *event) {
    if (!(event->button() & Qt::LeftButton)) {
        event->ignore();
        return;
    }

    _pressedSlider = INVALID;
    for (auto &p : slider) {
        LevelSliderParameters *lp = p.second.get();
        if (lp->visible && lp->rect.contains(event->pos())) {
            _pressedSlider = p.first;
            _pressedSliderParams = p.second.get();
            break;
        }
    }
    // Accept event if a slider was pressed
    event->setAccepted(_pressedSlider != INVALID);
}

void LevelSlider::mouseReleaseEvent(QMouseEvent *event) {
    if (!(event->button() & Qt::LeftButton) || _pressedSlider == INVALID) {
        event->ignore();
        return;
    }

    emit valueChanged(_pressedSlider, slider.at(_pressedSlider)->value);
    _pressedSlider = INVALID;
    _pressedSliderParams = nullptr;

    event->accept();
}

void LevelSlider::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    int flags = 0;
    switch (_direction) {
    case Qt::LeftArrow:
        flags = Qt::AlignLeft | Qt::AlignBottom;
        break;
    case Qt::UpArrow:
        flags = Qt::AlignTop | Qt::AlignHCenter;
        break;
    case Qt::DownArrow:
        flags = Qt::AlignBottom | Qt::AlignHCenter;
        break;
    default:
        flags = Qt::AlignRight | Qt::AlignBottom;
    }

    for (auto &p : slider) {
        LevelSliderParameters *lp = p.second.get();

        if (!lp->visible) continue;

        painter.setPen(lp->color);

        if (lp->text.isEmpty()) {
            QVector<QPoint> needlePoints;
            QRect &sRect = lp->rect;
            const int W = _sliderWidth;

            switch (_direction) {
            case Qt::LeftArrow:
                needlePoints << QPoint(sRect.left() + 4, sRect.top()) << QPoint(sRect.left() + 1, sRect.top() + 3)
                             << QPoint(sRect.left() + 4, sRect.top() + 6) << QPoint(sRect.left() + W, sRect.top() + 6)
                             << QPoint(sRect.left() + W, sRect.top());
                break;
            case Qt::UpArrow:
                needlePoints << QPoint(sRect.left(), sRect.top() + 4) << QPoint(sRect.left() + 3, sRect.top() + 1)
                             << QPoint(sRect.left() + 6, sRect.top() + 4) << QPoint(sRect.left() + 6, sRect.top() + W)
                             << QPoint(sRect.left(), sRect.top() + W);
                break;
            case Qt::DownArrow:
                needlePoints << QPoint(sRect.left(), sRect.top() + W - 5)
                             << QPoint(sRect.left() + 3, sRect.top() + W - 2)
                             << QPoint(sRect.left() + 6, sRect.top() + W - 5) << QPoint(sRect.left() + 6, sRect.top())
                             << QPoint(sRect.left(), sRect.top());
                break;
            case Qt::RightArrow:
                needlePoints << QPoint(sRect.left() + W - 5, sRect.top())
                             << QPoint(sRect.left() + W - 2, sRect.top() + 3)
                             << QPoint(sRect.left() + W - 5, sRect.top() + 6) << QPoint(sRect.left(), sRect.top() + 6)
                             << QPoint(sRect.left(), sRect.top());
                break;
            default:
                break;
            }

            painter.setBrush(QBrush(lp->color, isEnabled() ? Qt::SolidPattern : Qt::NoBrush));
            painter.drawPolygon(QPolygon(needlePoints));
            painter.setBrush(Qt::NoBrush);
        } else {
            // Get rect for text and draw needle
            QRect textRect = lp->rect;
            if (_direction == Qt::UpArrow || _direction == Qt::DownArrow) {
                textRect.setRight(textRect.right() - 1);
                if (_direction == Qt::UpArrow) {
                    textRect.setTop(textRect.top() + 1);
                    painter.drawLine(lp->rect.right(), 0, lp->rect.right(), 7);
                } else {
                    textRect.setBottom(textRect.bottom() - 1);
                    painter.drawLine(lp->rect.right(), _sliderWidth - 8, lp->rect.right(), _sliderWidth - 1);
                }
            } else {
                textRect.setBottom(textRect.bottom() - 1);
                if (_direction == Qt::LeftArrow) {
                    textRect.setLeft(textRect.left() + 1);
                    painter.drawLine(0, lp->rect.bottom(), 7, lp->rect.bottom());
                } else {
                    textRect.setRight(textRect.right() - 1);
                    painter.drawLine(_sliderWidth - 8, lp->rect.bottom(), _sliderWidth - 1, lp->rect.bottom());
                }
            }
            // Draw text
            painter.drawText(textRect, flags, lp->text);
        }
    }

    event->accept();
}

void LevelSlider::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event);
    for (auto &p : slider) {
        calculateRect(p.second.get());
        repaint();
    }
}

QRect LevelSlider::calculateRect(LevelSliderParameters *parameters) {
    // Is it a vertical slider?
    if (_direction == Qt::RightArrow || _direction == Qt::LeftArrow) {
        const long yBase =
            (long)((double)(height() - _preMargin - _postMargin - 1) * (parameters->maximum - parameters->value) /
                       (parameters->maximum - parameters->minimum) +
                   0.5f);

        // Is it a triangular needle?
        if (parameters->text.isEmpty()) {
            parameters->rect = QRect(0, // Start at the left side
                                     // The needle should be center-aligned, 0.5 pixel offset for
                                     // exact pixelization
                                     (int)yBase + _preMargin - 3,
                                     _sliderWidth, // Fill the whole width
                                     7             // The needle is 7 px wide
                                     );
        }
        // Or a thin needle with text?
        else {
            parameters->rect = QRect(0, // Start at the left side
                                     // The needle is at the bottom, the text above it, 0.5 pixel
                                     // offset for exact pixelization
                                     (int)yBase,
                                     _sliderWidth,   // Fill the whole width
                                     preMargin() + 1 // Use the full margin
                                     );
        }
    }
    // Or a horizontal slider?
    else {
        const long xBase =
            (long)((double)(width() - this->_preMargin - this->_postMargin - 1) *
                       (parameters->value - parameters->minimum) / (parameters->maximum - parameters->minimum) +
                   0.5f);
        // Is it a triangular needle?
        if (parameters->text.isEmpty()) {
            // The needle should be center-aligned, 0.5 pixel offset for exact
            // pixelization
            parameters->rect = QRect((int)xBase + _preMargin - 3,
                                     0,           // Start at the top
                                     7,           // The needle is 7 px wide
                                     _sliderWidth // Fill the whole height
                                     );
        }
        // Or a thin needle with text?
        else {
            int sliderLength = this->fontMetrics().size(0, parameters->text).width() + 2;
            // The needle is at the right side, the text before it, 0.5 pixel
            // offset for exact pixelization
            parameters->rect = QRect((int)xBase + _preMargin - sliderLength + 1,
                                     0,            // Start at the top
                                     sliderLength, // The width depends on the text
                                     _sliderWidth  // Fill the whole height
                                     );
        }
    }

    return parameters->rect;
}

int LevelSlider::calculateWidth() {
    // At least 12 px for the needles
    _sliderWidth = 12;

    // Is it a vertical slider?
    if (_direction == Qt::RightArrow || _direction == Qt::LeftArrow) {
        for (auto &p : slider) {
            LevelSliderParameters *lp = p.second.get();
            int newSliderWidth = fontMetrics().size(0, lp->text).width();
            if (newSliderWidth > _sliderWidth) _sliderWidth = newSliderWidth;
        }
    }
    // Or a horizontal slider?
    else {
        for (auto &p : slider) {
            LevelSliderParameters *lp = p.second.get();
            int newSliderWidth = fontMetrics().size(0, lp->text).height();
            if (newSliderWidth > _sliderWidth) _sliderWidth = newSliderWidth;
        }
    }

    return _sliderWidth;
}

void LevelSlider::fixValue(LevelSliderParameters *parameters) {
    double lowest = std::min(parameters->minimum, parameters->maximum);
    double highest = std::max(parameters->minimum, parameters->maximum);
    if (parameters->value < lowest) {
        parameters->value = lowest;
    } else if (parameters->value > highest) {
        parameters->value = highest;
    }
}
