// SPDX-License-Identifier: GPL-2.0+

#include "selfcalibration.h"
#include "dsocontrol.h"

SelfCalibration::SelfCalibration(DsoControl *dsocontrol)
    : m_dsocontrol(dsocontrol), m_spec(const_cast<Dso::ModelSpec *>(dsocontrol->specification())) {}

void SelfCalibration::start(ChannelID channelID) {
    if (m_isRunning) return;
    m_isRunning = true;
    m_currentHardwareGainStep = 0;
    m_isFirstSet = true;
    m_channelID = channelID;
    emit runningChanged(m_isRunning);
}

void SelfCalibration::cancel() {
    if (!m_isRunning) return;
    m_isRunning = false;
    emit runningChanged(m_isRunning);
}

void SelfCalibration::process(PPresult *data) {
    if (!m_isRunning) return;
    if (m_currentHardwareGainStep > m_spec->gain.size()) { cancel(); }

    if (m_dsocontrol->deviceSettings()->voltage[m_channelID]->gainStepIndex() != m_currentHardwareGainStep) {
        if (m_dsocontrol->setGain(m_channelID, m_currentHardwareGainStep, true) != Dso::ErrorCode::NONE) { cancel(); }
        return;
    }

    const DataChannel *channelData = data->data(0);
    QString task = valueToString(m_spec->gain[m_currentHardwareGainStep].gain, Unit::VOLTS);

    if (m_isFirstSet) {
        m_isFirstSet = false;
        minRaw = channelData->minRaw;
        maxRaw = channelData->maxRaw;
        minVoltage = channelData->minVoltage;
        maxVoltage = channelData->maxVoltage;
        emit progress((m_currentHardwareGainStep * 2 + 0) / m_spec->gain.size() * 2, task);
        return;
    }
    m_isFirstSet = true;
    emit progress((m_currentHardwareGainStep * 2 + 1) / m_spec->gain.size() * 2, task);

    // Compute average value
    minRaw = uint16_t(unsigned(minRaw + channelData->minRaw) / 2);
    maxRaw = uint16_t(unsigned(maxRaw + channelData->maxRaw) / 2);
    minVoltage = (minVoltage + channelData->minVoltage) / 2;
    maxVoltage = (maxVoltage + channelData->maxVoltage) / 2;

    // Compute offset
    double &oldOffset = m_spec->calibration[m_channelID][m_currentHardwareGainStep].offsetCorrection;
    oldOffset = minVoltage;

    // Compute gain limit / normalise factor.
    // double samplePoint = (v / limit - harwareOffset) * gainStep - offsetCorrection;
    const double hardwareGain = m_spec->gain[m_currentHardwareGainStep].gain;
    double newLimit = hardwareGain * (maxRaw - minRaw);
    double &oldLimiz = m_spec->calibration[m_channelID][m_currentHardwareGainStep].voltageLimit;
    oldLimiz = newLimit;

    ++m_currentHardwareGainStep;
    if (m_currentHardwareGainStep >= m_spec->gain.size()) { cancel(); }
}
