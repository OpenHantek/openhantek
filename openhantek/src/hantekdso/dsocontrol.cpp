// SPDX-License-Identifier: GPL-2.0+

#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

#include <QDebug>
#include <QList>
#include <QMutex>
#include <QTimer>

#include "dsocontrol.h"
#include "dsoloop.h"
#include "dsomodel.h"
#include "usb/usbdevice.h"

#ifdef WIN32
#include <Winsock2.h>
#else
#include <netinet/in.h> // for ntohs, network to host order for shorts: big to little endian
#endif

using namespace Hantek;
using namespace Dso;

#ifdef DEBUG
#define DBGNOTIFY(x, y) emit debugMessage(x, y)
#else
#define DBGNOTIFY(x, y)
#endif

DsoControl::DsoControl(USBDevice *device, std::shared_ptr<Dso::DeviceSettings> deviceSettings)
    : DsoCommandQueue(deviceSettings->spec, device, this), device(device), m_specification(deviceSettings->spec),
      m_settings(deviceSettings), m_channelUsage(m_specification->channels), m_loop(new DsoLoop(deviceSettings, this)) {
    if (device == nullptr) throw new std::runtime_error("No usb device for HantekDsoControl");

    qRegisterMetaType<DSOsamples *>();
    qRegisterMetaType<std::vector<Dso::FixedSampleRate>>("std::vector<Dso::FixedSampleRate>");
    qRegisterMetaType<Hantek::BulkCode>("Hantek::BulkCode");
    qRegisterMetaType<Hantek::ControlCode>("Hantek::ControlCode");
    qRegisterMetaType<Samplerate>("Samplerate");
    qRegisterMetaType<Dso::RecordLength>("Dso::RecordLength");

    if (m_specification->fixedUSBinLength) device->overwriteInPacketLength(m_specification->fixedUSBinLength);
    // Apply special requirements by the devices model
    device->getModel()->applyRequirements(this);

    // Check for gain level definitions
    bool error = m_specification->calibration.size() != m_specification->channels;
    if (!error)
        for (ChannelID channelId = 0; channelId < m_specification->channels; ++channelId) {
            error |= m_specification->calibration[channelId].size() != m_specification->gain.size();
        }

    if (error) {
        qWarning() << "Model definition is faulty. Please check voltageLimit and gain levels to be defined for at "
                      "least HANTEK_GAIN_STEPS";
    }
}

void DsoControl::start() {
    DBGNOTIFY("Init device", Debug::NotificationType::DSOControl);

    retrieveOffsetCalibrationData();
    retrieveConnectionSpeed();

    for (ChannelID channelId = 0; channelId < m_settings->voltage.size(); ++channelId) {
        setCoupling(channelId, m_settings->voltage[channelId]->coupling(m_specification));
        setGain(channelId, m_settings->voltage[channelId]->gainStepIndex(), true); // sets offset as well
        setTriggerOffset(channelId, m_settings->voltage[channelId]->triggerLevel(), true);
    }

    // connect to use-channel signals
    connect(&m_channelUsage, &ChannelUsage::usedChanged, this, &DsoControl::checkChannelUsage);

    checkChannelUsage();
    setRecordLengthByIndex(m_settings->recordLengthId());
    setTriggerMode(m_settings->trigger.mode());
    setPretriggerPosition(m_settings->trigger.position(), true);
    setTriggerSlope(m_settings->trigger.slope());
    setTriggerSource(m_settings->trigger.special(), m_settings->trigger.source());
    restoreTargets();
    m_loop->run();
}

double DsoControl::minSamplerate() const {
    return m_specification->normalSamplerate.minSamplerate(m_settings->recordLengthId());
}

double DsoControl::maxSamplerate() const { return m_settings->limits->maxSamplerate(m_settings->recordLengthId()); }

double DsoControl::maxSingleChannelSamplerate() const {
    return m_specification->normalSamplerate.maxSamplerate(m_settings->recordLengthId());
}

double DsoControl::computeTimebase(double samplerate) const {
    unsigned sampleCount = m_settings->getRecordLength();
    if (m_specification->isSoftwareTriggerDevice) sampleCount -= m_settings->trigger.swSampleMargin();
    return (double)(sampleCount) / samplerate;
}

