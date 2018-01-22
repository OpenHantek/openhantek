#include "markerandzoomsettings.h"
namespace Settings {
void ZoomViewSettings::setActiveMarker(unsigned markerID) {
    if (!size() || !contains(markerID)) {
        m_activeMarker = -1;
    } else
        m_activeMarker = (int)markerID;
    emit activeMarkerChanged(m_activeMarker);
}

void ZoomViewSettings::removeMarker(int markerID) {
    if (markerID < 0) return;
    if (std::map<unsigned, MarkerAndZoom>::erase((unsigned)markerID)) {
        if ((int)markerID == m_activeMarker) { m_activeMarker = -1; }
        emit markerChanged(m_activeMarker);
    }
}
}
