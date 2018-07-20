// SPDX-License-Identifier: GPL-2.0+

#pragma once

/**<
  * Contains all settings for a currently connected device as well as the state that this device is in.
  * Changes (write access) are only allowed from within the DsoControl class. You can connect to various signals
  * to be notified of changes.
  */

#include "enums.h"
#include "hantekdso/modelspecification.h"
#include "hantekprotocol/codes.h"
#include "hantekprotocol/types.h"

#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <set>

namespace Settings {
struct DeviceSettingsIO;
}

class DsoControl;

namespace Dso {

struct ControlSamplerateLimits;

/// \brief Stores the current or target samplerate settings of the device.
struct Samplerate {
    double samplerate = 1e8;       ///< The target samplerate set via setSamplerate
    double timebase = 1e-3;        ///< The target record time set via setRecordTime
    unsigned fixedSamplerateId = 0; ///< The target samplerate for fixed samplerate devices set via setFixedSamplerate
};

/// \brief Stores the current trigger settings of the device.
class Trigger : public QObject {
    friend struct Settings::DeviceSettingsIO;
    Q_OBJECT
  public:
    DsoE::TriggerMode mode() const { return m_mode; } ///< The trigger mode
    DsoE::Slope slope() const { return m_slope; }     ///< The trigger slope
    double position() const { return m_position; }   ///< The current pretrigger position in range [0,1]
    bool special() const { return m_special; }       ///< true, if the trigger source is special
    unsigned source() const { return m_source; }     ///< The trigger source
    unsigned point() const { return m_point; }       ///< The trigger position in Hantek coding
    unsigned swTriggerThreshold() const { return m_swTriggerThreshold; } ///< Software trigger, threshold
    unsigned swTriggerSampleSet() const { return m_swTriggerSampleSet; } ///< Software trigger, sample set
    unsigned swSampleMargin() const { return m_swSampleMargin; }         ///< Software trigger, sample margin

    void setPosition(double position);
    void setPoint(unsigned point) { this->m_point = point; }
    void setTriggerSource(ChannelID channel, bool specialChannel);
    void setSlope(DsoE::Slope slope);
    void setMode(DsoE::TriggerMode mode);

  private:
    DsoE::TriggerMode m_mode =                  //
        DsoE::TriggerMode::HARDWARE_SOFTWARE;   ///< The trigger mode
    DsoE::Slope m_slope = DsoE::Slope::Positive; ///< The trigger slope
    double m_position = 0.0;                   ///< The current pretrigger position in range [0,1]
    bool m_special = false;                    ///< true, if the trigger source is special
    unsigned m_source = 0;                     ///< The trigger source
    unsigned m_point = 0;                      ///< The trigger position in Hantek coding
    unsigned m_swTriggerThreshold = 7;         ///< Software trigger, threshold
    unsigned m_swTriggerSampleSet = 11;        ///< Software trigger, sample set
    unsigned m_swSampleMargin = 2000;          ///< Software trigger, sample margin
  signals:
    void modeChanged(DsoE::TriggerMode mode);           ///< The trigger mode has been changed
    void sourceChanged(bool special, unsigned int id); ///< The trigger source has been changed
    void slopeChanged(DsoE::Slope slope);               ///< The trigger slope has been changed
    void positionChanged(double position);             ///< The trigger position has been changed
};

/// \brief Stores the current amplification settings of the device.
class Channel : public QObject {
    Q_OBJECT
    friend struct Settings::DeviceSettingsIO;

  public:
    /// The current coupling index
    inline unsigned couplingIndex() const { return m_couplingIndex; }
    /// The vertical resolution gain index for gain in V
    inline unsigned gainStepIndex() const { return m_gainStepIndex; }
    /// The current offset value in [-1,1].
    inline double offset() const { return m_offset; }
    /// \return Returns the hardware applied offset. For devices that do not support hardware offsets, this will be 0.
    inline double offsetHardware() const { return m_offsetHardware; }
    /// \return Returns the trigger level in range [0,1]
    inline double triggerLevel() const { return m_triggerOffset; }

