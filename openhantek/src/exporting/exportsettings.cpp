// SPDX-License-Identifier: GPL-2.0+

#include <QApplication>
#include <QColor>
#include <QDebug>
#include <QSettings>

#include "exportsettings.h"

void Settings::DsoExportIO::read(QSettings *store, DsoExport &exporting) {
    store->beginGroup("exporting");
    if (store->contains("imageSize")) exporting.imageSize = store->value("imageSize").toSize();
    if (store->contains("exportSizeBytes")) exporting.exportSizeBytes = store->value("exportSizeBytes").toUInt();
    store->endGroup();
}

void Settings::DsoExportIO::write(QSettings *store, const DsoExport &exporting) {
    store->beginGroup("exporting");
    store->setValue("imageSize", exporting.imageSize);
    store->setValue("exportSizeBytes", exporting.exportSizeBytes);
    store->endGroup();
}
