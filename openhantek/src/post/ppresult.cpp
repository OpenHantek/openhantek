// SPDX-License-Identifier: GPL-2.0+

#include "ppresult.h"
#include "utils/getwithdefault.h"
#include <QDebug>
#include <stdexcept>

PPresult::PPresult() : inUse(false) {}

const DataChannel *PPresult::data(ChannelID channelID) const {
    auto it = analyzedData.find(channelID);
    if (it == analyzedData.end()) return nullptr;
    return &it->second;
}

DataChannel *PPresult::modifyData(ChannelID channelID) {
    auto it = analyzedData.find(channelID);
    if (it == analyzedData.end()) return nullptr;
    return &it->second;
}

DataChannel *PPresult::addChannel(ChannelID channelID, bool deviceChannel,
                                  std::shared_ptr<Settings::Channel> channelSettings) {
    auto it = analyzedData.find(channelID);
    if (it != analyzedData.end()) return &it->second;
    analyzedData.insert(std::make_pair(channelID, DataChannel(channelID, deviceChannel, channelSettings)));
    return &analyzedData.find(channelID)->second;
}

unsigned int PPresult::sampleCount() const { return (unsigned)analyzedData.begin()->second.voltage.sample.size(); }
