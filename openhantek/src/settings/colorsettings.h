// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QColor>
#include <vector>

#include "hantekprotocol/types.h"
#include "utils/getwithdefault.h"
#include "utils/observer.h"

class DsoConfigColorsPage;

namespace Settings {
/// \brief Holds the color values for the oscilloscope screen.
struct Colors {
    // The settings configuration page and settings-reader get full access
    friend class ::DsoConfigColorsPage;
    friend struct ViewIO;

    Colors() = default;
    Colors(const QColor &text, const QColor &axes, const QColor &background, const QColor &border, const QColor &grid,
           const QColor &markers, const QColor &markerHover, const QColor &markerSelected,
           const QColor &markerActive, // marker colors
           const QColor &zoomBackground, const QColor &zoomHover, const QColor &zoomSelected, const QColor &zoomActive)
        : _text(text), _axes(axes), _background(background), _border(border), _grid(grid), // everything else
          _markers(markers), _markerHover(markerHover), _markerSelected(markerSelected),
          _markerActive(markerActive), // markers
          _zoomBackground(zoomBackground), _zoomHover(zoomHover), _zoomSelected(zoomSelected), _zoomActive(zoomActive),
          _observer(this) {}

    inline const QColor &text() const { return _text; }
    inline const QColor &axes() const { return _axes; }
    inline const QColor &background() const { return _background; }
    inline const QColor &border() const { return _border; }
    inline const QColor &grid() const { return _grid; }
    inline const QColor &markers() const { return _markers; }
    inline const QColor &markerHover() const { return _markerHover; }
    inline const QColor &markerSelected() const { return _markerSelected; }
    inline const QColor &markerActive() const { return _markerActive; }
    inline const QColor &zoomBackground() const { return _zoomBackground; }
    inline const QColor &zoomHover() const { return _zoomHover; }
    inline const QColor &zoomSelected() const { return _zoomSelected; }
    inline const QColor &zoomActive() const { return _zoomActive; }
    inline const QColor &spectrum(ChannelID channelID) const {
        return GetWithDef(_spectrum, channelID, _spectrum.at(0));
    }
    inline const QColor &voltage(ChannelID channelID) const { return GetWithDef(_voltage, channelID, _voltage.at(0)); }
    inline void setVoltage(ChannelID channelID, const QColor &color) { _voltage[channelID] = color; }
    inline void setSpectrum(ChannelID channelID, const QColor &color) { _spectrum[channelID] = color; }
    inline Observer *observer() const { return &_observer; }

  protected:
    QColor _text;           ///< The default text color
    QColor _axes;           ///< X- and Y-axis and subdiv lines on them
    QColor _background;     ///< The scope clear color
    QColor _border;         ///< The border of the scope screen
    QColor _grid;           ///< The color of the grid
    QColor _markers;        ///< The color of the markers
    QColor _markerHover;    ///< The color of a hovered marker
    QColor _markerSelected; ///< The color of a selected marker
    QColor _markerActive;   ///< The color of an active marker
    QColor _zoomBackground; ///< A zoomview has a seperate background layer
    QColor _zoomHover;      ///< The color of a hovered zoomview
    QColor _zoomSelected;   ///< When the mouse is pressed down to move/resize a zoomview this color is used
    QColor _zoomActive;     ///< There can only be one zoomview marked as active, this is the color
    std::map<ChannelID, QColor> _spectrum; ///< The colors of the spectrum graphs
    std::map<ChannelID, QColor> _voltage;  ///< The colors of the voltage graphs
    mutable Observer _observer;
};
}
