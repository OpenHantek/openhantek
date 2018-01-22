// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QObject>
#include <memory>
#include <set>
#include <vector>

// Post processing forwards
class Processor;
class PPresult;

// Settings forwards
namespace Settings {
class DsoSettings;
}
namespace Dso {
struct ModelSpec;
}

namespace Exporter {
class ExporterInterface;

class Registry : public QObject {
    Q_OBJECT
  public:
    explicit Registry(const Dso::ModelSpec *deviceSpecification, Settings::DsoSettings *settings,
                      QObject *parent = nullptr);

    // Sample input. This will proably be performed in the post processing
    // thread context. Do not open GUI dialogs or interrupt the control flow.
    void input(std::shared_ptr<PPresult> data);

    void registerExporter(ExporterInterface *exporter);

    /**
     * Called from the GUI thread to start an export process.
     * @param exporter The exporter to use.
     */
    void exportNow(ExporterInterface *exporter);

    void stopContinous(ExporterInterface *exporter);

    inline std::shared_ptr<PPresult> lastDataSet() const { return m_lastDataset; }

    // Iterate over this class object
    std::vector<ExporterInterface *>::const_iterator begin();
    std::vector<ExporterInterface *>::const_iterator end();

    /// Device specifications
    const Dso::ModelSpec *deviceSpecification;
    const Settings::DsoSettings *settings;

  private:
    /// List of all available exporters
    std::vector<ExporterInterface *> exporters;
    /// List of exporters that collect samples at the moment
    std::set<ExporterInterface *> continousActiveExporters;

    std::shared_ptr<PPresult> m_lastDataset;
  signals:
    void exporterStatusChanged(const QString &exporterName, const QString &status);
};
}