    /// Get the coupling value for the specific channel
    inline DsoE::Coupling coupling(const Dso::ModelSpec *spec) const { return spec->couplings[m_couplingIndex]; }

    /**
     * @brief Sets the offset value and emit the corresponding signal. Only to be called by HantekDsoControl.
     * @param offset The offset value [-1,1]
     * @param offsetHardware Not necessary for math channels. The hardware applied offset. The received raw sample
     *        set will have this offset applied, so we need to remember and remove it later to output clean sample
     *        values in range [0,1].
     */
    void setOffset(double offset, double offsetHardware = 0.0);

    /**
     * Sets the trigger level / trigger offset and emit the corresponding signal. Only to be called by HantekDsoControl.
     * @param offset The offset value [-1,1]
     */
    void setTriggerOffset(double offset);

    /**
     * @brief Sets the gain id and emit the corresponding signal. Only to be called by HantekDsoControl.
     * @param gainId Gain ID. Must be in range with Dso::ModelSpec defined gain ids.
     */
    void setGainStepIndex(unsigned gainId);

    /**
     * @brief Sets the coupling id and emit the corresponding signal. Only to be called by HantekDsoControl.
     * @param couplingId Coupling ID. Must be in range with Dso::ModelSpec defined coupling ids.
     */
    void setCouplingIndex(unsigned couplingId);

  private:
    unsigned m_couplingIndex = 0;  ///< Coupling index (refers to one of the DsoE::Coupling values)
    unsigned m_gainStepIndex = 0;  ///< The vertical resolution gain index for gain in V
    double m_offset = 0.;          ///< The offset for each channel [-1,1].
    double m_offsetHardware = 0.0; ///< The hardware applied offset for each channel (Quantization+Min/Max considered)
    Voltage m_triggerOffset;       ///< The trigger level in V

  signals:
    void gainStepIndexChanged(unsigned gainId);
    void couplingIndexChanged(unsigned couplingIndex);
    void offsetChanged(double offset);
    void triggerLevelChanged(double triggerLevel);
};

/// A samplerate can be set/influenced via the timebase, a samplerate value, a fixed samplerate id that refers
/// to a samplerate. We need to keep track which is the source for the current device samplerate.
enum class SamplerateSource { Duration, FixedSamplerate, Samplerrate };

/// Contains the current device settings as well as the current state of the scope device.
/// Those settings and the state are highly interactive with the {@link HantekDsoControl} class.
/// If a change to the state is made, it is propagated via the signals of this class.
class DeviceSettings : public QObject {
    Q_OBJECT
    friend struct Settings::DeviceSettingsIO;

  public:
    DeviceSettings(const ModelSpec *specification);
    ~DeviceSettings();

    /// \brief Return the target samplerate, as set by the user.
    /// You have access to a (samperate,record-time,fixed-samplerate-id)-tuple,
    /// but only the value defined by samplerateSource() is valid.
    ///
    /// The value is not necessarly what is applied to the hardware. To lookup the current samplerate,
    /// use samplerate() instead.
    inline const Samplerate &target() const { return m_targetSamperate; }
    inline SamplerateSource samplerateSource() const { return m_samplerateSource; }

    /// Return the current (samperate,record-time,fixed-samplerate-id)-tuple. The fixed-samplerate-id
    /// is only valid, if this is a fixed samplerates model. Use spec->isFixedSamplerateDevice if in doubt.
    inline const Samplerate &samplerate() const { return m_samplerate; }

    /// Return true if roll-mode is enabled.
    inline bool isRollMode() const { return limits->recordLengths[m_recordLengthId].recordLength == ROLL_RECORDLEN; }

    /// Returns true if in fast rate mode (one channel uses all bandwith)
    inline bool isFastRate() const { return limits == &spec->fastrateSamplerate; }

    /// \brief Gets the record length id.
    /// The id can be used to look up the record length in the model specification.
    inline RecordLengthID recordLengthId() const { return m_recordLengthId; }

