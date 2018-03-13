// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "markerandzoomsettings.h"
#include "colorsettings.h"
#include "hantekdso/enums.h"
#include <QColor>
#include <QObject>

class QSettings;
namespace Settings {

/// \brief Holds all view settings.
class View : public QObject {
    Q_OBJECT
    friend struct ViewIO;
    View(const View &) = delete;
    View(const View &&) = delete;

  public:
    View() = default;
    Colors screen = {QColor(0xff, 0xff, 0xff, 0xff),       QColor(0xff, 0xff, 0xff, 0x7f),     // text, axes
                     QColor(0x00, 0x00, 0x00, 0xff),       QColor(0xff, 0xff, 0xff, 0xff),     // bg, border
                     QColor(0xff, 0xff, 0xff, 0x3f),                                           // grid
                     QColor(0xff, 0xff, 0xff, 0x0f),       QColor(0xff, 0xff, 0xff, 0xff),     // markers, markers hover
                     QColor(0xff, 0x00, 0x00, 0xff),       QColor(0xff, 0x00, 0x00, 0xff),     // markers sel+active
                     QColor::fromRgbF(0.3, 0.3, 0.3, 0.1), QColor::fromRgbF(0.1, 0.1, 0.1, 1), // zoom bg, zoom hover
                     QColor::fromRgbF(0.5, 0.1, 0.1, 1),   QColor::fromRgbF(0.1, 0.1, 0.1, 1)}; // zoom sel, zoom active
    Colors print = {QColor(0x00, 0x00, 0x00, 0xff),       QColor(0x00, 0x00, 0x00, 0xbf),       // text, axes
                    QColor(0xff, 0xff, 0xff, 0xff),       QColor(0x00, 0x00, 0x00, 0xff),       // bg, border
                    QColor(0x00, 0x00, 0x00, 0x7f),                                             // grid
                    QColor(0x00, 0x00, 0x00, 0xef),       QColor(0xff, 0x00, 0x00, 0x00),      // markers, markers hover
                    QColor(0xff, 0x00, 0x00, 0xff),       QColor(0xff, 0x00, 0x00, 0xff),      // markers sel+active
                    QColor::fromRgbF(0.7, 0.7, 0.7, 0.1), QColor::fromRgbF(0.9, 0.9, 0.9, 1),  // zoom bg, zoom hover
                    QColor::fromRgbF(0.5, 0.1, 0.1, 1),   QColor::fromRgbF(0.9, 0.9, 0.9, 1)}; // zoom sel, zoom active

    bool screenColorImages = false; ///< true exports images with screen colors
    ZoomViewSettings zoomviews;            ///< Settings for the zoomviews

    inline unsigned digitalPhosphorDraws() const { return m_digitalPhosphor ? m_digitalPhosphorDepth : 1; }
    inline bool digitalPhosphor() { return m_digitalPhosphor; }
    inline unsigned digitalPhosphorDepth() { return m_digitalPhosphorDepth; }
    void setDigitalPhosphor(bool enable, unsigned historyDepth);

    inline DsoE::InterpolationMode interpolation() const { return m_interpolation; }
    void setInterpolation(DsoE::InterpolationMode mode);

  private:
    DsoE::InterpolationMode m_interpolation = DsoE::InterpolationMode::LINEAR; ///< Interpolation mode for the graph
    bool m_digitalPhosphor = false;      ///< true slowly fades out the previous graphs
    unsigned m_digitalPhosphorDepth = 8; ///< Number of channels shown at one time
  signals:
    void interpolationChanged(View *view);
    void digitalPhosphorChanged(View *view);
};

class Scope;
struct ViewIO {
    static void read(QSettings *io, View &view, const Settings::Scope *scope);
    static void write(QSettings *io, const View &view);
    static void syncChannels(View &view, const Scope *scope);

  private:
    static void readColor(QSettings *store, Settings::Colors *colors, const Settings::Scope *scope);
    static void syncChannels(Settings::Colors &c, const Settings::Scope *scope);
};
}
