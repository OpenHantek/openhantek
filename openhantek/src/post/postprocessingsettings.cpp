// SPDX-License-Identifier: GPL-2.0+

#include "postprocessingsettings.h"
#include "utils/enumhelper.h"

#include <QSettings>

namespace PostProcessing {
void SettingsIO::read(QSettings *store, Settings &post) {
    store->beginGroup("postprocessing");
    post.m_spectrumLimit = store->value("imageSize", post.m_spectrumLimit).toDouble();
    post.m_spectrumReference = store->value("exportSizeBytes", post.m_spectrumReference).toUInt();
    post.m_spectrumWindow = loadForEnum(store, "spectrumWindow", post.m_spectrumWindow);
    store->endGroup();
}

void SettingsIO::write(QSettings *store, const Settings &post) {
    store->beginGroup("postprocessing");
    store->setValue("spectrumLimit", post.m_spectrumLimit);
    store->setValue("spectrumReference", post.m_spectrumReference);
    store->setValue("spectrumWindow", enumName(post.m_spectrumWindow));
    store->endGroup();
}
} // end namespace PostProcessing
