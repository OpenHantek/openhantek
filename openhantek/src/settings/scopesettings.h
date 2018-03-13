// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QObject>
#include <QRectF>

#include "hantekdso/enums.h"
#include "hantekprotocol/definitions.h"
#include "scopechannel.h"
#include "scopemathchannel.h"
#include "utils/getwithdefault.h"
#include <deque>
#include <memory>
#include <vector>

class QSettings;

namespace Settings {

/**
 * @brief Holds the settings for the oscilloscope.
 * Access this class object only from the main gui thread!
 */
class Scope : public QObject {
    Q_OBJECT
    friend struct ScopeIO;
    Scope(const Scope &) = delete;
    Scope(const Scope &&) = delete;

  public:
    Scope() = default;
    using ChannelMap = std::map<ChannelID, std::shared_ptr<Channel>>;
    using iterator = map_iterator<ChannelMap::iterator, false>;
    using const_iterator = map_iterator<ChannelMap::const_iterator, true>;
    using const_reverse_iterator = map_iterator<ChannelMap::const_reverse_iterator, true>;

    inline const std::shared_ptr<Channel> channel(ChannelID channel) const { return m_channels.at(channel); }
    inline std::shared_ptr<Channel> channel(ChannelID channel) { return m_channels[channel]; }

    inline iterator begin() { return make_map_iterator(m_channels.begin()); }
    inline iterator end() { return make_map_iterator(m_channels.end()); }

    inline const_iterator begin() const { return make_map_const_iterator(m_channels.begin()); }
    inline const_iterator end() const { return make_map_const_iterator(m_channels.end()); }

    inline const_reverse_iterator rbegin() const { return make_map_const_iterator(m_channels.rbegin()); }
    inline const_reverse_iterator rend() const { return make_map_const_iterator(m_channels.rend()); }

    inline const ChannelMap &channels() const { return m_channels; }

    MathChannel *addMathChannel(Dso::ChannelUsage *channelUsage, const Dso::DeviceSettings *deviceSettings);
    void removeMathChannel(ChannelID channelID);

    /// Graph drawing mode of the scope
    inline DsoE::GraphFormat format() const { return m_format; }
    void setFormat(DsoE::GraphFormat v);

    /// Frequencybase in Hz/div
    inline double frequencybase() const { return m_frequencybase; }
    void setFrequencybase(double v);

    /// If enabled, use only the natively supported hardware gain steps.
    /// Use predefined 200mV...5V non-native gain steps otherwise (which are mapped to the hardware gain steps)
    inline bool useHardwareGainSteps() { return m_useHardwareGain; }
    void setUseHardwareGainSteps(bool v);

  private:
    /// Settings for the channels of the graphs
    ChannelMap m_channels;

    DsoE::GraphFormat m_format = DsoE::GraphFormat::TY; ///< Graph drawing mode of the scope
    double m_frequencybase = 1e3;                     ///< Frequencybase in Hz/div
    bool m_useHardwareGain = false;
  signals:
    void formatChanged(const Scope *);
    void frequencybaseChanged(const Scope *);
    void mathChannelAdded(Settings::MathChannel *channel);
    void useHardwareGainChanged(bool useHardwareGainSteps);
};

struct ScopeIO {
    static void read(QSettings *io, Scope &scope, const Dso::DeviceSettings *deviceSpecification,
                     Dso::ChannelUsage *channelUsage);
    static void write(QSettings *io, const Scope &scope);
};
}