    /// \brief Sets the record length id.
    /// The id will be used to look up the record length in the model specification.
    /// Called by DsoControl. Never call this out of DsoControl because the values will not be applied back.
    void setRecordLengthId(RecordLengthID value);

    /// Updates the (samperate,record-time,fixed-samplerate-id)-tuple.
    /// Called by DsoControl. Never call this out of DsoControl because the values will not be applied back.
    void updateCurrentSamplerate(double samplerate, double timebase, unsigned fixedSamplerateIndex);

    /// A samplerate, recordtime or samplerate based on fixed ids can never be set alone. Each parameter
    /// influences the others. This method allows to manipulate the target Samplerate structure, but you must
    /// define which parameter should be the dominating one / the source for the others.
    /// Called by DsoControl. Never call this out of DsoControl because the values will not be applied back.
    Samplerate &updateTarget(SamplerateSource source);

    /// \brief Return the hardware applied gain in V.
    /// Uses the current gain step id and gain steps defined in the model specification.
    inline double gain(ChannelID channel) const { return spec->gain[voltage[channel]->gainStepIndex()].gain; }

    /// \brief Return the record length
    /// Uses the current recordLengthId and record lengths defined in the model specification.
    inline unsigned getRecordLength() const { return limits->recordLengths[m_recordLengthId].recordLength; }

    /// Returns a step value meant to be used for adjusting the offset value [-1,1]. Because of quantization
    /// of the offset which will be in the range of [offsetStart,offsetEnd] of calibration[channel][gainId]
    /// we can't just use a pure double.
    inline double offsetAdjustStep(ChannelID channel) const {
        /// For non physical channels or not supported hardware offset
        if (!spec->supportsOffset || channel > spec->calibration.size()) return 0.001;

        const Dso::ModelSpec::GainStepCalibration &c = spec->calibration[channel][voltage[channel]->gainStepIndex()];
        return 1.0 / (c.offsetEnd - c.offsetStart);
    }

    /// \brief Gets the maximum size of one packet transmitted via bulk transfer.
    inline unsigned packetSize() const { return m_packetSize; }

    inline unsigned getSampleCount() const {
        if (isRollMode())
            return packetSize();
        else {
            return (isFastRate()) ? getRecordLength() : getRecordLength() * spec->channels;
        }
    }

  public:
    const ModelSpec *spec;

    // Device settings
    std::vector<Channel *> voltage; ///< The amplification settings
    Trigger trigger;                ///< The trigger settings

    // State variables: Those are not stored/restored
    const ControlSamplerateLimits *limits; ///< The samplerate limits
    unsigned int downsampler = 1;          ///< The variable downsampling factor
    unsigned m_packetSize = 0;             ///< Device packet size

  private:
    // Device settings
    SamplerateSource m_samplerateSource;
    Samplerate m_targetSamperate; ///< The target samplerate values

    // State variables: Those are not stored/restored
    Samplerate m_samplerate;             ///< The samplerate settings
    RecordLengthID m_recordLengthId = 1; ///< The id in the record length array
  signals:
    /// The available samplerate range has changed
    void samplerateLimitsChanged(double minimum, double maximum);
    /// The available samplerate for fixed samplerate devices has changed
    void fixedSampleratesChanged(const std::vector<Dso::FixedSampleRate> &sampleSteps);
    /// The available record lengths. Is also emitted whenever the samplerate limits changed
    void availableRecordLengthsChanged(const std::vector<Dso::RecordLength> &recordLengths);
    /// The samplerate, or fixed samplerateId or recordTime has changed
    void samplerateChanged(Samplerate samplerate);
    /// The record length has changed
    void recordLengthChanged(unsigned m_recordLengthId);
};
}
Q_DECLARE_METATYPE(Dso::Samplerate)

class QSettings;
namespace Settings {
struct DeviceSettingsIO {
    static void read(QSettings *io, Dso::DeviceSettings &control);
    static void write(QSettings *io, const Dso::DeviceSettings &control);
};
}
