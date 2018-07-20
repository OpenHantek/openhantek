// SPDX-License-Identifier: GPL-2.0+

#include "dsoloop.h"

#include "dsocontrol.h"
#include "viewconstants.h"
#include "models/modelDSO6022.h"
#include "usb/usbdevice.h"
#include <QDebug>
#include <QTimer>

#ifdef DEBUG
#define DBGNOTIFY(x, y) emit m_control->debugMessage(x, y)
#else
#define DBGNOTIFY(x, y)
#endif

#if __has_cpp_attribute(clang::fallthrough)
#define FALLTHROUGH [[clang::fallthrough]];
#elif __has_cpp_attribute(fallthrough)
#define FALLTHROUGH [[fallthrough]];
#else
#define FALLTHROUGH
#endif

using namespace Hantek;
using namespace std::chrono;

DsoLoop::DsoLoop(std::shared_ptr<Dso::DeviceSettings> settings, DsoControl *control)
    : m_specification(settings->spec), m_settings(settings), m_modelID(control->device->getModel()->ID),
      result(settings->spec->channels), m_control(control) {}

void DsoLoop::runRollMode() {
    if (!m_control->sendPendingCommands()) return;

    int errorCode = 0;
    captureState = CAPTURE_WAITING;
    bool toNextState = true;

    switch (this->rollState) {
    case RollState::STARTSAMPLING:
        // Don't iterate through roll mode steps when stopped
        if (!this->sampling) {
            toNextState = false;
            break;
        }

        // Sampling hasn't started, update the expected sample count
        expectedSampleCount = m_settings->getSampleCount();

        errorCode = m_control->bulkCommand(m_control->getCommand(HantekE::BulkCode::STARTSAMPLING));
        if (errorCode < 0) {
            if (errorCode == LIBUSB_ERROR_NO_DEVICE) {
                emit m_control->communicationError();
                return;
            }
            break;
        }

        DBGNOTIFY("Starting to capture", Debug::NotificationType::DSOLoop);

        this->_samplingStarted = true;

        break;

    case RollState::ENABLETRIGGER:
        errorCode = m_control->bulkCommand(m_control->getCommand(HantekE::BulkCode::ENABLETRIGGER));
        if (errorCode < 0) {
            if (errorCode == LIBUSB_ERROR_NO_DEVICE) {
                emit m_control->communicationError();
                return;
            }
            break;
        }

        DBGNOTIFY("Enabling trigger", Debug::NotificationType::DSOLoop);

        break;

    case RollState::FORCETRIGGER:
        errorCode = m_control->bulkCommand(m_control->getCommand(HantekE::BulkCode::FORCETRIGGER));
        if (errorCode < 0) {
            if (errorCode == LIBUSB_ERROR_NO_DEVICE) {
                emit m_control->communicationError();
                return;
            }
            break;
        }

        DBGNOTIFY("Forcing trigger", Debug::NotificationType::DSOControl);

        break;

    case RollState::GETDATA: {
        m_control->retrieveSamples(expectedSampleCount);
        if (this->_samplingStarted) {
            convertRawDataToSamples(m_control->m_rawdata);
            emit m_control->samplesAvailable(&result);
        }
    }

        // Check if we're in single trigger mode
        if (m_settings->trigger.mode() == DsoE::TriggerMode::SINGLE && this->_samplingStarted)
            this->enableSampling(false);

        // Sampling completed, restart it when necessary
        this->_samplingStarted = false;

        break;

    default:
        DBGNOTIFY("Roll mode state unknown", Debug::NotificationType::DSOControl);
        break;
    }

    // Go to next state, or restart if last state was reached
    if (toNextState) this->rollState = (RollState)(((int)rollState + 1) % (int)RollState::_COUNT);

    this->updateInterval();
    // TODO Qt5.8: Use chrone cycletime directly
    QTimer::singleShot(cycleTime.count(), this,
                       m_settings->isRollMode() ? &DsoLoop::runRollMode : &DsoLoop::runStandardMode);
}

