// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "exportsettings.h"
#include <QPainter>
#include <QPrinter>
#include <QSize>
#include <memory>

namespace Settings {
class DsoSettings;
struct Colors;
}
class PPresult;
namespace Dso {
struct ModelSpec;
}

namespace Exporter {
/// \brief Exports the oscilloscope screen to a file or prints it.
/// TODO Grab DsoWidget instead of drawing all labels by hand
class LegacyExportDrawer {
  public:
    /// Draw the graphs coming from source and labels to the destination paintdevice.
    static bool exportSamples(std::shared_ptr<PPresult> source, QPaintDevice *dest,
                              const Dso::ModelSpec *deviceSpecification,
                              const Settings::DsoSettings *settings,
                              const Settings::Colors *colorValues);
};
}
