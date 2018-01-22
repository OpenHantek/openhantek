// SPDX-License-Identifier: GPL-2.0+

#include "postprocessing.h"
#include "settings/scopesettings.h"
#include <QDebug>

namespace PostProcessing {

Executor::Executor(const ::Settings::Scope *scope) : m_scope(scope) {
    qRegisterMetaType<std::shared_ptr<PPresult>>();
    // Call constructor for all datapool ppresults.
    for (unsigned i = 0; i < DATAPOOLSIZE; ++i) { new (&resultPool[i]) PPresult(); }
}

void Executor::registerProcessor(Processor *processor) { processors.push_back(processor); }

inline void convertData(const DSOsamples *source, PPresult *destination, const ::Settings::Scope *scope) {
    QReadLocker locker(&source->lock);

    for (unsigned index = 0; index < source->channelCount(); ++index) {
        auto &v = source->data[index];

        if (v.id == (unsigned)-1 || v.empty()) { continue; }

        // We also create a new shared_ptr reference to the underlying channelsettings. This way we do not get
        // suprises if the user removes a math_channel during processing.
        DataChannel *const channelData = destination->addChannel(v.id, true, scope->channel(v.id));
        channelData->voltage.interval = 1.0 / source->samplerate;
        // Data copy
        channelData->voltage.sample = v;
        channelData->maxVoltage = v.maxVoltage;
        channelData->minVoltage = v.minVoltage;
        channelData->maxRaw = v.maxRaw;
        channelData->minRaw = v.minRaw;
    }
}

void Executor::input(const DSOsamples *data) {
    PPresult *result = nullptr;
    // Find a free PPresult in the PPresult pool
    for (unsigned i = 0; i < DATAPOOLSIZE; ++i) {
        PPresult *p = &resultPool[i];
        if (!p->inUse) {
            p->softwareTriggerTriggered = false;
            p->removeNonDeviceChannels();
            p->inUse = true;
            result = p;
            break;
        }
    }
    // Nothing found: abort
    if (!result) {
        qWarning() << "Sampleset skipped. Too busy!";
        return;
    }

    convertData(data, result, m_scope);
    for (Processor *p : processors) p->process(result);
    // Create a shared_ptr with a custom "delete" function that resets inUse.
    emit processingFinished(std::shared_ptr<PPresult>(result, [](PPresult *r) { r->inUse = false; }));
}
}
