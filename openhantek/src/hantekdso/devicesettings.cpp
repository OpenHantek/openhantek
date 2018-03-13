// SPDX-License-Identifier: GPL-2.0+

#include "devicesettings.h"
#include "hantekprotocol/definitions.h"

#include <QDebug>
#include <QSettings>

namespace Dso {

DeviceSettings::DeviceSettings(const ModelSpec *specification) : spec(specification) {
    this->limits = &specification->normalSamplerate;
    while (voltage.size() < specification->channels) voltage.push_back(new Channel);
}

DeviceSettings::~DeviceSettings() {
    for (Channel *c : voltage) delete c;
}

void DeviceSettings::setRecordLengthId(RecordLengthID value) {
    m_recordLengthId = value;
    emit recordLengthChanged(m_recordLengthId);
}

void DeviceSettings::updateCurrentSamplerate(double samplerate, double timebase, unsigned fixedSamplerateIndex) {
    m_samplerate.samplerate = samplerate;
    m_samplerate.timebase = timebase;
    m_samplerate.fixedSamperateId = fixedSamplerateIndex;
}

Samplerate &DeviceSettings::updateTarget(SamplerateSource source) {
    m_samplerateSource = source;
    return m_targetSamperate;
}

void Channel::setOffset(double offset, double offsetHardware) {
    m_offset = offset;
    m_offsetHardware = offsetHardware;
    emit offsetChanged(offset);
}

void Channel::setTriggerOffset(double offset) {
    m_triggerOffset = offset;
    emit triggerLevelChanged(offset);
}

void Channel::setGainStepIndex(unsigned gainId) {
    m_gainStepIndex = gainId;
    emit gainStepIndexChanged(gainId);
}

void Channel::setCouplingIndex(unsigned couplingId) {
    m_couplingIndex = couplingId;
    emit couplingIndexChanged(couplingId);
}

void Trigger::setPosition(double position) {
    this->m_position = position;
    emit positionChanged(position);
}

void Trigger::setTriggerSource(ChannelID channel, bool specialChannel) {
    m_source = channel;
    m_special = specialChannel;
    emit sourceChanged(specialChannel, channel);
}

void Trigger::setSlope(Slope slope) {
    this->m_slope = slope;
    emit slopeChanged(slope);
}

void Trigger::setMode(TriggerMode mode) {
    this->m_mode = mode;
    emit modeChanged(mode);
}
}

void Settings::DeviceSettingsIO::read(QSettings *io, Dso::DeviceSettings &control) {
    control.m_recordLengthId = std::min(io->value("recordLengthId", control.m_recordLengthId).toUInt(),
                                        (unsigned)control.spec->normalSamplerate.recordLengths.size() - 1);
    Dso::Samplerate &localUpdateTarget = control.updateTarget(
        (Dso::SamplerateSource)io->value("samplerateSource", (unsigned)control.samplerateSource()).toUInt());
    localUpdateTarget.fixedSamperateId = io->value("fixedSamperateId", localUpdateTarget.fixedSamperateId).toUInt();
    localUpdateTarget.samplerate = io->value("samplerate", localUpdateTarget.samplerate).toDouble();
    localUpdateTarget.timebase = io->value("timebase", localUpdateTarget.timebase).toDouble();

    control.trigger.m_mode = (Dso::TriggerMode)io->value("trigger.mode", (unsigned)control.trigger.m_mode).toUInt();
    control.trigger.m_slope = (Dso::Slope)io->value("trigger.slope", (unsigned)control.trigger.m_slope).toUInt();
    control.trigger.m_position = io->value("trigger.position", control.trigger.m_position).toDouble();
    control.trigger.m_point = io->value("trigger.point", control.trigger.m_point).toUInt();
    control.trigger.m_source = io->value("trigger.source", control.trigger.m_source).toUInt();
    control.trigger.m_swTriggerThreshold =
        io->value("trigger.swTriggerThreshold", control.trigger.m_swTriggerThreshold).toUInt();
    control.trigger.m_swTriggerSampleSet =
        io->value("trigger.swTriggerSampleSet", control.trigger.m_swTriggerSampleSet).toUInt();
    control.trigger.m_swSampleMargin = io->value("trigger.swSampleMargin", control.trigger.m_swSampleMargin).toUInt();
    control.trigger.m_special = io->value("trigger.special", control.trigger.m_special).toBool();

    for (unsigned i = 0; i < control.voltage.size(); ++i) {
        io->beginGroup("channel" + QString::number(i));
        Dso::Channel *chan = control.voltage[i];
        chan->m_couplingIndex = io->value("couplingIndex", chan->couplingIndex()).toUInt();
        chan->m_gainStepIndex =
            std::min(io->value("gainId", chan->gainStepIndex()).toUInt(), (unsigned)control.spec->gain.size() - 1);
        chan->m_offset = io->value("offset", chan->offset()).toDouble();
        chan->m_offsetHardware = io->value("offsetReal", chan->offsetHardware()).toDouble();
        chan->m_triggerOffset = io->value("triggerLevel", chan->triggerLevel()).toDouble();
        io->endGroup();
    }
}

void Settings::DeviceSettingsIO::write(QSettings *io, const Dso::DeviceSettings &control) {
    io->setValue("recordLengthId", control.m_recordLengthId);
    io->setValue("samplerateSource", (unsigned)control.samplerateSource());
    io->setValue("fixedSamperateId", control.target().fixedSamperateId);
    io->setValue("samplerate", control.target().samplerate);
    io->setValue("timebase", control.target().timebase);

    io->setValue("trigger.mode", (unsigned)control.trigger.m_mode);
    io->setValue("trigger.slope", (unsigned)control.trigger.m_slope);
    io->setValue("trigger.position", control.trigger.m_position);
    io->setValue("trigger.point", control.trigger.m_point);
    io->setValue("trigger.source", control.trigger.m_source);
    io->setValue("trigger.swTriggerThreshold", control.trigger.m_swTriggerThreshold);
    io->setValue("trigger.swTriggerSampleSet", control.trigger.m_swTriggerSampleSet);
    io->setValue("trigger.swSampleMargin", control.trigger.m_swSampleMargin);
    io->setValue("trigger.special", control.trigger.m_special);

    for (unsigned i = 0; i < control.voltage.size(); ++i) {
        io->beginGroup("channel" + QString::number(i));
        const Dso::Channel *chan = control.voltage[i];
        io->setValue("couplingIndex", chan->couplingIndex());
        io->setValue("gainId", chan->gainStepIndex());
        io->setValue("offset", chan->offset());
        io->setValue("offsetReal", chan->offsetHardware());
        io->setValue("triggerLevel", chan->triggerLevel());
        io->endGroup();
    }
}
