// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "devicesettings.h"
#include "dsosamples.h"
#include "errorcodes.h"
#include "hantekdso/modelspecification.h"
#include "hantekprotocol/bulkStructs.h"
#include "hantekprotocol/controlStructs.h"
#include "hantekprotocol/definitions.h"
#include "utils/debugnotify.h"

#include <vector>

class DsoControl;

/**
 * Implements the Dso logic for fetching/converting the samples at the right time.
 */
class DsoLoop : public QObject {
    Q_OBJECT
  public:
    DsoLoop(std::shared_ptr<Dso::DeviceSettings> settings, DsoControl *control);

    /// Call this to start the processing loop.
    /// This method will call itself periodically from there on.
    inline void run() { m_settings->isRollMode() ? runRollMode() : runStandardMode(); }

    inline bool isSampling() const { return sampling; }

    /// \brief If sampling is disabled, no samplesAvailable() signals are send anymore, no samples
    /// are fetched from the device and no processing takes place.
    /// \param enabled Enables/Disables sampling
    void enableSampling(bool enabled);

    /// Return the last sample set
    inline const DSOsamples &getLastSamples() { return result; }

  private:
    void runRollMode();
    void runStandardMode();

    void updateInterval();

    /// \brief Calculates the trigger point from the CommandGetCaptureState data.
    /// \param value The data value that contains the trigger point.
    /// \return The calculated trigger point for the given data.
    static unsigned calculateTriggerPoint(unsigned value);

    /// \brief Converts raw oscilloscope data to sample data
    void convertRawDataToSamples(const std::vector<unsigned char> &rawData);

  private:
    int captureState = Hantek::CAPTURE_WAITING;
    Hantek::RollState rollState = Hantek::RollState::STARTSAMPLING;
    bool _samplingStarted = false;
    Dso::TriggerMode lastTriggerMode = (Dso::TriggerMode)-1;
    std::chrono::milliseconds cycleCounter = 0ms;
    std::chrono::milliseconds startCycle = 0ms;
    std::chrono::milliseconds cycleTime = 0ms;
    bool sampling = false;            ///< true, if the oscilloscope is taking samples
    unsigned expectedSampleCount = 0; ///< The expected total number of samples at
                                      /// the last check before sampling started
    // Device setup
    const Dso::ModelSpec *m_specification;           ///< The specifications of the device
    std::shared_ptr<Dso::DeviceSettings> m_settings; ///< The current settings of the device
    const int m_modelID;

    // Results
    DSOsamples result;

    DsoControl *m_control;
};
