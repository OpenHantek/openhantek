// SPDX-License-Identifier: GPL-2.0+

#include "exporterregistry.h"
#include "exporterinterface.h"

#include <algorithm>
#include <QDebug>

#include "hantekdso/modelspecification.h"
#include "post/ppresult.h"
#include "settings.h"

template <class T, class Comp, class Alloc, class Predicate>
void discard_if(std::set<T, Comp, Alloc> &c, Predicate pred) {
    for (auto it{c.begin()}, end{c.end()}; it != end;) {
        if (pred(*it)) {
            it = c.erase(it);
        } else {
            ++it;
        }
    }
}

namespace Exporter {
Registry::Registry(const Dso::ModelSpec *deviceSpecification, Settings::DsoSettings *settings, QObject *parent)
    : QObject(parent), deviceSpecification(deviceSpecification), settings(settings) {
}

void Registry::input(std::shared_ptr<PPresult> data) {
    m_lastDataset = data;
    discard_if(continousActiveExporters,
               [&data, this](ExporterInterface *const &i) { return i->samples(data) >= 1.0; });
}

void Registry::registerExporter(ExporterInterface *exporter) { exporters.push_back(exporter); }

void Registry::exportNow(ExporterInterface *exporter) {
    if (exporter->exportNow(this) && exporter->type() == ExporterInterface::Type::ContinousExport) {
        auto localFind = continousActiveExporters.find(exporter);
        if (localFind == continousActiveExporters.end()) { continousActiveExporters.insert(exporter); }
    }
}

void Registry::stopContinous(ExporterInterface *exporter) {
    if (exporter->type() == ExporterInterface::Type::ContinousExport) {
        auto localFind = continousActiveExporters.find(exporter);
        if (localFind != continousActiveExporters.end()) {
            continousActiveExporters.erase(localFind);
            exporter->stopContinous();
        }
    }
}

std::vector<ExporterInterface *>::const_iterator Registry::begin() { return exporters.begin(); }

std::vector<ExporterInterface *>::const_iterator Registry::end() { return exporters.end(); }
}
