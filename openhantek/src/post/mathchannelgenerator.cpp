// SPDX-License-Identifier: GPL-2.0+

#include "mathchannelgenerator.h"
#include "enums.h"
#include "post/postprocessingsettings.h"
#include "scopesettings.h"
#include "utils/getwithdefault.h"

namespace PostProcessing {

MathChannelGenerator::MathChannelGenerator(const ::Settings::Scope *scope) : scope(scope) {}

MathChannelGenerator::~MathChannelGenerator() {}

void MathChannelGenerator::process(PPresult *result) {
    for (std::pair<const ChannelID, const std::shared_ptr<::Settings::Channel>> item : scope->channels()) {
        ::Settings::Channel *channel = item.second.get();
        // Math channel enabled?
        if (!channel->isMathChannel() || !channel->anyVisible()) continue;

        const ::Settings::MathChannel *mathChannel = static_cast<const ::Settings::MathChannel *>(channel);
        if (mathChannel->firstID() == ::Settings::Channel::INVALID ||
            mathChannel->secondID() == ::Settings::Channel::INVALID)
            continue;

        SampleValues &targetVoltage = result->addChannel(channel->channelID(), false, item.second)->voltage;
        const SampleValues &firstSampleValues = result->data(mathChannel->firstID())->voltage;
        const std::vector<double> &firstChannel = firstSampleValues.sample;
        const std::vector<double> &secondChannel = result->data(mathChannel->secondID())->voltage.sample;

        // Resize the sample vector
        targetVoltage.interval = firstSampleValues.interval;
        std::vector<double> &resultData = targetVoltage.sample;
        resultData.resize(std::min(firstChannel.size(), secondChannel.size()));

        // Calculate values and write them into the sample buffer
        switch (mathChannel->mathMode()) {
        case PostProcessing::MathMode::ADD:
            // #pragma omp parallel for
            for (unsigned int i = 0; i < resultData.size(); ++i) resultData[i] = firstChannel[i] + secondChannel[i];
            break;
        case PostProcessing::MathMode::SUBSTRACT:
            // #pragma omp parallel for
            for (unsigned int i = 0; i < resultData.size(); ++i) resultData[i] = firstChannel[i] - secondChannel[i];
            break;
        case PostProcessing::MathMode::MULTIPLY:
            // #pragma omp parallel for
            for (unsigned int i = 0; i < resultData.size(); ++i) resultData[i] = firstChannel[i] * secondChannel[i];
            break;
        }
    }
}
}
