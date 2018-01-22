// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "enums.h"
#include "hantekprotocol/codes.h"
#include "hantekprotocol/controlStructs.h"
#include "hantekprotocol/definitions.h"
#include "hantekprotocol/types.h"
#include <QList>

#include <cmath>
#include <limits>

namespace Dso {
constexpr static unsigned ROLL_RECORDLEN = UINT_MAX;

struct RecordLength {
    unsigned recordLength;  ///< Record length, ROLL_RECORDLEN means rolling
    unsigned bufferDivider; ///< Samplerate dividers for record lengths
    inline RecordLength(unsigned recordLength, unsigned bufferDivider)
        : recordLength(recordLength), bufferDivider(bufferDivider) {}
    RecordLength() = default;
};

/// \brief Stores the samplerate limits for calculations.
struct ControlSamplerateLimits {
    double base;                             ///< The base for sample rate calculations
    double max;                              ///< The maximum sample rate
    unsigned maxDownsampler;                 ///< The maximum downsampling ratio
    std::vector<RecordLength> recordLengths; ///< Available record lengths

    inline double minSamplerate(RecordLengthID id) const {
        return base / maxDownsampler / recordLengths[id].recordLength;
    }
    inline double samplerate(RecordLengthID id, unsigned downsampler) const {
        return base / downsampler / recordLengths[id].recordLength;
    }
    inline double samplerate(RecordLengthID id, double recordTime) const {
        return recordLengths[id].bufferDivider / recordTime;
    }

    inline double maxSamplerate(RecordLengthID id) const { return max / recordLengths[id].bufferDivider; }

    inline unsigned computeDownsampler(RecordLengthID id, double samplerate) const {
        return unsigned(base / recordLengths[id].bufferDivider / samplerate);
    }
};

struct ControlSpecificationGainLevel {
    /// The index of the selected gain on the hardware
    unsigned char gainIdentificator;
    /// Available voltage steps in V/screenheight
    double gain;
};

struct FixedSampleRate {
    unsigned char id; ///< hardware id
    double samplerate;
};

struct SpecialTriggerChannel {
    std::string name;
    int hardwareID;
};

/// \brief Stores the specifications of the currently connected device.
struct ModelSpec {
    ModelSpec(unsigned channels);
    const ChannelID channels;

    // Interface
    Hantek::BulkCode cmdSetChannels = Hantek::BulkCode::INVALID;             ///< Command for setting used channels
    Hantek::BulkCode cmdSetSamplerate = Hantek::BulkCode::INVALID;           ///< Command for samplerate settings
    Hantek::BulkCode cmdSetRecordLength = Hantek::BulkCode::INVALID;         ///< Command for buffer settings
    Hantek::BulkCode cmdSetTrigger = Hantek::BulkCode::INVALID;              ///< Command for trigger settings
    Hantek::BulkCode cmdSetPretrigger = Hantek::BulkCode::INVALID;           ///< Command for pretrigger settings
    Hantek::BulkCode cmdForceTrigger = Hantek::BulkCode::FORCETRIGGER;       ///< Command for forcing a trigger event
    Hantek::BulkCode cmdCaptureStart = Hantek::BulkCode::STARTSAMPLING;      ///< Command for starting the sampling
    Hantek::BulkCode cmdTriggerEnabled = Hantek::BulkCode::ENABLETRIGGER;    ///< Command for enabling the trigger
    Hantek::BulkCode cmdGetData = Hantek::BulkCode::GETDATA;                 ///< Command for retrieve sample data
    Hantek::BulkCode cmdGetCaptureState = Hantek::BulkCode::GETCAPTURESTATE; ///< Command for retrieve the capture state
    Hantek::BulkCode cmdSetGain = Hantek::BulkCode::SETGAIN;                 ///< Command for setting the gain

    // Actual resolved commands based on the above interface
    const Hantek::ControlBeginCommand beginCommandControl;

    // Limits

    /// The limits for multi channel mode
    ControlSamplerateLimits normalSamplerate = {50e6, 50e6, 0, std::vector<RecordLength>()};
    /// The limits for single channel mode
    ControlSamplerateLimits fastrateSamplerate = {100e6, 100e6, 0, std::vector<RecordLength>()};

    unsigned char sampleSize; ///< Number of bits per sample

    /// For devices that support only fixed sample rates (isFixedSamplerateDevice=true)
    std::vector<FixedSampleRate> fixedSampleRates;

    // Calibration

    struct GainStepCalibration {
        double offsetCorrection = 0;
        unsigned short offsetStart = 0x0000;
        unsigned short offsetEnd = 0xffff;
        double voltageLimit = 255.0;
        inline GainStepCalibration(double offsetCorrection, unsigned short offsetStart, unsigned short offsetEnd,
                                   double voltageLimit)
            : offsetCorrection(offsetCorrection), offsetStart(offsetStart), offsetEnd(offsetEnd),
              voltageLimit(voltageLimit) {}
        inline GainStepCalibration(unsigned short offsetStart, unsigned short offsetEnd, double voltageLimit)
            : offsetStart(offsetStart), offsetEnd(offsetEnd), voltageLimit(voltageLimit) {}
    };

    /// The sample values at the top of the screen
    typedef std::vector<GainStepCalibration> gainStepCalibration;

    std::vector<gainStepCalibration> calibration; // Per channel

    /// Gain levels
    std::vector<ControlSpecificationGainLevel> gain; // Usually size==HANTEK_GAIN_STEPS

    // Features
    std::vector<SpecialTriggerChannel> specialTriggerChannels;
    std::vector<Coupling> couplings = {Dso::Coupling::DC, Dso::Coupling::AC};
    std::vector<TriggerMode> triggerModes = {TriggerMode::HARDWARE_SOFTWARE, TriggerMode::WAIT_FORCE,
                                             TriggerMode::SINGLE};
    bool isFixedSamplerateDevice = false;
    bool isSoftwareTriggerDevice = false;
    bool useControlNoBulk = false;
    bool supportsCaptureState = true;
    bool supportsOffset = true;
    bool supportsCouplingRelays = true;
    bool supportsFastRate = true;
    int fixedUSBinLength = 0;
    double testSignalAmplitude = 1.0; ///< Test signal amplitude in V. Usually 1V.

    inline int indexOfTriggerMode(TriggerMode mode) const {
        return int(std::find(triggerModes.begin(), triggerModes.end(), mode) - triggerModes.begin());
    }
};
}
Q_DECLARE_METATYPE(Dso::RecordLength);
