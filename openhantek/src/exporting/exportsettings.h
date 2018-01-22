// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QSize>
class QSettings;

namespace Settings {
/// \brief Holds the export options of the program.
struct DsoExport {
    QSize imageSize = QSize(640, 480);           ///< Size of exported images in pixels
    unsigned exportSizeBytes = 1024 * 1024 * 10; ///< For exporters that save a continous stream. Default: 10 Megabytes
};

struct DsoExportIO {
    static void read(QSettings *io, DsoExport &exportStruct);
    static void write(QSettings *io, const DsoExport &exportStruct);
};
}
