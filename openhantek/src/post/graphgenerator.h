// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <deque>

#include <QObject>
#include <QVector3D>

#include "hantekdso/enums.h"
#include "hantekprotocol/types.h"
#include "processor.h"

namespace Settings {
class Scope;
}
class PPresult;
namespace Dso {
class DeviceSettings;
class ChannelUsage;
struct ModelSpec;
}
namespace PostProcessing {

/// \brief Generates ready to be used vertex arrays
class GraphGenerator : public QObject, public Processor {
    Q_OBJECT

  public:
    GraphGenerator(const ::Settings::Scope *m_scope, const Dso::DeviceSettings *m_deviceSettings,
                   const Dso::ChannelUsage *channelUsage);
    void generateGraphsXY(PPresult *result, const ::Settings::Scope *m_scope);

  private:
    void generateGraphsTYvoltage(PPresult *result);
    void generateGraphsTYspectrum(PPresult *result);

  private:
    const ::Settings::Scope *m_scope;
    const Dso::DeviceSettings *m_deviceSettings;
    const Dso::ChannelUsage *m_channelUsage;

    // Processor interface
  private:
    virtual void process(PPresult *) override;
};
}
