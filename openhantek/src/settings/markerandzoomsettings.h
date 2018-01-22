// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QColor>
#include <QObject>
#include <QRectF>

namespace Settings {
struct MarkerAndZoom {
    QRectF zoomRect = QRectF();
    QRectF markerRect = QRectF();
    bool visible = true;
};

/**
 * A zoom view needs a position and geometry for the zoom view window as well as for
 * the zoomed area. This is stored together as MarkerAndZoom. ZoomViews stores those MarkerAndZooms
 * and notifies if new ones are added or if one is removed.
 */
class ZoomViewSettings : public QObject, private std::map<unsigned, MarkerAndZoom> {
    Q_OBJECT
  public:
    ZoomViewSettings() = default;
    ZoomViewSettings(const ZoomViewSettings &) = delete;
    ZoomViewSettings(const ZoomViewSettings &&) = delete;

    inline MarkerAndZoom &get(unsigned markerID) { return find(markerID)->second; }
    inline const MarkerAndZoom &get(unsigned markerID) const { return find(markerID)->second; }
    inline void insert(unsigned markerID, MarkerAndZoom &z) {
        std::map<unsigned, MarkerAndZoom>::insert(std::make_pair(markerID, z));
        emit markerChanged(m_activeMarker);
    }
    inline void eraseNoNotify(unsigned markerID) { std::map<unsigned, MarkerAndZoom>::erase(markerID); }
    inline bool contains(unsigned markerID) { return find(markerID) != end(); }
    inline size_t size() const { return std::map<unsigned, MarkerAndZoom>::size(); }
    inline iterator begin() { return std::map<unsigned, MarkerAndZoom>::begin(); }
    inline iterator end() { return std::map<unsigned, MarkerAndZoom>::end(); }
    inline const_iterator begin() const { return std::map<unsigned, MarkerAndZoom>::begin(); }
    inline const_iterator end() const { return std::map<unsigned, MarkerAndZoom>::end(); }

    typedef std::pair<const unsigned, MarkerAndZoom> value_type;

    /// There is always one of the zoomviews that is marked as actived. The GUI will show detailed information
    /// about an active zoom view.
    void setActiveMarker(unsigned markerID);
    void removeMarker(int markerID);
    inline int activeMarker() const { return m_activeMarker; }
    /// Call this method if the geometry of the current active marker changed
    inline void notifyDataChanged() { emit markerDataChanged(m_activeMarker); }

  private:
    int m_activeMarker = 0;
  signals:
    /// This signal is emitted whenever a marker/zoomview is added or removed. The (new) active marker is given.
    void markerChanged(int activeMarker);
    /// Emitted as soon as the active marker changes or no marker is selected (activeMarker==-1)
    void activeMarkerChanged(int activeMarker);
    /// Listen to this signal to be notifed if the currently active marker/zoomview changed its geometry
    void markerDataChanged(int activeMarker);
};
}