Dso::ErrorCode DsoControl::retrieveOffsetCalibrationData() {
    // Get channel level data
    Hantek::ControlGetLimits cmdGetLimits(m_specification->channels);

    int readBytes = device->controlRead(&cmdGetLimits);
    if (readBytes < 0) {
        const QString errMsg = "Couldn't get channel level data from oscilloscope";
        qWarning() << errMsg;
        DBGNOTIFY(errMsg, Debug::NotificationType::DSOControl);
        emit communicationError();
        return Dso::ErrorCode::CONNECTION;
    }

    if ((unsigned)readBytes != sizeof(Hantek::ControlGetLimits::OffsetsPerGainStep) * m_specification->channels) {
        DBGNOTIFY("Offset calibration data not supported", Debug::NotificationType::DSOControl);
        return Dso::ErrorCode::UNSUPPORTED;
    }

    DBGNOTIFY("Offset calibration data received", Debug::NotificationType::DSOControl);

    // Access model specification in write-mode. Only retrieveOffsetCalibrationData and the (self-)calibration
    // are allowed to do so.
    std::vector<Dso::ModelSpec::gainStepCalibration> &cal = const_cast<Dso::ModelSpec *>(m_specification)->calibration;
    Hantek::ControlGetLimits::OffsetsPerGainStep *data = cmdGetLimits.offsetLimit.get();
    for (ChannelID channelId = 0; channelId < m_specification->channels; ++channelId) {
        for (unsigned gainId = 0; gainId < Hantek::ControlGetLimits::HANTEK_GAIN_STEPS; ++gainId) {
            // Convert little->big endian if necessary
            cal[channelId][gainId].offsetStart = ntohs(data[channelId].step[gainId].start);
            cal[channelId][gainId].offsetEnd = ntohs(data[channelId].step[gainId].end);
        }
    }

    return Dso::ErrorCode::NONE;
}

std::pair<int, unsigned> DsoControl::retrieveCaptureState() const {
    int errorCode;

    if (!m_specification->supportsCaptureState) return std::make_pair(CAPTURE_READY, 0);

    errorCode = bulkCommand(getCommand(BulkCode::GETCAPTURESTATE), 1);
    if (errorCode < 0) {
        qWarning() << "Getting capture state failed: " << libUsbErrorString(errorCode);
        return std::make_pair(CAPTURE_ERROR, 0);
    }

    BulkResponseGetCaptureState response;
    errorCode = device->bulkRead(&response);
    if (errorCode < 0) {
        qWarning() << "Getting capture state failed: " << libUsbErrorString(errorCode);
        return std::make_pair(CAPTURE_ERROR, 0);
    }

    return std::make_pair((int)response.getCaptureState(), response.getTriggerPoint());
}

ErrorCode DsoControl::retrieveSamples(unsigned &previousSampleCount) {
    int errorCode;
    if (!m_specification->useControlNoBulk) {
        // Request data
        errorCode = bulkCommand(getCommand(BulkCode::GETDATA), 1);
    } else {
        errorCode = device->controlWrite(getCommand(ControlCode::ACQUIRE_DATA));
    }
    if (errorCode <= 0) {
        qWarning() << "Getting sample data failed: " << libUsbErrorString(errorCode);
        emit communicationError();
        return ErrorCode::PARAMETER;
    }

    unsigned totalSampleCount = m_settings->getSampleCount();

    // To make sure no samples will remain in the scope buffer, also check the
    // sample count before the last sampling started
    if (totalSampleCount < previousSampleCount) {
        std::swap(totalSampleCount, previousSampleCount);
    } else {
        previousSampleCount = totalSampleCount;
    }

    unsigned dataLength = (m_specification->sampleSize > 8) ? totalSampleCount * 2 : totalSampleCount;

    // Save raw data to temporary buffer
    m_rawdata.resize(dataLength);
    int errorcode = device->bulkReadMulti(m_rawdata.data(), dataLength);
    if (errorcode < 0) {
        DBGNOTIFY(QString("Getting sample data failed: %1").arg(libUsbErrorString(errorcode)),
                  Debug::NotificationType::DSOControl);
        return ErrorCode::PARAMETER;
    }
    m_rawdata.resize((size_t)errorcode);

    static unsigned id = 0;
    DBGNOTIFY(QString("Received packet %1").arg(id++), Debug::NotificationType::DSOLoop);

    return ErrorCode::NONE;
}

ErrorCode DsoControl::retrieveConnectionSpeed() {
    int errorCode;
    ControlGetSpeed response;
    errorCode = device->controlRead(&response);
    if (errorCode < 0) {
        qWarning() << "Retrieve connection speed failed" << libusb_error_name(errorCode);
        return ErrorCode::UNEXPECTED;
    }

    if (response.getSpeed() == ConnectionSpeed::FULLSPEED)
        m_settings->m_packetSize = 64;
    else if (response.getSpeed() == ConnectionSpeed::HIGHSPEED)
        m_settings->m_packetSize = 512;
    else {
        qWarning() << "Unknown USB speed. Please correct source code in DsoControl::retrieveConnectionSpeed()";
        throw new std::runtime_error("Unknown USB speed");
    }
    return ErrorCode::NONE;
}

