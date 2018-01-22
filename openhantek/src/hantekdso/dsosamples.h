// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "hantekprotocol/types.h"
#include <QReadLocker>
#include <QReadWriteLock>
#include <QWriteLocker>
#include <vector>

struct DSOsamples {
    struct ChannelSamples : public std::vector<double> {
        ChannelID id;
        // some statistics
        double minVoltage;
        double maxVoltage;
        uint16_t minRaw;
        uint16_t maxRaw;
    };
    std::vector<ChannelSamples> data; ///< Pointer to input data from device
    double samplerate = 0.0;          ///< The samplerate of the input data
    bool append = false;              ///< true, if waiting data should be appended
    unsigned availableChannels = 0;
    mutable QReadWriteLock lock;

    DSOsamples(unsigned channels) { data.resize(channels); }
    inline unsigned channelCount() const { return availableChannels; }
    /**
     * Clears the sample array and sets all fields. For performance reasons, we do not
     * resize the channel dimension of the data array. This way all allocated resources
     * are still allocated and can potentially be reused.
     *
     * @param channels The number of available channels. This must be lower than the channel count
     *  of the constructor.
     * @param samplerate A samplerate
     * @param append Roll mode or not
     */
    inline void prepareForWrite(unsigned channels, double samplerate, bool append) {
        this->samplerate = samplerate;
        this->append = append;
        this->availableChannels = channels;
        for (ChannelSamples &v : data) {
            v.id = (unsigned)-1; /// Invalid id
            v.clear();           /// Clear all samples
            v.maxRaw = 0;
            v.minRaw = 0;
            v.minVoltage = 2;
            v.maxVoltage = -2;
        }
    }
};
