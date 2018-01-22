// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include <tuple>
namespace Settings {
class Scope;
}
namespace Dso {
class DeviceSettings;
class ChannelUsage;
}
class PPresult;

namespace PostProcessing {

/**
 * Contains software trigger algorithm
 */
class SoftwareTrigger {
  public:
    typedef std::tuple<unsigned, unsigned, unsigned> PrePostStartTriggerSamples;
    /**
     * @brief Computes a software trigger point.
     * @param data Post processing sample set
     * @param scope Scope settings
     * @return Returns a tuple of positions [preTrigger, postTrigger, startTrigger]
     */
    static PrePostStartTriggerSamples compute(const PPresult *data, const Dso::DeviceSettings *control,
                                              const Settings::Scope *scope, const Dso::ChannelUsage *channelUsage);
};
}
