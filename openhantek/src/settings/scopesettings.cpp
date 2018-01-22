// SPDX-License-Identifier: GPL-2.0+

#include <QApplication>
#include <QColor>
#include <QDebug>
#include <QSettings>

#include "scopesettings.h"
#include "utils/enumhelper.h"

void Settings::ScopeIO::read(QSettings *store, Scope &scope, const Dso::DeviceSettings *deviceSpecification,
                             Dso::ChannelUsage *channelUsage) {
    scope.m_channels.clear();
    // Add new channels to the list
    for (ChannelID i = 0; i < deviceSpecification->voltage.size(); ++i) {
        Channel *newVoltage = Channel::createReal(channelUsage, deviceSpecification->voltage[i], i);
        newVoltage->m_name = QApplication::tr("CH%1").arg(scope.m_channels.size() + 1);
        newVoltage->spectrum()->m_name = QApplication::tr("SP%1").arg(scope.m_channels.size() + 1);
        scope.m_channels[i] = std::shared_ptr<Channel>(newVoltage);
    }

    // Oscilloscope settings
    store->beginGroup("scope");
    scope.m_format = loadForEnum(store, "format", scope.format());
    scope.m_frequencybase = store->value("frequencybase", scope.frequencybase()).toDouble();

    // Vertical axis
    unsigned channelCount = (unsigned)store->beginReadArray("channels");
    for (ChannelID channel = 0; channel < channelCount; ++channel) {
        store->setArrayIndex((int)channel);
        auto c = scope.m_channels[channel];
        if (!c) {
            c = std::shared_ptr<Channel>(MathChannel::createMath(channelUsage, channel));
            scope.m_channels[channel] = c;
        }
        store->beginGroup("spectrum");
        c->spectrum()->m_name = store->value("name", c->spectrum()->name()).toString();
        c->spectrum()->m_magnitude = store->value("magnitude", c->spectrum()->magnitude()).toDouble();
        c->spectrum()->m_offset = store->value("offset", c->spectrum()->offset()).toDouble();
        c->spectrum()->m_visible = store->value("used", c->spectrum()->visible()).toBool();
        store->endGroup();
        store->beginGroup("voltage");
        c->m_name = store->value("name", c->name()).toString();
        c->m_visible = store->value("used", c->m_visible).toBool();
        c->m_inverted = store->value("inverted", c->m_inverted).toBool();
        if (c->isMathChannel()) {
            MathChannel *mc = ((MathChannel *)c.get());
            mc->m_mode = loadForEnum(store, "mathMode", mc->m_mode);
            mc->m_first = store->value("first", mc->m_first).toUInt();
            mc->m_second = store->value("second", mc->m_second).toUInt();
            if (mc->m_first >= deviceSpecification->voltage.size()) mc->m_first = 0;
            if (mc->m_second >= deviceSpecification->voltage.size()) mc->m_second = 1;
            mc->m_firstChannel = deviceSpecification->voltage[mc->m_first];
            mc->m_secondChannel = deviceSpecification->voltage[mc->m_second];
        }
        store->endGroup();
    }
    store->endArray();
    store->endGroup(); // end "scope"
}

void Settings::ScopeIO::write(QSettings *store, const Scope &scope) {
    // Oszilloskope settings
    store->beginGroup("scope");
    store->setValue("format", enumName(scope.format()));
    store->setValue("frequencybase", scope.frequencybase());

    // Vertical axis
    store->beginWriteArray("channels", (int)scope.m_channels.size());
    int newChannelIndex = 0;
    for (const Channel *channel : scope) {
        store->setArrayIndex(newChannelIndex);

        store->beginGroup("spectrum");
        store->setValue("magnitude", channel->spectrum()->magnitude());
        store->setValue("offset", channel->spectrum()->offset());
        store->setValue("used", channel->spectrum()->visible());
        store->setValue("name", channel->spectrum()->name());
        store->endGroup();

        store->beginGroup("voltage");
        store->setValue("name", channel->name());
        store->setValue("used", channel->visible());
        store->setValue("inverted", channel->m_inverted);
        if (channel->m_isMathChannel) {
            MathChannel *mchannel = ((MathChannel *)channel);
            store->setValue("mathMode", enumName(mchannel->m_mode));
            store->setValue("first", mchannel->m_first);
            store->setValue("second", mchannel->m_second);
        }
        store->endGroup();

        ++newChannelIndex;
    }
    store->endArray();

    store->endGroup();
}

Settings::MathChannel *Settings::Scope::addMathChannel(Dso::ChannelUsage *channelUsage,
                                                       const Dso::DeviceSettings *deviceSettings) {
    ChannelID highest = m_channels.size() ? m_channels.rbegin()->first : 0;
    MathChannel *m = MathChannel::createMath(channelUsage, highest + 1);
    m->setName(tr("Math %1").arg(highest));
    m->setFirstChannel(0, deviceSettings->voltage[0]);
    m->setSecondChannel(1, deviceSettings->voltage[1]);
    m_channels[m->channelID()] = std::shared_ptr<Channel>(m);
    emit mathChannelAdded(m);
    return m;
}

void Settings::Scope::removeMathChannel(ChannelID channelID) {
    for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
        if (it->first == channelID) {
            it->second->setVoltageVisible(false);
            it->second->setSpectrumVisible(false);
            m_channels.erase(it);
            break;
        }
    }
}

void Settings::Scope::setFormat(Dso::GraphFormat v) {
    m_format = v;
    emit formatChanged(this);
}

void Settings::Scope::setFrequencybase(double v) {
    m_frequencybase = v;
    emit frequencybaseChanged(this);
}

void Settings::Scope::setUseHardwareGainSteps(bool v) {
    m_useHardwareGain = v;
    emit useHardwareGainChanged(v);
}

void Settings::Spectrum::setMagnitude(double v) {
    m_magnitude = v;
    emit magnitudeChanged(this);
}

void Settings::Spectrum::setOffset(double v) {
    m_offset = v;
    emit offsetChanged(this);
}
