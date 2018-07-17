// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "enums-post.h"

class QSettings;
class DsoConfigAnalysisPage;

namespace PostProcessing {
struct Settings {
    friend class ::DsoConfigAnalysisPage;
    friend struct SettingsIO;

    inline ::PostProcessingE::WindowFunction spectrumWindow() const { return m_spectrumWindow; }
    inline double spectrumReference() const { return m_spectrumReference; }
    inline double spectrumLimit() const { return m_spectrumLimit; }

  private:
    ::PostProcessingE::WindowFunction m_spectrumWindow = //
        ::PostProcessingE::WindowFunction::HANN;         ///< Window function for DFT
    double m_spectrumReference = 0.0;                   ///< Reference level for spectrum in dBm
    double m_spectrumLimit = -20.0;                     ///< Minimum magnitude of the spectrum (Avoids peaks)
};

struct SettingsIO {
    static void read(QSettings *io, Settings &post);
    static void write(QSettings *io, const Settings &post);
};
}