void DsoLoop::runStandardMode() {
    if (!m_control->sendPendingCommands()) return;

    int errorCode = 0;
    this->rollState = RollState::STARTSAMPLING;

    const int lastCaptureState = this->captureState;
    unsigned triggerPoint;
    std::tie(captureState, triggerPoint) = m_control->retrieveCaptureState();
    m_settings->trigger.setPoint(calculateTriggerPoint(triggerPoint));
    if (this->captureState < 0) {
        const QString errMsg = QString("Getting capture state failed: %1").arg(libUsbErrorString(this->captureState));
        qWarning() << errMsg;
        DBGNOTIFY(errMsg, Debug::NotificationType::DSOControl);
    } else if (this->captureState != lastCaptureState)
        DBGNOTIFY(QString("Capture state changed to %1").arg(this->captureState), Debug::NotificationType::DSOLoop);

    switch (this->captureState) {
    case CAPTURE_READY:
    case CAPTURE_READY2250:
    case CAPTURE_READY5200: {
        m_control->retrieveSamples(expectedSampleCount);
        if (this->_samplingStarted) {
            convertRawDataToSamples(m_control->m_rawdata);
            emit m_control->samplesAvailable(&result);
        }
    }

        // Check if we're in single trigger mode
        if (m_settings->trigger.mode() == DsoE::TriggerMode::SINGLE && this->_samplingStarted)
            this->enableSampling(false);

        // Sampling completed, restart it when necessary
        this->_samplingStarted = false;

        // Start next capture if necessary by leaving out the break statement

        if (!this->sampling)
            break;
        else {
            FALLTHROUGH
        }
    case CAPTURE_WAITING:
        // Sampling hasn't started, update the expected sample count
        expectedSampleCount = m_settings->getSampleCount();

        if (!m_specification->useControlNoBulk) {
            if (_samplingStarted && lastTriggerMode == m_settings->trigger.mode()) {
                ++this->cycleCounter;

                if (this->cycleCounter == this->startCycle && !m_settings->isRollMode()) {
                    // Buffer refilled completely since start of sampling, enable the
                    // trigger now
                    errorCode = m_control->bulkCommand(m_control->getCommand(HantekE::BulkCode::ENABLETRIGGER));
                    if (errorCode < 0) {
                        if (errorCode == LIBUSB_ERROR_NO_DEVICE) {
                            emit m_control->communicationError();
                            return;
                        }
                        break;
                    }

                    DBGNOTIFY("Enabling trigger", Debug::NotificationType::DSOLoop);
                } else if (cycleCounter >= 8ms + startCycle &&
                           m_settings->trigger.mode() == DsoE::TriggerMode::WAIT_FORCE) {
                    // Force triggering
                    errorCode = m_control->bulkCommand(m_control->getCommand(HantekE::BulkCode::FORCETRIGGER));
                    if (errorCode < 0) {
                        if (errorCode == LIBUSB_ERROR_NO_DEVICE) {
                            emit m_control->communicationError();
                            return;
                        }
                        break;
                    }

                    DBGNOTIFY("Forcing trigger", Debug::NotificationType::DSOLoop);
                }

                if (cycleCounter < 20ms || cycleCounter < milliseconds(4000 / cycleTime.count())) break;
            }

            // Start capturing
            errorCode = m_control->bulkCommand(m_control->getCommand(HantekE::BulkCode::STARTSAMPLING));
            if (errorCode < 0) {
                if (errorCode == LIBUSB_ERROR_NO_DEVICE) {
                    emit m_control->communicationError();
                    return;
                }
                break;
            }

            DBGNOTIFY("Starting to capture", Debug::NotificationType::DSOLoop);
        }

        this->_samplingStarted = true;
        this->cycleCounter = 0ms;
        this->startCycle = milliseconds(int(m_settings->trigger.position() * 1000 / cycleTime.count())) + 1ms;
        this->lastTriggerMode = m_settings->trigger.mode();
        break;

    case CAPTURE_SAMPLING:
        break;
    default:
        break;
    }

    this->updateInterval();
    // TODO Qt5.8: Use chrone cycletime directly
    QTimer::singleShot(cycleTime.count(), this,
                       m_settings->isRollMode() ? &DsoLoop::runRollMode : &DsoLoop::runStandardMode);
}

