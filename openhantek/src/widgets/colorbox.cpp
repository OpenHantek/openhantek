// SPDX-License-Identifier: GPL-2.0+

#include <QColorDialog>
#include <QFocusEvent>
#include <QPushButton>

#include "colorbox.h"

ColorBox::ColorBox(QColor color, QWidget *parent) : QPushButton(parent) {
    setColor(color);
    connect(this, &QAbstractButton::clicked, this, &ColorBox::waitForColor);
    QFont f = font();
    f.setStyleHint(QFont::Monospace);
    setFont(f);
}

void ColorBox::setColor(QColor newColor) {
    color = newColor;
    if (color.alphaF() >= 1.0) {
        setText(QString("#%1").arg(color.rgb() & 0xffffff, 6, 16, QChar('0')));
    } else {
        setText(QString("#%1/%2").arg(color.rgb() & 0xffffff, 6, 16, QChar('0')).arg(color.alpha(), 2, 16, QChar('0')));
    }
    setPalette(QPalette(color));

    emit colorChanged(color);
}

void ColorBox::waitForColor() {
    setFocus();
    setDown(true);

    QColor newColor = QColorDialog::getColor(color, this, 0, QColorDialog::ShowAlphaChannel);
    if (newColor.isValid()) setColor(newColor);
}
