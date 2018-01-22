// SPDX-License-Identifier: GPL-2.0+

#include "post/softwaretrigger.h"
#include "channelusage.h"
#include "post/ppresult.h"
#include "scopesettings.h"
#include "utils/printutils.h"
#include "viewconstants.h"

namespace PostProcessing {
typedef bool (*opcmp)(double, double, double);
typedef bool (*smplcmp)(double, double);

static opcmp posOpcmp = [](double value, double level, double prev) { return value > level && prev <= level; };
static smplcmp posSmplcmp = [](double sampleK, double value) { return sampleK >= value; };

static opcmp negOpcmp = [](double value, double level, double prev) { return value < level && prev >= level; };
static smplcmp negSmplcmp = [](double sampleK, double value) { return sampleK < value; };

SoftwareTrigger::PrePostStartTriggerSamples SoftwareTrigger::compute(const PPresult *data,
                                                                     const Dso::DeviceSettings *control,
                                                                     const Settings::Scope *scope,
                                                                     const Dso::ChannelUsage *channelUsage) {
    unsigned int preTrigSamples = 0;
    unsigned int postTrigSamples = 0;
    unsigned int swTriggerStart = 0;
    ChannelID channel = control->trigger.source();

    // Trigger channel not in use
    if (!channelUsage->isUsed(channel) || !data->data(channel) || data->data(channel)->voltage.sample.empty())
        return PrePostStartTriggerSamples(preTrigSamples, postTrigSamples, swTriggerStart);

    const double gain = scope->channel(channel)->gain();
    const std::vector<double> &samples = data->data(channel)->voltage.sample;
    // Trigger level is in range [-1,1] -> map to [-DIV_VOLTAGE/2,DIV_VOLTAGE/2]
    double level = control->voltage[channel]->triggerLevel() * DIVS_VOLTAGE / 2;
    // The raw signal is in range [-1,1] as well but the user may have applied a gain factor other than 1V
    // and is using an offset. We need to compensate and adjust the level by the current gain factor and offset.
    level -= control->voltage[channel]->offset() * DIVS_VOLTAGE / 2;
    level *= gain;
    size_t sampleCount = samples.size();
    // Not the entire wave is visible at a time, only a DIVS_TIME part
    double timeDisplay = control->samplerate().timebase / DIVS_TIME;
    double samplesDisplay = timeDisplay * control->samplerate().samplerate;

    if (samplesDisplay >= sampleCount) {
        // For sure not enough samples to adjust for jitter.
        // Following options exist:
        //    1: Decrease sample rate
        //    2: Change trigger mode to auto
        //    3: Ignore samples
        // For now #3 is chosen
        timestampDebug(QString("Too few samples to make a steady picture. Decrease sample rate"));
        return PrePostStartTriggerSamples(preTrigSamples, postTrigSamples, swTriggerStart);
    }
    preTrigSamples = (unsigned)(control->trigger.position() * samplesDisplay * DIVS_TIME);
    postTrigSamples = (unsigned)sampleCount - ((unsigned)samplesDisplay - preTrigSamples);

    double prev;
    bool (*opcmp_)(double, double, double);
    bool (*smplcmp_)(double, double);

    if (control->trigger.slope() == Dso::Slope::Positive) {
        prev = INT_MAX;
        opcmp_ = posOpcmp;
        smplcmp_ = posSmplcmp;
    } else {
        prev = INT_MIN;
        opcmp_ = negOpcmp;
        smplcmp_ = negSmplcmp;
    }

    for (unsigned int i = preTrigSamples; i < postTrigSamples; i++) {
        const double value = samples[i];
        if (opcmp_(value, level, prev)) {
            unsigned rising = 0;
            for (unsigned int k = i + 1; k < i + control->trigger.swTriggerSampleSet() && k < sampleCount; k++) {
                if (smplcmp_(samples[k], value)) { rising++; }
            }
            if (rising > control->trigger.swTriggerThreshold()) {
                swTriggerStart = i;
                break;
            }
        }
        prev = value;
    }
    if (swTriggerStart == 0) {
        timestampDebug(QString("Trigger not asserted. Data ignored"));
        preTrigSamples = 0; // preTrigSamples may never be greater than swTriggerStart
        postTrigSamples = 0;
    }
    return PrePostStartTriggerSamples(preTrigSamples, postTrigSamples, swTriggerStart);
}
}
