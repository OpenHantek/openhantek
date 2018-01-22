// SPDX-License-Identifier: GPL-2.0+

#pragma once

#define NOMINMAX // disable windows.h min/max global methods
#include <limits>

#include "channelusage.h"
#include "devicesettings.h"
#include "dsocommandqueue.h"
#include "dsosamples.h"
#include "errorcodes.h"
#include "modelspecification.h"
#include "states.h"
#include "utils/debugnotify.h"
#include "utils/printutils.h"

#include "hantekprotocol/definitions.h"

#include <vector>

#include <QMetaEnum>
#include <QMutex>
#include <QStringList>
#include <QThread>
#include <QTimer>

class USBDevice;
class DsoLoop;

/// \brief The DsoControl abstraction layer for Hantek USB DSOs.
class DsoControl : public DsoCommandQueue {
    Q_OBJECT
    friend class DsoLoop;
    friend class DsoCommandQueue;

  public:
    /**
     * Creates a dsoControl object. The actual event loop / timer is not started.
     * You can optionally create a thread and move the created object to the
     * thread.
     *
     * You need to call updateInterval() to start the timer. This is done implicitly
     * if run() is called.
     *
     * DSO channels are not enabled by default. To enable a channel,
     * use deviceSettings->voltage[channel]->addChannelUser().
     *
     * @param device The usb device. No ownership is taken.
     * @param deviceSettings Runtime device settings. You must provide this, but you can use
     *    a default constructed object. If no other consumer of the device settings exist, this
     *    class will clean it up.
     */
    DsoControl(USBDevice *device, std::shared_ptr<Dso::DeviceSettings> deviceSettings);

    /// Call this to initialize the device with the deviceSettings and start the processing, by calling run()
    /// internally.
    /// It is wise to move this class object to an own thread and call start() from there.
    ///
    /// To stop processing, just destruct this object, disconnect the usbdevice object or stop the corresponding thread.
    void start();

    /// Return a read-only device control settings pointer. Use the set- Methods on this class to change
    /// device settings. You can save device settings and restore the device state by initializing this
    /// class with a loaded device settings object.
    inline const std::shared_ptr<Dso::DeviceSettings> deviceSettings() const { return m_settings; }

    /// Return the device specification. This is a convenience function and returns the same
    /// as getDevice()->getModel()->spec().
    inline const Dso::ModelSpec *specification() const { return m_specification; }

    /**
     * @return Returns the management object responsible for channel usage
     */
    inline Dso::ChannelUsage *channelUsage() { return &m_channelUsage; }

    /// \brief Get minimum samplerate for this oscilloscope.
    /// \return The minimum samplerate for the current configuration in S/s.
    double minSamplerate() const;

    /// \brief Get maximum samplerate for this oscilloscope.
    /// \return The maximum samplerate for the current configuration in S/s.
    double maxSamplerate() const;

    double maxSingleChannelSamplerate() const;

    /// Return the associated usb device.
    inline const USBDevice *getDevice() const { return device; }

    inline DsoLoop *loopControl() { return m_loop.get(); }

  protected: // Non-public methods
    double computeTimebase(double samplerate) const;

    /// Called right at the beginning to retrieve scope calibration data
    Dso::ErrorCode retrieveOffsetCalibrationData();

    /// \brief Gets the current state.
    /// \return The current CaptureState of the oscilloscope.
    std::pair<int, unsigned> retrieveCaptureState() const;

    /// \brief Retrieve sample data from the oscilloscope
    Dso::ErrorCode retrieveSamples(unsigned &expectedSampleCount);

    /// \brief Gets the speed of the connection.
    /// \return The ::ConnectionSpeed of the USB connection.
    Dso::ErrorCode retrieveConnectionSpeed();

    /// \brief The resulting tuple of the computeBestSamplerate() function
    struct BestSamplerateResult {
        unsigned downsampler = 0;
        double samplerate = 0.0;
        bool fastrate = false;
    };
    /// \brief Calculate the nearest samplerate supported by the oscilloscope.
    /// \param samplerate The target samplerate, that should be met as good as possible.
    /// \param maximum The target samplerate is the maximum allowed when true, the
    /// minimum otherwise.
    /// \return Tuple: The nearest samplerate supported, 0.0 on error and downsampling factor.
    BestSamplerateResult computeBestSamplerate(double samplerate, bool maximum = false) const;

    /// \brief Sets the samplerate based on the parameters calculated by
    /// Control::getBestSamplerate.
    /// \param downsampler The downsampling factor.
    /// \param fastRate true, if one channel uses all buffers.
    /// \return The downsampling factor that has been set.
    unsigned updateSamplerate(unsigned downsampler, bool fastRate);

    /// \brief Restore the samplerate/timebase targets after divider updates.
    void restoreTargets();

    /// \brief Update the minimum and maximum supported samplerate.
    void notifySamplerateLimits();

