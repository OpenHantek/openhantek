// SPDX-License-Identifier: GPL-2.0+

#include "exportcsv.h"
#include "exporterregistry.h"
#include "iconfont/QtAwesome.h"
#include "post/ppresult.h"
#include "settings.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>

namespace Exporter {
CSV::CSV() {}

QIcon CSV::icon() { return iconFont->icon(fa::filetexto); }

QString CSV::name() { return QCoreApplication::tr("Export CSV"); }

ExporterInterface::Type CSV::type() { return Type::SnapshotExport; }

float CSV::samples(const std::shared_ptr<PPresult>) { return 1; }

bool CSV::exportNow(Registry *registry) {
    auto data = registry->lastDataSet();
    if (!data) return false;

    QStringList filters;
    filters << QCoreApplication::tr("Comma-Separated Values (*.csv)");

    QFileDialog fileDialog(nullptr, QCoreApplication::tr("Export file..."), QString(), filters.join(";;"));
    fileDialog.setFileMode(QFileDialog::AnyFile);
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    if (fileDialog.exec() != QDialog::Accepted) return false;

    QFile csvFile(fileDialog.selectedFiles().first());
    if (!csvFile.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream csvStream(&csvFile);
    csvStream.setRealNumberNotation(QTextStream::FixedNotation);
    csvStream.setRealNumberPrecision(10);

    std::map<ChannelID, const std::vector<double> *> voltageData;
    std::map<ChannelID, const std::vector<double> *> spectrumData;
    size_t maxRow = 0;
    double timeInterval = 0;
    double freqInterval = 0;

    const Settings::Scope *scope = &registry->settings->scope;
    for (const Settings::Channel *c : *scope) {
        const DataChannel *dataChannel = data->data(c->channelID());
        if (!dataChannel) continue;
        if (c->visible()) {
            const std::vector<double> *samples = &(dataChannel->voltage.sample);
            voltageData[c->channelID()] = samples;
            maxRow = std::max(maxRow, samples->size());
            timeInterval = dataChannel->voltage.interval;
        }
        if (c->spectrum()->visible()) {
            const std::vector<double> *samples = &(dataChannel->spectrum.sample);
            spectrumData[c->channelID()] = samples;
            maxRow = std::max(maxRow, samples->size());
            freqInterval = dataChannel->spectrum.interval;
        }
    }

    // Start with channel names
    csvStream << "\"t\"";
    for (auto &c : voltageData) { csvStream << ",\"" << scope->channel(c.first)->name() << "\""; }
    csvStream << ",\"f\"";
    for (auto &c : spectrumData) { csvStream << ",\"" << scope->channel(c.first)->name() << "\""; }
    csvStream << "\n";

    for (unsigned int row = 0; row < maxRow; ++row) {

        csvStream << timeInterval * row;
        for (auto &c : voltageData) {
            csvStream << ",";
            if (row < c.second->size()) { csvStream << (*c.second)[row]; }
        }

        csvStream << "," << freqInterval * row;
        for (auto &c : spectrumData) {
            csvStream << ",";
            if (row < c.second->size()) { csvStream << (*c.second)[row]; }
        }
        csvStream << "\n";
    }

    csvFile.close();

    return true;
}

QKeySequence CSV::shortcut() { return QKeySequence(); }
}
