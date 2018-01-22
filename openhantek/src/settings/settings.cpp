// SPDX-License-Identifier: GPL-2.0+

#include <QApplication>
#include <QColor>
#include <QDebug>
#include <QSettings>

#include "settings.h"
#include "viewconstants.h"

bool Settings::DsoSettings::setFilename(const QString &filename) {
    std::unique_ptr<QSettings> local = std::unique_ptr<QSettings>(new QSettings(filename, QSettings::IniFormat));
    if (local->status() != QSettings::NoError) {
        qWarning() << "Could not change the settings file to " << filename;
        return false;
    }
    store.swap(local);
    return true;
}

/// \brief Set the number of channels.
/// \param channels The new channel count, that will be applied to lists.
Settings::DsoSettings::DsoSettings(const Dso::ModelSpec *deviceSpecification) {
    deviceSettings = std::make_shared<Dso::DeviceSettings>(deviceSpecification);
}

void Settings::DsoSettings::save() {
    // Main window layout and other general options
    store->beginGroup("options");
    store->setValue("alwaysSave", alwaysSave);
    store->endGroup();

    store->beginGroup("window");
    store->setValue("geometry", mainWindowGeometry);
    store->setValue("state", mainWindowState);
    store->endGroup();

    DeviceSettingsIO::write(store.get(), *deviceSettings.get());
    ScopeIO::write(store.get(), scope);
    ViewIO::write(store.get(), view);
    DsoExportIO::write(store.get(), exporting);
    PostProcessing::SettingsIO::write(store.get(), post);
}

void Settings::DsoSettings::load(Dso::ChannelUsage *channelUsage) {
    // General options
    store->beginGroup("options");
    if (store->contains("alwaysSave")) alwaysSave = store->value("alwaysSave").toBool();
    store->endGroup();

    store->beginGroup("window");
    mainWindowGeometry = store->value("geometry").toByteArray();
    mainWindowState = store->value("state").toByteArray();
    store->endGroup();

    DeviceSettingsIO::read(store.get(), *deviceSettings.get());
    ScopeIO::read(store.get(), scope, deviceSettings.get(), channelUsage);
    ViewIO::read(store.get(), view, &scope);
    DsoExportIO::read(store.get(), exporting);
    PostProcessing::SettingsIO::read(store.get(), post);
    /// Initally, after loading, the stored channel settings contain information about
    /// enabled/disabled (visible/hidden) physical and math channels. Those are not
    /// in sync with the deviceSettings usage information. We force a manual sync here.
    for (Channel *c : scope) {
        c->setVoltageVisible(c->visible());
        c->setSpectrumVisible(c->spectrum()->visible());
    }
}
