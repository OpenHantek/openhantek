// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QSettings>
#include <QSize>
#include <QString>
#include <memory>

#include "exporting/exportsettings.h"
#include "post/postprocessingsettings.h"
#include "scopesettings.h"
#include "viewsettings.h"

namespace Settings {
/// \brief Holds the settings of the program.
class DsoSettings {
    DsoSettings(const DsoSettings &) = delete;
    DsoSettings(const DsoSettings &&) = delete;

  public:
    explicit DsoSettings(const Dso::ModelSpec *deviceSpecification);
    bool setFilename(const QString &filename);

    /// All device related settings. This is shared with DsoControl and manipulated through DsoControl.
    /// You should not modify anything directly, as soon as DsoControl is running, except the channel usages.
    std::shared_ptr<Dso::DeviceSettings> deviceSettings;
    Scope scope;                   ///< All oscilloscope related settings
    View view;                     ///< All view related settings
    DsoExport exporting;           ///< General options of the program
    PostProcessing::Settings post; ///< All post processing related settings

    bool alwaysSave = true;        ///< Always save the settings on exit
    QByteArray mainWindowGeometry; ///< Geometry of the main window
    QByteArray mainWindowState;    ///< State of docking windows and toolbars

    /// \brief Save settings to the underlying QSettings file.
    void save();
    /// \brief Load settings from the underlying QSettings file.
    void load(Dso::ChannelUsage *channelUsage);

  private:
    std::unique_ptr<QSettings> store = std::unique_ptr<QSettings>(new QSettings);
};
}
