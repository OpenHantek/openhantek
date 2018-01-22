// SPDX-License-Identifier: GPL-2.0+

#include <QWidget>

#include "settings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

/// \brief Config page for the data analysis.
class DsoConfigAnalysisPage : public QWidget {
    Q_OBJECT

  public:
    DsoConfigAnalysisPage(Settings::DsoSettings *settings, QWidget *parent = 0);

  public slots:
    void saveSettings();

  private:
    Settings::DsoSettings *settings;

    QVBoxLayout *mainLayout;

    QGroupBox *spectrumGroup;
    QGridLayout *spectrumLayout;
    QLabel *windowFunctionLabel;
    QComboBox *windowFunctionComboBox;

    QLabel *referenceLevelLabel;
    QDoubleSpinBox *referenceLevelSpinBox;
    QLabel *referenceLevelUnitLabel;
    QHBoxLayout *referenceLevelLayout;

    QLabel *minimumMagnitudeLabel;
    QDoubleSpinBox *minimumMagnitudeSpinBox;
    QLabel *minimumMagnitudeUnitLabel;
    QHBoxLayout *minimumMagnitudeLayout;
};