DsoControl::BestSamplerateResult DsoControl::computeBestSamplerate(double samplerate, bool maximum) const {
    BestSamplerateResult r;

    // Abort if the input value is invalid
    if (samplerate <= 0.0) return r;

    // When possible, enable fast rate if it is required to reach the requested
    // samplerate
    r.fastrate = m_specification->supportsFastRate && (m_channelUsage.countUsedChannels() <= 1) &&
                 (samplerate > maxSingleChannelSamplerate());

    // Get samplerate specifications for this mode and model
    const ControlSamplerateLimits *limits;
    if (r.fastrate)
        limits = &(m_specification->fastrateSamplerate);
    else
        limits = &(m_specification->normalSamplerate);

    // Get downsampling factor that would provide the requested rate
    r.downsampler = limits->computeDownsampler(m_settings->recordLengthId(), samplerate);
    // Base samplerate sufficient, or is the maximum better?
    if (r.downsampler < 1.0 && (samplerate <= limits->maxSamplerate(m_settings->recordLengthId()) || !maximum)) {
        r.downsampler = 0.0;
        r.samplerate = limits->maxSamplerate(m_settings->recordLengthId());
    } else {
        switch (m_specification->cmdSetSamplerate) {
        case BulkCode::SETTRIGGERANDSAMPLERATE:
            // DSO-2090 supports the downsampling factors 1, 2, 4 and 5 using
            // valueFast or all even values above using valueSlow
            if ((maximum && r.downsampler <= 5.0) || (!maximum && r.downsampler < 6.0)) {
                // valueFast is used
                if (maximum) {
                    // The samplerate shall not be higher, so we round up
                    r.downsampler = ceil(r.downsampler);
                    if (r.downsampler > 2.0) // 3 and 4 not possible with the DSO-2090
                        r.downsampler = 5.0;
                } else {
                    // The samplerate shall not be lower, so we round down
                    r.downsampler = floor(r.downsampler);
                    if (r.downsampler > 2.0 && r.downsampler < 5.0) // 3 and 4 not possible with the DSO-2090
                        r.downsampler = 2.0;
                }
            } else {
                // valueSlow is used
                if (maximum) {
                    r.downsampler = ceil(r.downsampler / 2.0) * 2.0; // Round up to next even value
                } else {
                    r.downsampler = floor(r.downsampler / 2.0) * 2.0; // Round down to next even value
                }
                if (r.downsampler > 2.0 * 0x10001) // Check for overflow
                    r.downsampler = 2.0 * 0x10001;
            }
            break;

        case BulkCode::CSETTRIGGERORSAMPLERATE:
            // DSO-5200 may not supports all downsampling factors, requires testing
            if (maximum) {
                r.downsampler = ceil(r.downsampler); // Round up to next integer value
            } else {
                r.downsampler = floor(r.downsampler); // Round down to next integer value
            }
            break;

        case BulkCode::ESETTRIGGERORSAMPLERATE:
            // DSO-2250 doesn't have a fast value, so it supports all downsampling
            // factors
            if (maximum) {
                r.downsampler = ceil(r.downsampler); // Round up to next integer value
            } else {
                r.downsampler = floor(r.downsampler); // Round down to next integer value
            }
            break;

        default:
            return r;
        }

        // Limit maximum downsampler value to avoid overflows in the sent commands
        if (r.downsampler > limits->maxDownsampler) r.downsampler = limits->maxDownsampler;
        r.samplerate = limits->base / r.downsampler / limits->recordLengths[m_settings->recordLengthId()].bufferDivider;
    }
    return r;
}

