// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QWidget>

#include <memory>

class QColor;

/// \brief Contains the color, text and value of one slider.
struct LevelSliderParameters {
    QColor color; ///< The color of the slider and font
    QString text; ///< The text beside the slider, a empty string disables text
    bool visible; ///< Visibility of the slider

    double minimum; ///< Minimum (left/top) value for the slider
    double maximum; ///< Maximum (right/bottom) value for the slider
    double step;    ///< The distance between selectable slider positions
    double value;   ///< The current value of the slider

    // Needed for moving and drawing
    QRect rect; ///< The area where the slider is drawn
};

/// \brief Slider widget for multiple level sliders.
/// These are used for the trigger levels, offsets and so on.
class LevelSlider : public QWidget {
    Q_OBJECT

  public:
    typedef int IndexType;
    static const IndexType INVALID = INT_MAX;

    /// \brief Initializes the slider container.
    /// \param direction The side on which the sliders are shown.
    /// \param parent The parent widget.
    LevelSlider(Qt::ArrowType direction = Qt::RightArrow, QWidget *parent = 0);

    /// \brief Size hint for the widget.
    /// \return The recommended size for the widget.
    QSize sizeHint() const;

    /// \brief Return the margin before the slider.
    /// \return The margin the Slider has at the top/left.
    int preMargin() const;
    /// \brief Return the margin after the slider.
    /// \return The margin the Slider has at the bottom/right.
    int postMargin() const;

    /// \brief Add a new slider to the slider container.
    /// \param index The index where the slider should be inserted, 0 to append.
    /// \param text The text that will be shown next to the slider.
    /// \return The index of the slider, INVALID on error.
    IndexType addSlider(IndexType index = INVALID, const QString &text = "");

    /// \brief Remove a slider from the slider container.
    /// \param index The index of the slider that should be removed.
    void removeSlider(IndexType index);

    /// Remove all sliders
    void removeAll();

    /// Return the highest index. Only call this if there is at least one slider added
    inline IndexType lastItemIndex() { return slider.rbegin()->first; }

    /// Return the highest+1 index, designated for a new item for example. Return 1 if there is no slider yet.
    inline IndexType newItemIndex() { return slider.size() ? slider.rbegin()->first + 1 : 1; }

    /// \brief Return the color of a slider.
    /// \param index The index of the slider whose color should be returned.
    /// \return The current color of the slider.
    const QColor color(IndexType index) const;

    /// \brief Set the color of the slider.
    /// \param index The index of the slider whose color should be set.
    /// \param color The new color for the slider.
    /// \return The index of the slider, -1 on error.
    void setColor(IndexType index, QColor color);

    /// \brief Return the text shown beside a slider.
    /// \param index The index of the slider whose text should be returned.
    /// \return The current text of the slider.
    const QString text(IndexType index) const;

    /// \brief Set the text for a slider.
    /// \param index The index of the slider whose text should be set.
    /// \param text The text shown next to the slider.
    /// \return The index of the slider, -1 on error.
    void setText(IndexType index, const QString &text);

    /// \brief Return the visibility of a slider.
    /// \param index The index of the slider whose visibility should be returned.
    /// \return true if the slider is visible, false if it's hidden.
    bool visible(IndexType index) const;

    /// \brief Set the visibility of a slider.
    /// \param index The index of the slider whose visibility should be set.
    /// \param visible true to show the slider, false to hide it.
    /// \return The index of the slider, -1 on error.
    void setIndexVisible(IndexType index, bool visible);

    /// \brief Return the minimal value of the sliders.
    /// \return The value a slider has at the bottommost/leftmost position.
    double minimum(IndexType index) const;

    /// \brief Return the maximal value of the sliders.
    /// \return The value a slider has at the topmost/rightmost position.
    double maximum(IndexType index) const;

    /// \brief Set the maximal value of the sliders.
    /// \param index The index of the slider whose limits should be set.
    /// \param minimum The value a slider has at the bottommost/leftmost position.
    /// \param maximum The value a slider has at the topmost/rightmost position.
    /// \return -1 on error, fixValue result on success.
    void setLimits(IndexType index, double minimum, double maximum);

    /// \brief Return the step width of the sliders.
    /// \param index The index of the slider whose step width should be returned.
    /// \return The distance between the selectable slider positions.
    double step(IndexType index) const;

    /// \brief Set the step width of the sliders.
    /// \param index The index of the slider whose step width should be set.
    /// \param step The distance between the selectable slider positions.
    /// \return The new step width.
    double setStep(IndexType index, double step);

    /// \brief Return the current position of a slider.
    /// \param index The index of the slider whose value should be returned.
    /// \return The value of the slider.
    double value(IndexType index) const;

    /// \brief Set the current position of a slider.
    /// \param index The index of the slider whose value should be set.
    /// \param value The new value of the slider.
    /// \return The new value of the slider.
    void setValue(IndexType index, double value);

    /// \brief Return the direction of the sliders.
    /// \return The side on which the sliders are shown.
    Qt::ArrowType direction() const;

    /// \brief Set the direction of the sliders.
    /// \param direction The side on which the sliders are shown.
    /// \return The index of the direction, -1 on error.
    void setDirection(Qt::ArrowType direction);

  protected:
    /// \brief Move the slider if it's pressed.
    /// \param event The mouse event that should be handled.
    void mouseMoveEvent(QMouseEvent *event);

    /// \brief Prepare slider for movement if the left mouse button is pressed.
    /// \param event The mouse event that should be handled.
    void mousePressEvent(QMouseEvent *event);

    /// \brief Movement is done if the left mouse button is released.
    /// \param event The mouse event that should be handled.
    void mouseReleaseEvent(QMouseEvent *event);

    /// \brief Paint the widget.
    /// \param event The paint event that should be handled.
    void paintEvent(QPaintEvent *event);

    /// \brief Resize the widget and adapt the slider positions.
    /// \param event The resize event that should be handled.
    void resizeEvent(QResizeEvent *event);

    /// \brief Calculate the drawing area for the slider for it's current value.
    /// \param sliderId The id of the slider whose rect should be calculated.
    /// \return The calculated rect.
    QRect calculateRect(LevelSliderParameters *parameters);

    /// \brief Search for the widest slider element.
    /// \return The calculated width of the slider.
    int calculateWidth();

    /// \brief Fix the value if it's outside the limits.
    /// \param index The index of the slider who should be fixed.
    /// \return 0 when ok, -1 on error, 1 when increased and 2 when decreased.
    void fixValue(LevelSliderParameters *parameters);

    void setValue(LevelSliderParameters *parameter, double value);

    std::map<IndexType, std::unique_ptr<LevelSliderParameters>> slider;   ///< The parameters for each slider
    IndexType _pressedSlider = INVALID;                    ///< The currently pressed (moved) slider
    LevelSliderParameters *_pressedSliderParams = nullptr; ///< The currently pressed (moved) slider
    int _sliderWidth;                                      ///< The slider width (dimension orthogonal to the sliding
                                                           /// direction)

    Qt::ArrowType _direction; ///< The direction the sliders point to
    int _preMargin;           ///< The margin before the minimum slider position
    int _postMargin;          ///< The margin after the maximum slider position

  signals:
    void valueChanged(IndexType index, double value); ///< The value of a slider has changed
};
