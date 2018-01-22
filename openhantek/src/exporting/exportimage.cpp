// SPDX-License-Identifier: GPL-2.0+

#include "exportimage.h"
#include "exporterregistry.h"
#include "iconfont/QtAwesome.h"
#include "legacyexportdrawer.h"
#include "post/ppresult.h"
#include "settings.h"
#include "viewsettings.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFileDialog>
#include <QPrinter>
namespace Exporter {
Image::Image() {}

QIcon Image::icon() { return iconFont->icon(fa::image); }

QString Image::name() { return QCoreApplication::tr("Export Image/PDF"); }

ExporterInterface::Type Image::type() { return Type::SnapshotExport; }

float Image::samples(const std::shared_ptr<PPresult>) { return 1; }

bool Image::exportNow(Registry *registry) {
    auto data = registry->lastDataSet();
    if (!data) return false;
    QStringList filters;
    filters << QCoreApplication::tr("Image (*.png *.xpm *.jpg *.bmp)")
            << QCoreApplication::tr("Portable Document Format (*.pdf)");

    QFileDialog fileDialog(nullptr, QCoreApplication::tr("Export file..."), QString(), filters.join(";;"));
    fileDialog.selectFile(QDateTime::currentDateTime().toString(Qt::ISODate) + ".png");
    fileDialog.setFileMode(QFileDialog::AnyFile);
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    if (fileDialog.exec() != QDialog::Accepted) return false;

    bool isPdf = filters.indexOf(fileDialog.selectedNameFilter()) == 1;
    const QString filename = fileDialog.selectedFiles().first();
    std::unique_ptr<QPaintDevice> paintDevice;

    const Settings::Colors *colorValues = &(registry->settings->view.print);

    if (!isPdf) {
        // We need a QPixmap for image-export
        QPixmap *qPixmap = new QPixmap(registry->settings->exporting.imageSize);
        qPixmap->fill(colorValues->background());
        paintDevice = std::unique_ptr<QPaintDevice>(qPixmap);
    } else {
        // We need a QPrinter for printing, pdf- and ps-export
        std::unique_ptr<QPrinter> printer = std::unique_ptr<QPrinter>(new QPrinter(QPrinter::HighResolution));
        printer->setOrientation(QPrinter::Landscape);
        printer->setPageMargins(20, 20, 20, 20, QPrinter::Millimeter);
        printer->setOutputFileName(filename);
        printer->setOutputFormat(QPrinter::PdfFormat);
        paintDevice = std::move(printer);
    }

    if (!paintDevice) return false;

    LegacyExportDrawer::exportSamples(data, paintDevice.get(), registry->deviceSpecification, registry->settings,
                                      colorValues);

    if (!isPdf) static_cast<QPixmap *>(paintDevice.get())->save(filename);
    return true;
}

QKeySequence Image::shortcut() { return QKeySequence(Qt::CTRL | Qt::Key_E); }
}
