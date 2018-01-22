// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QObject>
#include <QRectF>
#include <QString>

namespace Settings {
/// \brief Holds the settings for the spectrum analysis post processing.
class Spectrum : public QObject {
    Q_OBJECT
    friend struct ScopeIO;
    friend class Channel;
    friend class MathChannel;

  public:
    void setName(const QString &name) { m_name = name; }
    inline const QString &name() const { return m_name; }

    void setMagnitude(double v);
    inline double magnitude() const { return m_magnitude; }

    void setOffset(double v);
    inline double offset() const { return m_offset; }

    inline bool visible() const { return m_visible; }

  protected:
    QString m_name;            ///< Name of this channel
    double m_magnitude = 20.0; ///< The vertical resolution in dB/div
    double m_offset = 0.0;     ///< Vertical offset in divs
    bool m_visible = false;    ///< true if the spectrum is turned on
  signals:
    void magnitudeChanged(const Spectrum *);
    void offsetChanged(const Spectrum *);
    void visibleChanged(bool visible);
};
}