unsigned DsoControl::updateSamplerate(unsigned downsampler, bool fastRate) {
    fastRate |= m_specification->supportsFastRate;
    // Get samplerate limits
    const ControlSamplerateLimits *limits =
        fastRate ? &m_specification->normalSamplerate : &m_specification->fastrateSamplerate;
    // Update settings
    bool fastRateChanged = m_settings->limits != limits;
    if (fastRateChanged) { m_settings->limits = limits; }

    // Set the calculated samplerate
    switch (m_specification->cmdSetSamplerate) {
    case BulkCode::SETTRIGGERANDSAMPLERATE: {
        short int downsamplerValue = 0;
        unsigned char samplerateId = 0;
        bool downsampling = false;

        if (downsampler <= 5) {
            // All dividers up to 5 are done using the special samplerate IDs
            if (downsampler == 0 && limits->base >= limits->max)
                samplerateId = 1;
            else if (downsampler <= 2)
                samplerateId = downsampler;
            else { // Downsampling factors 3 and 4 are not supported
                samplerateId = 3;
                downsampler = 5;
                downsamplerValue = (short int)0xffff;
            }
        } else {
            // For any dividers above the downsampling factor can be set directly
            downsampler &= ~0x0001; // Only even values possible
            downsamplerValue = (short int)(0x10001 - (downsampler >> 1));

            downsampling = true;
        }

        // Pointers to needed commands
        BulkSetTriggerAndSamplerate *commandSetTriggerAndSamplerate =
            modifyCommand<BulkSetTriggerAndSamplerate>(BulkCode::SETTRIGGERANDSAMPLERATE);

        // Store if samplerate ID or downsampling factor is used
        commandSetTriggerAndSamplerate->setDownsamplingMode(downsampling);
        // Store samplerate ID
        commandSetTriggerAndSamplerate->setSamplerateId(samplerateId);
        // Store downsampling factor
        commandSetTriggerAndSamplerate->setDownsampler(downsamplerValue);
        // Set fast rate when used
        commandSetTriggerAndSamplerate->setFastRate(false /*fastRate*/);

        break;
    }
    case BulkCode::CSETTRIGGERORSAMPLERATE: {
        // Split the resulting divider into the values understood by the device
        // The fast value is kept at 4 (or 3) for slow sample rates
        long int valueSlow = qMax(((long int)downsampler - 3) / 2, (long int)0);
        unsigned char valueFast = downsampler - valueSlow * 2;

        // Pointers to needed commands
        BulkSetSamplerate5200 *commandSetSamplerate5200 =
            modifyCommand<BulkSetSamplerate5200>(BulkCode::CSETTRIGGERORSAMPLERATE);
        BulkSetTrigger5200 *commandSetTrigger5200 =
            modifyCommand<BulkSetTrigger5200>(BulkCode::ESETTRIGGERORSAMPLERATE);

        // Store samplerate fast value
        commandSetSamplerate5200->setSamplerateFast(4 - valueFast);
        // Store samplerate slow value (two's complement)
        commandSetSamplerate5200->setSamplerateSlow(valueSlow == 0 ? 0 : 0xffff - valueSlow);
        // Set fast rate when used
        commandSetTrigger5200->setFastRate(fastRate);

        break;
    }
    case BulkCode::ESETTRIGGERORSAMPLERATE: {
        // Pointers to needed commands
        BulkSetSamplerate2250 *commandSetSamplerate2250 =
            modifyCommand<BulkSetSamplerate2250>(BulkCode::ESETTRIGGERORSAMPLERATE);

        bool downsampling = downsampler >= 1;
        // Store downsampler state value
        commandSetSamplerate2250->setDownsampling(downsampling);
        // Store samplerate value
        commandSetSamplerate2250->setSamplerate(downsampler > 1 ? 0x10001 - downsampler : 0);
        // Set fast rate when used
        commandSetSamplerate2250->setFastRate(fastRate);

        break;
    }
    default:
        return UINT_MAX;
    }

    m_settings->downsampler = downsampler;
    double samplerate;
    if (downsampler)
        samplerate = m_settings->limits->samplerate(m_settings->recordLengthId(), downsampler);
    else
        samplerate = m_settings->limits->maxSamplerate(m_settings->recordLengthId());

    const double timebase = computeTimebase(samplerate);
    m_settings->updateCurrentSamplerate(samplerate, timebase, UINT_MAX);

    // Update dependencies
    setPretriggerPosition(m_settings->trigger.position());

    // Emit signals for changed settings
    if (fastRateChanged) {
        std::vector<unsigned> r;
        emit m_settings->availableRecordLengthsChanged(m_settings->limits->recordLengths);
        emit m_settings->recordLengthChanged(m_settings->recordLengthId());
    }

    emit m_settings->samplerateChanged(m_settings->samplerate());

    return downsampler;
}

void DsoControl::restoreTargets() {
    switch (m_settings->samplerateSource()) {
    case SamplerateSource::Samplerrate:
        setSamplerate(m_settings->target().samplerate);
        break;
    case SamplerateSource::FixedSamplerate:
        setFixedSamplerate(m_settings->target().fixedSamperateId);
        break;
    case SamplerateSource::Duration:
        setRecordTime(m_settings->target().timebase);
        break;
    }
}

void DsoControl::notifySamplerateLimits() {
    if (m_specification->isFixedSamplerateDevice) {
        emit m_settings->fixedSampleratesChanged(m_specification->fixedSampleRates);
    } else {
        emit m_settings->samplerateLimitsChanged(minSamplerate(), maxSamplerate());
    }
}

Dso::ErrorCode DsoControl::setRecordLengthByIndex(RecordLengthID index) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;
    const auto &recLengths = m_settings->limits->recordLengths;
    if (index >= recLengths.size()) return Dso::ErrorCode::PARAMETER;
    if (recLengths[index].recordLength == ROLL_RECORDLEN) return Dso::ErrorCode::PARAMETER;
    QMutexLocker l(&m_commandMutex);

    switch (m_specification->cmdSetRecordLength) {
    case BulkCode::SETTRIGGERANDSAMPLERATE:
        modifyCommand<BulkSetTriggerAndSamplerate>(BulkCode::SETTRIGGERANDSAMPLERATE)->setRecordLength((uint8_t)index);
        break;

    case BulkCode::DSETBUFFER:
        if (m_specification->cmdSetPretrigger == BulkCode::FSETBUFFER) {
            modifyCommand<BulkSetRecordLength2250>(BulkCode::DSETBUFFER)->setRecordLength((uint8_t)index);
        } else {
            // SetBuffer5200 bulk command for record length
            BulkSetBuffer5200 *commandSetBuffer5200 = modifyCommand<BulkSetBuffer5200>(BulkCode::DSETBUFFER);

            commandSetBuffer5200->setUsedPre(DTriggerPositionUsed::ON);
            commandSetBuffer5200->setUsedPost(DTriggerPositionUsed::ON);
            commandSetBuffer5200->setRecordLength((uint8_t)index);
        }

        break;

    default:
        return Dso::ErrorCode::PARAMETER;
    }

    // Check if the divider has changed and adapt samplerate limits accordingly
    bool bDividerChanged = recLengths[index].bufferDivider != recLengths[m_settings->recordLengthId()].bufferDivider;

    m_settings->setRecordLengthId(index);

    if (bDividerChanged) {
        notifySamplerateLimits();
        // Samplerate dividers changed, recalculate it
        restoreTargets();
        setPretriggerPosition(m_settings->trigger.position());
    }

    return Dso::ErrorCode::NONE;
}

