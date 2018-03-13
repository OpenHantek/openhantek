// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <vector>

#include <QMutex>
#include <QThread>
#include <memory>

#include "dsosamples.h"
#include "ppresult.h"
#include "utils/printutils.h"

#include "enums.h"
#include "processor.h"

namespace Settings {
class Scope;
class DsoSettings;
}

namespace PostProcessing {
struct Settings;

/// \brief Analyzes the data from the dso.
/// Calculates the spectrum and various data about the signal and saves the
/// time-/frequencysteps between two values.
class SpectrumGenerator : public Processor {
  public:
    SpectrumGenerator(const ::Settings::Scope *scope, const Settings *postprocessing);
    virtual ~SpectrumGenerator();
    virtual void process(PPresult *data) override;

  private:
    const ::Settings::Scope *scope;
    const Settings *postprocessing;
    unsigned int lastRecordLength = 0; ///< The record length of the previously analyzed data
    PostProcessingE::WindowFunction lastWindow =
        (PostProcessingE::WindowFunction)-1; ///< The previously used dft window function
    double *lastWindowBuffer = nullptr;
};
}