    /// \brief Enables/disables filtering of the given channel.
    ///
    /// This may have influence on the sampling speed: Some scopes support a fast-mode, if only
    /// a limited set of channels (usually only 1) is activated.
    ///
    /// This method is not meant to be used directly. Use the deviceSettings to enable/disable channels,
    /// by "using" them.
    ///
    /// \param channel The channel that should be set.
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode checkChannelUsage();

  protected: // non-public variables
    // Communication with device
    USBDevice *device; ///< The USB device for the oscilloscope

    // Device setup
    const Dso::ModelSpec *m_specification;           ///< The specifications of the device
    std::shared_ptr<Dso::DeviceSettings> m_settings; ///< The current settings of the device
    Dso::ChannelUsage m_channelUsage;

    // Raw sample cache
    std::vector<unsigned char> m_rawdata;
    std::unique_ptr<DsoLoop> m_loop;

  public slots:
    /// \brief Sets the size of the oscilloscopes sample buffer.
    /// \param index The record length index that should be set.
    Dso::ErrorCode setRecordLengthByIndex(RecordLengthID size);

    /// \brief Sets the samplerate of the oscilloscope.
    /// \param samplerate The samplerate that should be met (S/s)
    /// current samplerate. You cannot set a samplerate for a fixed samplerate device with
    /// this method. Use setFixedSamplerate() instead.
    Dso::ErrorCode setSamplerate(double samplerate);

    /// \brief Sets the samplerate of the oscilloscope for fixed samplerate devices. Does
    /// nothing on a device that supports a ranged samplerate value. Check with
    /// deviceSpecification->isFixedSamplerateDevice if in doubt.
    /// \param samplerrateId The samplerate id
    Dso::ErrorCode setFixedSamplerate(unsigned samplerrateId);

    /// \brief Sets the time duration of one aquisition by adapting the samplerate.
    /// \param duration The record time duration that should be met (s)
    Dso::ErrorCode setRecordTime(double duration);

    /// \brief Set the coupling for the given channel.
    /// \param channel The channel that should be set.
    /// \param coupling The new coupling for the channel.
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode setCoupling(ChannelID channel, Dso::Coupling coupling);

    /// \brief Sets the gain for the given channel.
    /// Get the actual gain by specification.gainSteps[hardwareGainIndex]
    /// \param channel The channel that should be set.
    /// \param hardwareGainIndex The gain index that refers to a gain value, defined by the device model.
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode setGain(ChannelID channel, unsigned hardwareGainIndex, bool overwrite = false);

    /// \brief Set the offset for the given channel.
    /// Get the actual offset for the channel from devicesettings.voltage[channel].offsetReal
    /// \param channel The channel that should be set.
    /// \param offset The new offset value [-1.0,1.0]. Default is 0.
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode setOffset(ChannelID channel, double offset, bool overwrite = false);

    /// \brief Set the trigger mode.
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode setTriggerMode(Dso::TriggerMode mode);

    /// \brief Set the trigger source.
    /// \param special true for a special channel (EXT, ...) as trigger source.
    /// \param id The number of the channel, that should be used as trigger.
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode setTriggerSource(bool special, ChannelID channel);

    /// \brief Set the trigger level.
    /// \param channel The channel that should be set.
    /// \param level The new trigger offset value [-1.0,1.0]. Default is 0.
    /// \return The trigger level that has been set, ::Dso::ErrorCode on error.
    Dso::ErrorCode setTriggerOffset(ChannelID channel, double offset, bool overwrite = false);

    /// \brief Set the trigger slope.
    /// \param slope The Slope that should cause a trigger.
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode setTriggerSlope(Dso::Slope slope);

    /// \brief Set the pre-trigger sample range in percentage.
    ///
    /// A sample set is longer than what is displayed on screen and only the part where the hard- or software trigger
    /// found a matching signal slope is shown. To move the visible area, this method can be used by providing a
    /// percentage value.
    ///
    /// \param position The new pre-trigger position [0.0,1.0]. Default is 0.
    /// \return See ::Dso::ErrorCode.
    Dso::ErrorCode setPretriggerPosition(double position, bool overwrite = false);

    /// \brief Forces a hardware-trigger to trigger although the condition is not met
    /// Does nothing on software-trigger devices.
    void forceTrigger();

  signals:
    void samplingStatusChanged(bool enabled);         ///< The oscilloscope started/stopped sampling/waiting for trigger
    void samplesAvailable(const DSOsamples *samples); ///< New sample data is available
    void communicationError() const;                  ///< USB device error (disconnect/transfer/misbehave problem)
    void debugMessage(const QString &msg, Debug::NotificationType typeEnum) const;
};

Q_DECLARE_METATYPE(DSOsamples *)
Q_DECLARE_METATYPE(std::vector<Dso::FixedSampleRate>)