Dso::ErrorCode DsoControl::setFixedSamplerate(unsigned samplerateId) {
    if (!m_specification->isFixedSamplerateDevice) return Dso::ErrorCode::PARAMETER;
    if (samplerateId > m_specification->fixedSampleRates.size()) return Dso::ErrorCode::PARAMETER;
    QMutexLocker l(&m_commandMutex);

    m_settings->updateTarget(SamplerateSource::FixedSamplerate).fixedSamperateId = samplerateId;

    modifyCommand<ControlSetTimeDIV>(ControlCode::SETTIMEDIV)
        ->setDiv(m_specification->fixedSampleRates[samplerateId].id);

    const double samplerate = m_specification->fixedSampleRates[samplerateId].samplerate;
    const double timebase = computeTimebase(samplerate);
    m_settings->updateCurrentSamplerate(samplerate, timebase, samplerateId);

    // Update dependencies
    setPretriggerPosition(m_settings->trigger.position());

    // Check for Roll mode
    emit m_settings->samplerateChanged(m_settings->samplerate());

    return Dso::ErrorCode::NONE;
}

Dso::ErrorCode DsoControl::setSamplerate(double samplerate) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;
    if (m_specification->isFixedSamplerateDevice) return Dso::ErrorCode::PARAMETER;
    QMutexLocker l(&m_commandMutex);

    m_settings->updateTarget(SamplerateSource::Samplerrate).samplerate = samplerate;

    // What is the nearest, at least as high samplerate the scope can provide?
    BestSamplerateResult r = computeBestSamplerate(samplerate, false);

    // Set the calculated samplerate
    if (updateSamplerate(r.downsampler, r.fastrate) == UINT_MAX)
        return Dso::ErrorCode::PARAMETER;
    else {
        return Dso::ErrorCode::NONE;
    }
}

Dso::ErrorCode DsoControl::setRecordTime(double duration) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;
    QMutexLocker l(&m_commandMutex);

    m_settings->updateTarget(SamplerateSource::Duration).timebase = duration;

    if (!m_specification->isFixedSamplerateDevice) {
        // Calculate the maximum samplerate that would still provide the requested
        // duration
        double maxSamplerate = m_specification->normalSamplerate.samplerate(m_settings->recordLengthId(), duration);

        // When possible, enable fast rate if the record time can't be set that low
        // to improve resolution
        bool fastRate = m_specification->supportsFastRate && (m_channelUsage.countUsedChannels() <= 1) &&
                        (maxSamplerate >=
                         m_specification->fastrateSamplerate.samplerate(m_settings->recordLengthId(), (unsigned)1));

        // What is the nearest, at most as high samplerate the scope can provide?
        unsigned downsampler = 0;

        // Set the calculated samplerate
        if (this->updateSamplerate(downsampler, fastRate) == UINT_MAX)
            return Dso::ErrorCode::PARAMETER;
        else {
            return Dso::ErrorCode::NONE;
        }
    } else {
        unsigned sampleCount = m_settings->getRecordLength();
        // Ensure that at least 1/2 of remaining samples are available for SW trigger algorithm
        if (m_specification->isSoftwareTriggerDevice) sampleCount -= m_settings->trigger.swSampleMargin();
        unsigned samplerateId = 0;
        double diff = UINT_MAX;
        for (unsigned i = 0; i < m_specification->fixedSampleRates.size(); ++i) {
            double d = std::abs(m_specification->fixedSampleRates[i].samplerate * duration - sampleCount);
            if (d < diff) {
                diff = d;
                samplerateId = i;
            }
        }

        // Usable sample value
        modifyCommand<ControlSetTimeDIV>(ControlCode::SETTIMEDIV)
            ->setDiv(m_specification->fixedSampleRates[samplerateId].id);

        const double samplerate = m_specification->fixedSampleRates[samplerateId].samplerate;
        const double timebase = computeTimebase(samplerate);
        m_settings->updateCurrentSamplerate(samplerate, timebase, samplerateId);

        // Update dependencies
        setPretriggerPosition(m_settings->trigger.position());

        emit m_settings->samplerateChanged(m_settings->samplerate());
        return Dso::ErrorCode::NONE;
    }
}

