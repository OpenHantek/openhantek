// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QRectF>
#include <QString>

#include "hantekdso/devicesettings.h"
#include "hantekdso/enums.h"
#include "spectrum.h"
#include <vector>

class QSettings;
namespace Dso {
class ChannelUsage;
}

namespace Settings {

/// \brief Holds the settings for a graph channel, inluding some post processing capabilities.
class Channel : public QObject {
    Q_OBJECT
    friend struct ScopeIO;

  public:
    ~Channel();
    Channel(const Channel &) = default;
    static Channel *createReal(Dso::ChannelUsage *channelUsage, const Dso::Channel *channel, ChannelID channelID);
    enum { INVALID = UINT_MAX };

    /// Return true if voltage or spectrum is in use for this channel
    inline bool anyVisible() const { return visible() | m_spectrum.visible(); }
    virtual void setSpectrumVisible(bool visible);
    virtual void setVoltageVisible(bool visible);

    inline bool visible() const { return m_visible; }

    /// true if the channel is inverted (mirrored on cross-axis)
    inline bool inverted() const { return m_inverted; }
    void setInverted(bool e);

    /// Screen gain value. Defaults to 1.0. Do not confuse this with the hardware gain value
    /// to be found in devicespec->gain[channel->voltage()->gainStepIndex()].gain, which can be
    /// set by DsoControl->setGain(channelId, gainStepIndex).
    ///
    /// This is nothing more than a simple stretch factor, that is influenced by the voltage gain
    /// setup as well as optional probe dividers etc.
    inline float gain() const { return m_gain; }
    void setGain(float e);

    /// Name of this channel
    inline QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

    /// Return true if this is a math channel.
    inline bool isMathChannel() const { return m_isMathChannel; }

    /// Returns the channel ID. This is usually the position in the channel array, except for math
    /// channels that can be added/removed dynamically.
    inline unsigned channelID() const { return m_channelid; }

    inline Spectrum *spectrum() { return &m_spectrum; }
    inline const Spectrum *spectrum() const { return &m_spectrum; }
    inline const Dso::Channel *voltage() const { return m_voltage; }

  protected:
    /// Spectrum channel data
    Spectrum m_spectrum;
    /// Pointer to device channel (or artificial math channel) for voltage
    const Dso::Channel *m_voltage;
    Dso::ChannelUsage *m_channelUsage;
    QString m_name; ///< Name of this channel
    bool m_isMathChannel = false;
    bool m_inverted = false; ///< true if the channel is inverted (mirrored on cross-axis)
    bool m_visible = false;
    float m_gain = 1.0f;
    ChannelID m_channelid; ///< The channel id. This is usually just the position in the channel array.
    Channel() = default;
  signals:
    void visibleChanged(bool visible);
    void invertedChanged(bool inverted);
    void gainChanged(float gainValue);
};
}
