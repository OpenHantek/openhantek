// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "processor.h"
#include <QObject>

class DsoControl;
namespace Dso {
struct ModelSpec;
}

class SelfCalibration : public QObject, public PostProcessing::Processor {
    Q_OBJECT
  public:
    SelfCalibration(DsoControl *m_dsocontrol);
    void start(ChannelID channelID);
    void cancel();

  protected:
    virtual void process(PPresult *) override;

    unsigned m_currentHardwareGainStep = 0;
    bool m_isRunning = false;
    DsoControl *m_dsocontrol;
    Dso::ModelSpec *m_spec;
    bool m_isFirstSet;
    ChannelID m_channelID;

    // We use two sample-sets to build an average. Save values of first sample-set.
    double minVoltage;
    double maxVoltage;
    uint16_t minRaw;
    uint16_t maxRaw;

  signals:
    void runningChanged(bool m_isRunning);
    void progress(double progress, const QString &task);
};
