// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QRectF>
#include <QString>

#include "post/enums.h"
#include "scopechannel.h"

class QSettings;

namespace Settings {

class MathChannel : public Channel {
    Q_OBJECT
    friend struct ScopeIO;

  public:
    static MathChannel *createMath(Dso::ChannelUsage *channelUsage, ChannelID channelID);

    virtual void setSpectrumVisible(bool visible) override;
    virtual void setVoltageVisible(bool visible) override;

    void setOffset(double offset);

    /// mode for math-channels
    inline ::PostProcessing::MathMode mathMode() const { return m_mode; }
    void setMathMode(::PostProcessing::MathMode e);

    inline ChannelID firstID() const { return m_first; }
    void setFirstChannel(ChannelID channel, const Dso::Channel *channelPointer);

    inline ChannelID secondID() const { return m_second; }
    void setSecondChannel(ChannelID channel, const Dso::Channel *channelPointer);

  private:
    ::PostProcessing::MathMode m_mode = ::PostProcessing::MathMode::ADD; ///< mode for math-channels
    ChannelID m_first = Channel::INVALID;                                ///< For storing/restoring
    ChannelID m_second = Channel::INVALID;                               ///< For storing/restoring
    const Dso::Channel *m_firstChannel = nullptr;                        ///< Pointer to real channel
    const Dso::Channel *m_secondChannel = nullptr;                       ///< Pointer to real channel

  signals:
    void mathModeChanged(const Channel *);
    void firstChannelChanged(ChannelID channelid);
    void secondChannelChanged(ChannelID channelid);
};
}