void DsoLoop::enableSampling(bool enabled) {
    sampling = enabled;
    emit m_control->samplingStatusChanged(enabled);
}

/// \brief Updates the interval of the periodic thread timer.
void DsoLoop::updateInterval() {
    // Check the current oscilloscope state everytime 25% of the time the buffer
    // should be refilled
    double recLen;
    if (m_settings->isRollMode())
        recLen = m_settings->packetSize() / (m_settings->isFastRate() ? 1 : m_specification->channels);
    else
        recLen = m_settings->getRecordLength();

    // Not more often than every 10 ms though but at least once every second
    cycleTime = std::chrono::milliseconds(qBound(10, int(recLen / m_settings->samplerate().samplerate * 250), 1000));
}

unsigned DsoLoop::calculateTriggerPoint(unsigned value) {
    unsigned result = value;

    // Each set bit inverts all bits with a lower value
    for (unsigned bitValue = 1; bitValue; bitValue <<= 1)
        if (result & bitValue) result ^= bitValue - 1;

    return result;
}

void DsoLoop::convertRawDataToSamples(const std::vector<unsigned char> &rawData) {
    const size_t totalSampleCount = (m_specification->sampleSize > 8) ? rawData.size() / 2 : rawData.size();

    QWriteLocker locker(&result.lock);

    const unsigned extraBitsSize = m_specification->sampleSize - 8;    // Number of extra bits
    const uint16_t extraBitsMask = (0x00ff << extraBitsSize) & 0xff00; // Mask for extra bits extraction

    // Convert channel data
    if (m_settings->isFastRate()) {
        result.prepareForWrite(1, m_settings->samplerate().samplerate, m_settings->isRollMode());

        // Fast rate mode, one channel is using all buffers. Find that channel
        ChannelID channelId = 0;
        for (; channelId < m_specification->channels; ++channelId) {
            if (m_control->m_channelUsage.isUsed(channelId)) break;
        }

        if (channelId >= m_specification->channels) return;

        // Resize sample vector
        result.data[0].id = channelId;
        DSOsamples::ChannelSamples &samples = result.data[0];
        samples.resize(totalSampleCount);

        const unsigned gainID = m_settings->voltage[channelId]->gainStepIndex();
        const double limit = m_specification->calibration[channelId][gainID].voltageLimit;
        const double offsetCorrection = m_specification->calibration[channelId][gainID].offsetCorrection;
        const double harwareOffset = m_settings->voltage[channelId]->offsetHardware();
        const double gainStep = m_specification->gain[gainID].gain;

        // Convert data from the oscilloscope and write it into the sample buffer
        unsigned bufferPosition = m_settings->trigger.point() * 2;
        if (m_specification->sampleSize > 8) {
            for (unsigned pos = 0; pos < totalSampleCount; ++pos, ++bufferPosition) {
                if (bufferPosition >= totalSampleCount) bufferPosition %= totalSampleCount;

                const uint16_t low = rawData[bufferPosition];
                const unsigned extraBitsPosition = bufferPosition % m_specification->channels;
                const unsigned shift = (8 - (m_specification->channels - 1 - extraBitsPosition) * extraBitsSize);
                const uint16_t high =
                    ((uint16_t)rawData[totalSampleCount + bufferPosition - extraBitsPosition] << shift) & extraBitsMask;
                const uint16_t v = low + high;
                double samplePoint = (v / limit - harwareOffset) * gainStep - offsetCorrection;
                if (samplePoint < samples.minVoltage) samples.minVoltage = samplePoint;
                if (samplePoint > samples.maxVoltage) samples.maxVoltage = samplePoint;
                if (v < samples.minRaw) samples.minRaw = v;
                if (v > samples.maxRaw) samples.maxRaw = v;
                samples[pos] = samplePoint;
            }
        } else {
            for (unsigned pos = 0; pos < totalSampleCount; ++pos, ++bufferPosition) {
                if (bufferPosition >= totalSampleCount) bufferPosition %= totalSampleCount;
                const uint16_t v = rawData[bufferPosition];
                double samplePoint = (v / limit - harwareOffset) * gainStep - offsetCorrection;
                if (samplePoint < samples.minVoltage) samples.minVoltage = samplePoint;
                if (samplePoint > samples.maxVoltage) samples.maxVoltage = samplePoint;
                if (v < samples.minRaw) samples.minRaw = v;
                if (v > samples.maxRaw) samples.maxRaw = v;
                samples[pos] = samplePoint;
            }
        }
    } else {
        result.prepareForWrite(m_specification->channels, m_settings->samplerate().samplerate,
                               m_settings->isRollMode());

        // Normal mode, channels are using their separate buffers
        for (ChannelID channelId = 0; channelId < m_specification->channels; ++channelId) {
            DSOsamples::ChannelSamples &samples = result.data[channelId];
            samples.id = channelId;
            samples.resize(totalSampleCount / m_specification->channels);

            const unsigned gainID = m_settings->voltage[channelId]->gainStepIndex();
            const double limit = m_specification->calibration[channelId][gainID].voltageLimit;
            const double offsetCorrection = m_specification->calibration[channelId][gainID].offsetCorrection;
            const double harwareOffset = m_settings->voltage[channelId]->offsetHardware();
            const double gainStep = m_specification->gain[gainID].gain;
            int16_t shiftDataBuf = 0;

            // Convert data from the oscilloscope and write it into the sample buffer
            unsigned bufferPosition = m_settings->trigger.point() * 2;
            if (m_specification->sampleSize > 8) {
                // Additional most significant bits after the normal data
                const unsigned extraBitsIndex = 8 - channelId * 2; // Bit position offset for extra bits extraction
                const unsigned lowPosShift = m_specification->channels - 1 - channelId;

                for (unsigned pos = 0; pos < samples.size(); ++pos, bufferPosition += m_specification->channels) {
                    if (bufferPosition >= totalSampleCount) bufferPosition %= totalSampleCount;

                    const uint16_t low = rawData[bufferPosition + lowPosShift];
                    const uint16_t high =
                        ((uint16_t)rawData[bufferPosition + totalSampleCount] << extraBitsIndex) & extraBitsMask;

                    const uint16_t v = low + high;
                    double samplePoint = (v / limit - harwareOffset) * gainStep - offsetCorrection;
                    if (samplePoint < samples.minVoltage) samples.minVoltage = samplePoint;
                    if (samplePoint > samples.maxVoltage) samples.maxVoltage = samplePoint;
                    if (v < samples.minRaw) samples.minRaw = v;
                    if (v > samples.maxRaw) samples.maxRaw = v;
                    samples[pos] = samplePoint;
                }
            } else if (m_modelID == ModelDSO6022BE::ID) {
                // if device is 6022BE/BL, drop heading & trailing samples
                const unsigned DROP_DSO6022_HEAD = 0x410;
                const unsigned DROP_DSO6022_TAIL = 0x3F0;
                if (!m_settings->isRollMode()) {
                    samples.resize(samples.size() - (DROP_DSO6022_HEAD + DROP_DSO6022_TAIL));
                    // DROP_DSO6022_HEAD two times for two channels
                    bufferPosition += DROP_DSO6022_HEAD * 2;
                }
                bufferPosition += channelId;
                shiftDataBuf = 0x83;
            } else {
                bufferPosition += m_specification->channels - 1 - channelId;
            }
            for (unsigned pos = 0; pos < samples.size(); ++pos, bufferPosition += m_specification->channels) {
                if (bufferPosition >= totalSampleCount) bufferPosition %= totalSampleCount;
                const int16_t v = rawData[bufferPosition] - shiftDataBuf;
                double samplePoint = (v / limit - (harwareOffset+1)/2) * gainStep - offsetCorrection;
                if (samplePoint < samples.minVoltage) samples.minVoltage = samplePoint;
                if (samplePoint > samples.maxVoltage) samples.maxVoltage = samplePoint;
                if (v < samples.minRaw) samples.minRaw = v;
                if (v > samples.maxRaw) samples.maxRaw = v;
                samples[pos] = samplePoint;
            }
        }
    }
}
