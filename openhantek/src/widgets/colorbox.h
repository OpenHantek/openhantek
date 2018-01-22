// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QColor>
#include <QPushButton>

/// \brief A widget for the selection of a color.
class ColorBox : public QPushButton {
    Q_OBJECT

  public:
    /// \brief Initializes the widget.
    /// \param color Initial color value.
    /// \param parent The parent widget.
    ColorBox(QColor color, QWidget *parent = 0);

    /// \brief Get the current color.
    /// \return The current color as QColor.
    inline const QColor getColor() { return color; }

  public slots:
    /// \brief Sets the color.
    /// \param color The new color.
    void setColor(QColor newColor);

    /// \brief Wait for the color dialog and apply chosen color.
    void waitForColor();

  private:
    QColor color;

  signals:
    void colorChanged(QColor color); ///< The color has been changed
};