Dso::ErrorCode DsoControl::checkChannelUsage() {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;

    // Calculate the UsedChannels field for the command
    UsedChannels usedChannels = m_channelUsage.isUsed(0) ? UsedChannels::USED_CH1 : UsedChannels::USED_NONE;

    if (m_channelUsage.isUsed(1)) {
        if (m_channelUsage.isUsed(0)) {
            usedChannels = UsedChannels::USED_CH1CH2;
        } else {
            // DSO-2250 uses a different value for channel 2
            if (m_specification->cmdSetChannels == BulkCode::BSETCHANNELS)
                usedChannels = UsedChannels::BUSED_CH2;
            else
                usedChannels = UsedChannels::USED_CH2;
        }
    }

    switch (m_specification->cmdSetChannels) {
    case BulkCode::SETTRIGGERANDSAMPLERATE: {
        // SetTriggerAndSamplerate bulk command for trigger source
        modifyCommand<BulkSetTriggerAndSamplerate>(BulkCode::SETTRIGGERANDSAMPLERATE)
            ->setUsedChannels((uint8_t)usedChannels);
        break;
    }
    case BulkCode::BSETCHANNELS: {
        // SetChannels2250 bulk command for active channels
        modifyCommand<BulkSetChannels2250>(BulkCode::BSETCHANNELS)->setUsedChannels((uint8_t)usedChannels);
        break;
    }
    case BulkCode::ESETTRIGGERORSAMPLERATE: {
        // SetTrigger5200s bulk command for trigger source
        modifyCommand<BulkSetTrigger5200>(BulkCode::ESETTRIGGERORSAMPLERATE)->setUsedChannels((uint8_t)usedChannels);
        break;
    }
    default:
        break;
    }

    notifySamplerateLimits();

    return Dso::ErrorCode::NONE;
}

Dso::ErrorCode DsoControl::setCoupling(ChannelID channel, Dso::Coupling coupling) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;
    if (channel >= m_specification->channels) return Dso::ErrorCode::PARAMETER;
    QMutexLocker l(&m_commandMutex);

    unsigned index = INT_MAX;
    for (unsigned i = 0; i < m_specification->couplings.size(); ++i)
        if (m_specification->couplings[i] == coupling) {
            index = i;
            break;
        }
    if (index == INT_MAX) return Dso::ErrorCode::PARAMETER;

    // SetRelays control command for coupling relays
    if (m_specification->supportsCouplingRelays) {
        modifyCommand<ControlSetRelays>(ControlCode::SETRELAYS)->setCoupling(channel, coupling != Dso::Coupling::AC);
    }

    m_settings->voltage[channel]->setCouplingIndex(index);

    return Dso::ErrorCode::NONE;
}

Dso::ErrorCode DsoControl::setGain(ChannelID channel, unsigned gainId, bool overwrite) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;
    if (channel >= m_specification->channels) return Dso::ErrorCode::PARAMETER;
    if (gainId >= m_specification->gain.size()) return Dso::ErrorCode::PARAMETER;
    Channel *c = m_settings->voltage[channel];
    if (!overwrite && c->gainStepIndex() == gainId) return Dso::ErrorCode::UNCHANGED;
    QMutexLocker l(&m_commandMutex);

    const ControlSpecificationGainLevel &gain = m_specification->gain[gainId];

    if (m_specification->useControlNoBulk) {
        if (channel == 0) {
            modifyCommand<ControlSetVoltDIV_CH1>(ControlCode::SETVOLTDIV_CH1)->setDiv(gain.gainIdentificator);
        } else if (channel == 1) {
            modifyCommand<ControlSetVoltDIV_CH2>(ControlCode::SETVOLTDIV_CH2)->setDiv(gain.gainIdentificator);
        } else
            qDebug("%s: Unsuported channel: %i\n", __func__, channel);
    } else {
        modifyCommand<BulkSetGain>(BulkCode::SETGAIN)->setGain(channel, gain.gainIdentificator);

        // SetRelays control command for gain relays
        ControlSetRelays *controlSetRelays = modifyCommand<ControlSetRelays>(ControlCode::SETRELAYS);
        controlSetRelays->setBelow1V(channel, gainId < 3); // TODO That needs to be changed to rely on spec information
        controlSetRelays->setBelow100mV(channel, gainId < 6);
    }

    c->setGainStepIndex(gainId);
    setOffset(channel, c->offset(), overwrite);
    setTriggerOffset(channel, c->triggerLevel(), overwrite);

    return Dso::ErrorCode::NONE;
}

Dso::ErrorCode DsoControl::setOffset(ChannelID channel, double offset, bool overwrite) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;
    if (offset < -1 || offset > 1) return Dso::ErrorCode::PARAMETER;
    if (channel >= m_specification->channels) return Dso::ErrorCode::PARAMETER;
    Channel *c = m_settings->voltage[channel];
    if (!overwrite && c->offset() == offset) return Dso::ErrorCode::UNCHANGED;
    QMutexLocker l(&m_commandMutex);

    if (m_specification->supportsOffset) {
        const auto &channelOffLimit = m_specification->calibration[channel][c->gainStepIndex()];
        const uint16_t minimum = channelOffLimit.offsetStart;
        const uint16_t maximum = channelOffLimit.offsetEnd;
        const uint16_t range = maximum - minimum;

        // Offset is at the moment in range [-1.0,1.0] but we need it as [0.0,1.0]
        const double normalizedOffset = (offset + 1) / 2.0;
        // We are now in [0,1] but hardware wants it in [min,max]
        const uint16_t offsetValue = (uint16_t)std::ceil(normalizedOffset * range + minimum);
        modifyCommand<ControlSetOffset>(ControlCode::SETOFFSET)->setOffset(channel, offsetValue);
        /// Due to uint16_ts limited resolution,
        /// the hardware applied offset is a little off compared to the given offset. We need to store it as well
        /// to later compensate for it on the received sampleset.
        c->setOffset(offset, std::ceil(offset * range) / (double)range);
        DBGNOTIFY(QString("HardOffset c:%1,l:%2").arg(channel).arg(c->offsetHardware()),
                  Debug::NotificationType::DSOControl);
    } else {
        c->setOffset(offset, 0);
        DBGNOTIFY(QString("SoftOffset c:%1,l:%2").arg(channel).arg(c->offset()), Debug::NotificationType::DSOControl);
    }

    return Dso::ErrorCode::NONE;
}

