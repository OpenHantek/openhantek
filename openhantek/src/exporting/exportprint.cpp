// SPDX-License-Identifier: GPL-2.0+

#include "exportprint.h"
#include "exporterregistry.h"
#include "iconfont/QtAwesome.h"
#include "legacyexportdrawer.h"
#include "post/ppresult.h"
#include "settings.h"
#include "viewsettings.h"

#include <QCoreApplication>
#include <QPrintDialog>
#include <QPrinter>

namespace Exporter {
Print::Print() {}

QIcon Print::icon() { return iconFont->icon(fa::print); }

QString Print::name() { return QCoreApplication::tr("Print"); }

ExporterInterface::Type Print::type() { return Type::SnapshotExport; }

float Print::samples(const std::shared_ptr<PPresult>) { return 1; }

bool Print::exportNow(Registry *registry) {
    auto data = registry->lastDataSet();
    if (!data) return false;
    // We need a QPrinter for printing, pdf- and ps-export
    std::unique_ptr<QPrinter> printer = std::unique_ptr<QPrinter>(new QPrinter(QPrinter::HighResolution));
    printer->setOrientation(QPrinter::Landscape);
    printer->setPageMargins(20, 20, 20, 20, QPrinter::Millimeter);

    // Show the printing dialog
    QPrintDialog dialog(printer.get());
    dialog.setWindowTitle(QCoreApplication::tr("Print oscillograph"));
    if (dialog.exec() != QDialog::Accepted) { return false; }

    const Settings::Colors *colorValues = &(registry->settings->view.print);

    LegacyExportDrawer::exportSamples(data, printer.get(), registry->deviceSpecification, registry->settings,
                                      colorValues);

    return true;
}

QKeySequence Print::shortcut() { return QKeySequence(Qt::CTRL | Qt::Key_P); }
}
