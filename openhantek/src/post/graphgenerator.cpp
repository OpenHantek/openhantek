// SPDX-License-Identifier: GPL-2.0+

#include <QDebug>
#include <QMutex>
#include <exception>

#include "hantekdso/modelspecification.h"
#include "post/graphgenerator.h"
#include "post/ppresult.h"
#include "post/softwaretrigger.h"
#include "scopesettings.h"
#include "utils/printutils.h"
#include "viewconstants.h"

namespace PostProcessing {

GraphGenerator::GraphGenerator(const ::Settings::Scope *scope, const Dso::DeviceSettings *deviceSettings,
                               const Dso::ChannelUsage *channelUsage)
    : m_scope(scope), m_deviceSettings(deviceSettings), m_channelUsage(channelUsage) {}

void GraphGenerator::generateGraphsTYvoltage(PPresult *result) {
    unsigned preTrigSamples = 0;
    unsigned postTrigSamples = 0;
    unsigned swTriggerStart = 0;

    // check trigger point for software trigger
    if (m_deviceSettings->spec->isSoftwareTriggerDevice &&
        m_deviceSettings->trigger.source() < m_deviceSettings->voltage.size())
        std::tie(preTrigSamples, postTrigSamples, swTriggerStart) =
            SoftwareTrigger::compute(result, m_deviceSettings, m_scope, m_channelUsage);
    result->softwareTriggerTriggered = postTrigSamples > preTrigSamples;

    for (DataChannel &channelData : *result) {
        ChannelGraph &target = channelData.voltage.graph;
        const std::vector<double> &source = channelData.voltage.sample;

        // Check if this channel is used and available at the data analyzer
        if (source.empty() || !channelData.channelSettings->visible()) {
            // Delete all vector arrays
            target.clear();
            continue;
        }
        // Check if the sample count has changed
        size_t sampleCount = source.size();
        if (sampleCount > 500000) {
            qWarning() << "Sample count too high!";
            throw new std::runtime_error("Sample count too high!");
        }

        const unsigned offSamples = unsigned(swTriggerStart - preTrigSamples);
        sampleCount -= offSamples;

        // Set size directly to avoid reallocations
        target.resize(sampleCount);

        // Data samples are in volts (as long as the voltageLimits are set correctly).
        // The offset needs to be applied now, as well as the gain.
        const float timeFactor =
            float(channelData.voltage.interval / m_deviceSettings->samplerate().timebase);
        const float offY = (float)channelData.channelSettings->voltage()->offset() * DIVS_VOLTAGE / 2;
        const float offX = -DIVS_TIME / 2;
        const int invert = channelData.channelSettings->inverted() ? -1.0 : 1.0;
        const float gain = invert / channelData.channelSettings->gain() * DIVS_VOLTAGE;

#pragma omp parallel for
        for (unsigned int position = 0; position < sampleCount; ++position) {
            const float v = (float)source[position + offSamples];
            target[position] = (QVector3D(position * timeFactor + offX, v * gain + offY, 0.0));
        }
    }
}

void GraphGenerator::generateGraphsTYspectrum(PPresult *result) {
    for (DataChannel &channelData : *result) {
        ChannelGraph &target = channelData.spectrum.graph;
        const std::vector<double> &source = channelData.spectrum.sample;

        // Check if this channel is used and available at the data analyzer
        if (source.empty()) {
            // Delete all vector arrays
            target.clear();
            continue;
        }
        // Check if the sample count has changed
        size_t sampleCount = source.size();
        if (sampleCount > 500000) {
            qWarning() << "Sample count too high!";
            throw new std::runtime_error("Sample count too high!");
        }

        // Set size directly to avoid reallocations
        target.resize(sampleCount);

        // What's the horizontal distance between sampling points?
        const float timeFactor = (float)(channelData.spectrum.interval / m_scope->frequencybase());

        const float magnitude = (float)channelData.channelSettings->spectrum()->magnitude();
        const float offY = (float)channelData.channelSettings->spectrum()->offset() * DIVS_VOLTAGE / 2;
        const float offX = -DIVS_TIME / 2;

#pragma omp parallel for
        for (unsigned int position = 0; position < sampleCount; ++position) {
            const float v = (float)source[position];
            target[position] = (QVector3D(position * timeFactor + offX, v / magnitude + offY, 0.0));
        }
    }
}

void GraphGenerator::process(PPresult *data) {
    if (m_scope->format() == DsoE::GraphFormat::TY) {
        generateGraphsTYspectrum(data);
        generateGraphsTYvoltage(data);
    } else
        generateGraphsXY(data);
}

void GraphGenerator::generateGraphsXY(PPresult *result) {
    DataChannel *lastChannel = nullptr;

    for (DataChannel &channelData : *result) {
        // Delete all spectrum graphs
        channelData.spectrum.graph.clear();
        channelData.voltage.graph.clear();

        // Generate voltage graphs for pairs of channels
        if (!lastChannel) {
            lastChannel = &channelData;
            continue;
        }

        DataChannel *thisChannel = &channelData;

        ChannelGraph &target = lastChannel->voltage.graph;
        const std::vector<double> &xSamples = lastChannel->voltage.sample;
        const std::vector<double> &ySamples = thisChannel->voltage.sample;
        const ::Settings::Channel *xSettings = lastChannel->channelSettings.get();
        const ::Settings::Channel *ySettings = thisChannel->channelSettings.get();

        // The channels need to be active
        if (!xSamples.size() || !ySamples.size()) {
            lastChannel->voltage.graph.clear();
            thisChannel->voltage.graph.clear();
            continue;
        }

        // Check if the sample count has changed
        const size_t sampleCount = std::min(xSamples.size(), ySamples.size());
        target.resize(sampleCount * 2);

        // Fill vector array
        const double xGain = m_deviceSettings->spec->gain[xSettings->voltage()->gainStepIndex()].gain;
        const double yGain = m_deviceSettings->spec->gain[ySettings->voltage()->gainStepIndex()].gain;
        const double xOffset = ((float)xSettings->voltage()->offset() / DIVS_VOLTAGE) + 0.5f;
        const double yOffset = ((float)ySettings->voltage()->offset() / DIVS_VOLTAGE) + 0.5f;
        const double xInvert = xSettings->inverted() ? -1.0 : 1.0;
        const double yInvert = ySettings->inverted() ? -1.0 : 1.0;

#pragma omp parallel for
        for (unsigned int position = 0; position < sampleCount; ++position) {
            target[position] = (QVector3D((float)(xSamples[position] / xGain * xInvert + xOffset),
                                          (float)(ySamples[position] / yGain * yInvert + yOffset), 0.0));
        }

        // Wait for another pair of channels
        lastChannel = nullptr;
    }
}
}