Dso::ErrorCode DsoControl::setTriggerMode(Dso::TriggerMode mode) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;
    QMutexLocker l(&m_commandMutex);

    m_settings->trigger.setMode(mode);
    return Dso::ErrorCode::NONE;
}

Dso::ErrorCode DsoControl::setTriggerSource(bool special, ChannelID channel) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;
    QMutexLocker l(&m_commandMutex);

    if (m_specification->isSoftwareTriggerDevice) {
        DBGNOTIFY(QString("TriggerSrc %1").arg(channel), Debug::NotificationType::DSOControl);
        m_settings->trigger.setTriggerSource(channel, special);
        return Dso::ErrorCode::NONE;
    }

    if (!special && channel >= m_specification->channels) return Dso::ErrorCode::PARAMETER;

    if (special && channel >= m_specification->specialTriggerChannels.size()) return Dso::ErrorCode::PARAMETER;

    int hardwareID = special ? m_specification->specialTriggerChannels[channel].hardwareID : (int)channel;

    switch (m_specification->cmdSetTrigger) {
    case BulkCode::SETTRIGGERANDSAMPLERATE:
        // SetTriggerAndSamplerate bulk command for trigger source
        modifyCommand<BulkSetTriggerAndSamplerate>(BulkCode::SETTRIGGERANDSAMPLERATE)->setTriggerSource(1 - hardwareID);
        break;

    case BulkCode::CSETTRIGGERORSAMPLERATE:
        // SetTrigger2250 bulk command for trigger source
        modifyCommand<BulkSetTrigger2250>(BulkCode::CSETTRIGGERORSAMPLERATE)->setTriggerSource(2 + hardwareID);
        break;

    case BulkCode::ESETTRIGGERORSAMPLERATE:
        // SetTrigger5200 bulk command for trigger source
        modifyCommand<BulkSetTrigger5200>(BulkCode::ESETTRIGGERORSAMPLERATE)->setTriggerSource(1 - hardwareID);
        break;

    default:
        return Dso::ErrorCode::UNSUPPORTED;
    }

    // SetRelays control command for external trigger relay
    modifyCommand<ControlSetRelays>(ControlCode::SETRELAYS)->setTrigger(special);

    DBGNOTIFY(QString("TriggerSrc %1").arg(channel), Debug::NotificationType::DSOControl);
    m_settings->trigger.setTriggerSource(channel, special);

    // Apply trigger level of the new source
    if (special) {
        // SetOffset control command for changed trigger level
        modifyCommand<ControlSetOffset>(ControlCode::SETOFFSET)->setTriggerLevel(0x7f);
    } else
        this->setTriggerOffset(channel, m_settings->voltage[channel]->triggerLevel());

    return Dso::ErrorCode::NONE;
}

Dso::ErrorCode DsoControl::setTriggerOffset(ChannelID channel, double offset, bool overwrite) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;
    if (offset < -1 || offset > 1) return Dso::ErrorCode::PARAMETER;
    if (channel >= m_specification->channels) return Dso::ErrorCode::PARAMETER;

    Channel *c = m_settings->voltage[channel];
    if (!overwrite && c->triggerLevel() == offset) return Dso::ErrorCode::UNCHANGED;
    QMutexLocker l(&m_commandMutex);

    c->setTriggerOffset(offset);

    if (!m_specification->isSoftwareTriggerDevice) {
        const auto &channelOffLimit = m_specification->calibration[channel][c->gainStepIndex()];
        const uint16_t minimum = channelOffLimit.offsetStart;
        const uint16_t maximum = channelOffLimit.offsetEnd;
        const uint16_t range = maximum - minimum;

        // Offset is at the moment in range [-1.0,1.0] but we need it as [0.0,1.0]
        const double normalizedOffset = (offset + 1) / 2.0;
        // We are now in [0,1] but hardware wants it in [min,max]
        const uint16_t offsetValue = (uint16_t)std::ceil(normalizedOffset * range + minimum);
        modifyCommand<ControlSetOffset>(ControlCode::SETOFFSET)->setTriggerLevel(offsetValue);
        DBGNOTIFY(QString("HardTriggerLevel c:%1,l:%2").arg(channel).arg(offsetValue),
                  Debug::NotificationType::DSOControl);
    } else {
        DBGNOTIFY(QString("SoftTriggerLevel c:%1,l:%2").arg(channel).arg(c->triggerLevel()),
                  Debug::NotificationType::DSOControl);
    }

    return Dso::ErrorCode::NONE;
}

