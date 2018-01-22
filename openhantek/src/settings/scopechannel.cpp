#include "scopechannel.h"
#include "hantekdso/channelusage.h"
#include "scopemathchannel.h"

Settings::Channel::~Channel() {
    if (isMathChannel()) delete m_voltage;
}

Settings::Channel *Settings::Channel::createReal(Dso::ChannelUsage *channelUsage, const Dso::Channel *channel,
                                                 ChannelID channelID) {
    Channel *v = new Channel;
    v->m_voltage = channel;
    v->m_channelUsage = channelUsage;
    v->m_channelid = channelID;
    return v;
}

Settings::MathChannel *Settings::MathChannel::createMath(Dso::ChannelUsage *channelUsage, ChannelID channelID) {
    MathChannel *v = new MathChannel;
    v->m_voltage = new Dso::Channel();
    v->m_isMathChannel = true;
    v->m_channelid = channelID;
    v->m_channelUsage = channelUsage;
    return v;
}

void Settings::MathChannel::setSpectrumVisible(bool visible) {
    m_spectrum.m_visible = visible && m_firstChannel && m_secondChannel;
    emit spectrum()->visibleChanged(m_spectrum.m_visible);

    if (m_spectrum.m_visible) {
        m_channelUsage->addChannelUser(m_first, spectrum());
        m_channelUsage->addChannelUser(m_second, spectrum());
    } else {
        if (Q_LIKELY(m_firstChannel)) m_channelUsage->removeChannelUser(m_first, spectrum());
        if (Q_LIKELY(m_secondChannel)) m_channelUsage->removeChannelUser(m_second, spectrum());
    }
}

void Settings::MathChannel::setVoltageVisible(bool visible) {
    m_visible = visible && m_firstChannel && m_secondChannel;
    emit visibleChanged(m_visible);

    if (m_visible) {
        m_channelUsage->addChannelUser(m_first, this);
        m_channelUsage->addChannelUser(m_second, this);
    } else {
        if (Q_LIKELY(m_firstChannel)) m_channelUsage->removeChannelUser(m_first, this);
        if (Q_LIKELY(m_secondChannel)) m_channelUsage->removeChannelUser(m_second, this);
    }
}

void Settings::MathChannel::setOffset(double offset) { const_cast<Dso::Channel *>(m_voltage)->setOffset(offset); }

void Settings::MathChannel::setMathMode(::PostProcessing::MathMode e) {
    m_mode = e;
    emit mathModeChanged(this);
}

void Settings::MathChannel::setFirstChannel(ChannelID channel, const Dso::Channel *channelPointer) {
    m_first = channel;
    m_firstChannel = channelPointer;
    emit firstChannelChanged(channel);
}

void Settings::MathChannel::setSecondChannel(ChannelID channel, const Dso::Channel *channelPointer) {
    m_second = channel;
    m_secondChannel = channelPointer;
    emit secondChannelChanged(channel);
}

void Settings::Channel::setInverted(bool e) {
    m_inverted = e;
    emit invertedChanged(m_inverted);
}

void Settings::Channel::setGain(float e) {
    m_gain = e;
    emit gainChanged(m_gain);
}

void Settings::Channel::setSpectrumVisible(bool visible) {
    m_spectrum.m_visible = visible;
    emit spectrum()->visibleChanged(m_spectrum.m_visible);

    if (visible)
        m_channelUsage->addChannelUser(m_channelid, spectrum());
    else
        m_channelUsage->removeChannelUser(m_channelid, spectrum());
}

void Settings::Channel::setVoltageVisible(bool visible) {
    m_visible = visible;
    emit visibleChanged(m_visible);

    if (visible)
        m_channelUsage->addChannelUser(m_channelid, this);
    else
        m_channelUsage->removeChannelUser(m_channelid, this);
}