Dso::ErrorCode DsoControl::setTriggerSlope(Dso::Slope slope) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;
    QMutexLocker l(&m_commandMutex);

    switch (m_specification->cmdSetTrigger) {
    case BulkCode::SETTRIGGERANDSAMPLERATE: {
        // SetTriggerAndSamplerate bulk command for trigger slope
        modifyCommand<BulkSetTriggerAndSamplerate>(BulkCode::SETTRIGGERANDSAMPLERATE)->setTriggerSlope((uint8_t)slope);
        break;
    }
    case BulkCode::CSETTRIGGERORSAMPLERATE: {
        // SetTrigger2250 bulk command for trigger slope
        modifyCommand<BulkSetTrigger2250>(BulkCode::CSETTRIGGERORSAMPLERATE)->setTriggerSlope((uint8_t)slope);
        break;
    }
    case BulkCode::ESETTRIGGERORSAMPLERATE: {
        // SetTrigger5200 bulk command for trigger slope
        modifyCommand<BulkSetTrigger5200>(BulkCode::ESETTRIGGERORSAMPLERATE)->setTriggerSlope((uint8_t)slope);
        break;
    }
    default:
        if (!m_specification->isSoftwareTriggerDevice) return Dso::ErrorCode::UNSUPPORTED;
    }

    DBGNOTIFY(QString("TriggerSlope %1").arg((int)slope), Debug::NotificationType::DSOControl);
    m_settings->trigger.setSlope(slope);
    return Dso::ErrorCode::NONE;
}

void DsoControl::forceTrigger() { modifyCommand<BulkCommand>(BulkCode::FORCETRIGGER); }

ErrorCode DsoControl::setPretriggerPosition(double position, bool overwrite) {
    if (!device->isConnected()) return Dso::ErrorCode::CONNECTION;
    if (!overwrite && m_settings->trigger.position() == position) return Dso::ErrorCode::NONE;
    Samples positionSamples = position * m_settings->samplerate().samplerate;
    QMutexLocker l(&m_commandMutex);

    // All trigger positions are measured in samples
    unsigned recordLength = m_settings->getRecordLength();
    // Fast rate mode uses both channels
    if (m_settings->isFastRate()) positionSamples /= m_specification->channels;

    switch (m_specification->cmdSetPretrigger) {
    case BulkCode::SETTRIGGERANDSAMPLERATE: {
        // Calculate the position value (Start point depending on record length)
        unsigned triggerPosition = m_settings->isRollMode() ? 0x1 : 0x7ffff - recordLength + (unsigned)positionSamples;

        // SetTriggerAndSamplerate bulk command for trigger position
        modifyCommand<BulkSetTriggerAndSamplerate>(BulkCode::SETTRIGGERANDSAMPLERATE)
            ->setTriggerPosition(triggerPosition);
        DBGNOTIFY(QString("TriggerPosition %1").arg(position), Debug::NotificationType::DSOControl);
        break;
    }
    case BulkCode::FSETBUFFER: {
        // Calculate the position values (Inverse, maximum is 0x7ffff)
        unsigned positionPre = 0x7ffff - recordLength + (unsigned)positionSamples;
        unsigned positionPost = 0x7ffff - (unsigned)positionSamples;

        // SetBuffer2250 bulk command for trigger position
        BulkSetBuffer2250 *commandSetBuffer2250 = modifyCommand<BulkSetBuffer2250>(BulkCode::FSETBUFFER);
        commandSetBuffer2250->setTriggerPositionPre(positionPre);
        commandSetBuffer2250->setTriggerPositionPost(positionPost);
        DBGNOTIFY(QString("TriggerPosition %1").arg(position), Debug::NotificationType::DSOControl);
        break;
    }
    case BulkCode::ESETTRIGGERORSAMPLERATE: {
        // Calculate the position values (Inverse, maximum is 0xffff)
        unsigned positionPre = 0xffff - recordLength + (unsigned)positionSamples;
        unsigned positionPost = 0xffff - (unsigned)positionSamples;

        // SetBuffer5200 bulk command for trigger position
        BulkSetBuffer5200 *commandSetBuffer5200 = modifyCommand<BulkSetBuffer5200>(BulkCode::DSETBUFFER);
        commandSetBuffer5200->setTriggerPositionPre((uint16_t)positionPre);
        commandSetBuffer5200->setTriggerPositionPost((uint16_t)positionPost);
        DBGNOTIFY(QString("TriggerPosition %1").arg(position), Debug::NotificationType::DSOControl);
        break;
    }
    default:
        if (!m_specification->isSoftwareTriggerDevice) return Dso::ErrorCode::UNSUPPORTED;
        DBGNOTIFY(QString("SoftTriggerPosition %1").arg(position), Debug::NotificationType::DSOControl);
    }

    m_settings->trigger.setPosition(position);
    return Dso::ErrorCode::NONE;
}
