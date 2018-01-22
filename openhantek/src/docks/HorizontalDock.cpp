// SPDX-License-Identifier: GPL-2.0+

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QSignalBlocker>
#include <QStringListModel>

#include <cmath>

#include "HorizontalDock.h"
#include "dockwindows.h"
#include "hantekdso/devicesettings.h"
#include "hantekdso/dsocontrol.h"
#include "hantekdso/enums.h"
#include "hantekdso/modelspecification.h"
#include "scopesettings.h"
#include "utils/enumhelper.h"
#include "utils/printutils.h"
#include "widgets/sispinbox.h"

Q_DECLARE_METATYPE(std::vector<unsigned>)
Q_DECLARE_METATYPE(std::vector<double>)

template <typename... Args> struct SELECT {
    template <typename C, typename R> static constexpr auto OVERLOAD_OF(R (C::*pmf)(Args...)) -> decltype(pmf) {
        return pmf;
    }
};

/// A simple Qt Model with the fixed samplerates as display values
class FixedSamplerateModel : public QAbstractListModel {
    Q_OBJECT
  public:
    FixedSamplerateModel(const std::vector<Dso::FixedSampleRate> &steps, QObject *parent = nullptr)
        : QAbstractListModel(parent), steps(steps) {
        for (const Dso::FixedSampleRate &v : steps) {
            stepStrings.push_back(valueToString(v.samplerate, Unit::SAMPLES, 3));
        }
    }

  private:
    const std::vector<Dso::FixedSampleRate> steps;
    std::vector<QString> stepStrings;

    // QAbstractItemModel interface
  public:
    virtual int rowCount(const QModelIndex &) const override { return (int)steps.size(); }
    virtual QVariant data(const QModelIndex &index, int role) const override {
        if (role == Qt::DisplayRole) { return stepStrings[(unsigned)index.row()]; }
        return QVariant();
    }
};
#include "HorizontalDock.moc"

HorizontalDock::HorizontalDock(Settings::Scope *scope, DsoControl *dsocontrol, QWidget *parent, Qt::WindowFlags flags)
    : QDockWidget(tr("Horizontal"), parent, flags) {

    QGridLayout *dockLayout;           ///< The main layout for the dock window
    QWidget *dockWidget;               ///< The main widget for the dock window
    QLabel *samplerateLabel;           ///< The label for the samplerate spinbox
    QLabel *timebaseLabel;             ///< The label for the timebase spinbox
    QLabel *frequencybaseLabel;        ///< The label for the frequencybase spinbox
    QLabel *recordLengthLabel;         ///< The label for the record length combobox
    QLabel *formatLabel;               ///< The label for the format combobox
    SiSpinBox *samplerateSiSpinBox;    ///< Selects the samplerate for aquisitions
    QComboBox *fixedSamplerateBox;     ///< Selects the samplerate for aquisitions (fixed samplerrates)
    SiSpinBox *timebaseSiSpinBox;      ///< Selects the timebase for voltage graphs
    SiSpinBox *frequencybaseSiSpinBox; ///< Selects the frequencybase for spectrum graphs
    QComboBox *recordLengthComboBox;   ///< Selects the record length for aquisitions
    QComboBox *formatComboBox;         ///< Selects the way the sampled data is
                                       /// interpreted and shown

    // Initialize elements
    samplerateLabel = new QLabel(tr("Samplerate"), this);
    samplerateSiSpinBox = new SiSpinBox(Unit::SAMPLES, this);
    samplerateSiSpinBox->setRange(0, 0);
    samplerateSiSpinBox->setUnitPostfix("/s");

    fixedSamplerateBox = new QComboBox(this);

    std::vector<double> timebaseSteps = {1.0, 2.0, 4.0, 10.0};

    timebaseLabel = new QLabel(tr("Timebase"));
    timebaseSiSpinBox = new SiSpinBox(Unit::SECONDS, this);
    timebaseSiSpinBox->setSteps(timebaseSteps);

    frequencybaseLabel = new QLabel(tr("Frequencybase"));
    frequencybaseSiSpinBox = new SiSpinBox(Unit::HERTZ, this);
    frequencybaseSiSpinBox->setMinimum(1.0);
    frequencybaseSiSpinBox->setMaximum(100e6);
    frequencybaseSiSpinBox->setToolTip(
        tr("From %1 to %2")
            .arg(frequencybaseSiSpinBox->textFromValue(frequencybaseSiSpinBox->minimum()))
            .arg(frequencybaseSiSpinBox->textFromValue(frequencybaseSiSpinBox->maximum())));

    recordLengthLabel = new QLabel(tr("Record length"), this);
    recordLengthComboBox = new QComboBox(this);

    formatLabel = new QLabel(tr("Format"), this);
    formatComboBox = new QComboBox(this);
    for (Dso::GraphFormat format : Enum<Dso::GraphFormat>()) formatComboBox->addItem(Dso::graphFormatString(format));

    dockLayout = new QGridLayout;
    dockLayout->setColumnMinimumWidth(0, 64);
    dockLayout->setColumnStretch(1, 1);
    dockLayout->addWidget(samplerateLabel, 0, 0);
    dockLayout->addWidget(samplerateSiSpinBox, 0, 1);
    dockLayout->addWidget(fixedSamplerateBox, 0, 1);
    dockLayout->addWidget(timebaseLabel, 1, 0);
    dockLayout->addWidget(timebaseSiSpinBox, 1, 1);
    dockLayout->addWidget(frequencybaseLabel, 2, 0);
    dockLayout->addWidget(frequencybaseSiSpinBox, 2, 1);
    dockLayout->addWidget(recordLengthLabel, 3, 0);
    dockLayout->addWidget(recordLengthComboBox, 3, 1);
    dockLayout->addWidget(formatLabel, 4, 0);
    dockLayout->addWidget(formatComboBox, 4, 1);

    dockWidget = new QWidget(this);
    SetupDockWidget(this, dockWidget, dockLayout);

    auto deviceSettings = dsocontrol->deviceSettings().get();

    // Set values
    if (dsocontrol->specification()->isFixedSamplerateDevice) {
        samplerateSiSpinBox->setVisible(false);
        fixedSamplerateBox->setVisible(true);
        fixedSamplerateBox->setModel(new FixedSamplerateModel(dsocontrol->specification()->fixedSampleRates, this));
        fixedSamplerateBox->setCurrentIndex((int)deviceSettings->samplerate().fixedSamperateId);
    } else {
        samplerateSiSpinBox->setVisible(true);
        fixedSamplerateBox->setVisible(false);
        samplerateSiSpinBox->setMinimum(dsocontrol->minSamplerate());
        samplerateSiSpinBox->setMaximum(dsocontrol->maxSamplerate());
        samplerateSiSpinBox->setValue(deviceSettings->samplerate().samplerate);
    }
    formatComboBox->setCurrentIndex((int)scope->format());
    frequencybaseSiSpinBox->setValue(scope->frequencybase());
    timebaseSiSpinBox->setValue(deviceSettings->samplerate().timebase); // /DIVS_TIME
    for (auto &entry : deviceSettings->limits->recordLengths) {
        recordLengthComboBox->addItem(
            entry.recordLength == UINT_MAX ? tr("Roll") : valueToString(entry.recordLength, Unit::SAMPLES, 3),
            entry.recordLength);
    }
    recordLengthComboBox->setCurrentIndex((int)deviceSettings->recordLengthId());

    // Connect signals and slots
    connect(samplerateSiSpinBox, SELECT<double>::OVERLOAD_OF(&QDoubleSpinBox::valueChanged), this,
            [this, dsocontrol](double samplerate) { dsocontrol->setSamplerate(samplerate); });
    connect(fixedSamplerateBox, SELECT<int>::OVERLOAD_OF(&QComboBox::currentIndexChanged), this,
            [this, dsocontrol](int index) { dsocontrol->setFixedSamplerate((unsigned)index); });
    connect(timebaseSiSpinBox, SELECT<double>::OVERLOAD_OF(&QDoubleSpinBox::valueChanged), this,
            [this, dsocontrol, timebaseSiSpinBox, deviceSettings](double recordTime) {
                QSignalBlocker timebaseBlocker(timebaseSiSpinBox);
                dsocontrol->setRecordTime(recordTime); /* *DIVS_TIME */
            });
    connect(frequencybaseSiSpinBox, SELECT<double>::OVERLOAD_OF(&QDoubleSpinBox::valueChanged), this,
            [this, scope](double frequencybase) { scope->setFrequencybase(frequencybase); });
    connect(recordLengthComboBox, SELECT<int>::OVERLOAD_OF(&QComboBox::currentIndexChanged), this,
            [this, dsocontrol](int index) { dsocontrol->setRecordLengthByIndex(index); });
    connect(formatComboBox, SELECT<int>::OVERLOAD_OF(&QComboBox::currentIndexChanged), this,
            [this, scope](int index) { scope->setFormat((Dso::GraphFormat)index); });

    connect(deviceSettings, &Dso::DeviceSettings::samplerateLimitsChanged, this,
            [samplerateSiSpinBox, fixedSamplerateBox, timebaseSiSpinBox](double minimum, double maximum) {
                QSignalBlocker blocker(samplerateSiSpinBox);
                QSignalBlocker blocker2(fixedSamplerateBox);
                QSignalBlocker timebaseBlocker(timebaseSiSpinBox);
                samplerateSiSpinBox->setVisible(true);
                fixedSamplerateBox->setVisible(false);
                samplerateSiSpinBox->setMinimum(minimum);
                samplerateSiSpinBox->setMaximum(maximum);
                timebaseSiSpinBox->setMinimum(1e-9);
                timebaseSiSpinBox->setMaximum(3.6e3);
                timebaseSiSpinBox->setToolTip(tr("From %1 to %2")
                                                  .arg(timebaseSiSpinBox->textFromValue(timebaseSiSpinBox->minimum()))
                                                  .arg(timebaseSiSpinBox->textFromValue(timebaseSiSpinBox->maximum())));
            });
    connect(deviceSettings, &Dso::DeviceSettings::fixedSampleratesChanged, this,
            [samplerateSiSpinBox, fixedSamplerateBox, timebaseSiSpinBox,
             deviceSettings](const std::vector<Dso::FixedSampleRate> &sampleSteps) {
                QSignalBlocker blocker(samplerateSiSpinBox);
                QSignalBlocker blocker2(fixedSamplerateBox);
                samplerateSiSpinBox->setVisible(false);
                fixedSamplerateBox->setVisible(true);
                fixedSamplerateBox->setModel(new FixedSamplerateModel(sampleSteps, fixedSamplerateBox));
                QSignalBlocker timebaseBlocker(timebaseSiSpinBox);
                const double reducedRecLen =
                    deviceSettings->getRecordLength() - deviceSettings->trigger.swSampleMargin();
                timebaseSiSpinBox->setMinimum(reducedRecLen / sampleSteps.back().samplerate);
                timebaseSiSpinBox->setMaximum(reducedRecLen / sampleSteps.front().samplerate);
                timebaseSiSpinBox->setToolTip(tr("From %1 to %2")
                                                  .arg(timebaseSiSpinBox->textFromValue(timebaseSiSpinBox->minimum()))
                                                  .arg(timebaseSiSpinBox->textFromValue(timebaseSiSpinBox->maximum())));
            });
    connect(deviceSettings, &Dso::DeviceSettings::availableRecordLengthsChanged, this, [recordLengthComboBox,
                                                                                        deviceSettings]() {
        QSignalBlocker blocker(recordLengthComboBox);
        recordLengthComboBox->clear();
        for (auto entry : deviceSettings->limits->recordLengths) {
            recordLengthComboBox->addItem(
                entry.recordLength == UINT_MAX ? tr("Roll") : valueToString(entry.recordLength, Unit::SAMPLES, 3),
                entry.recordLength);
        }
    });

    connect(deviceSettings, &Dso::DeviceSettings::samplerateChanged, this,
            [samplerateSiSpinBox, timebaseSiSpinBox, fixedSamplerateBox](Dso::Samplerate samplerate) {
                QSignalBlocker blocker(samplerateSiSpinBox);
                QSignalBlocker blocker2(timebaseSiSpinBox);
                QSignalBlocker blocker3(fixedSamplerateBox);
                samplerateSiSpinBox->setValue(samplerate.samplerate);
                timebaseSiSpinBox->setValue(samplerate.timebase); // /DIVS_TIME
                fixedSamplerateBox->setCurrentIndex(samplerate.fixedSamperateId);
            });
    connect(deviceSettings, &Dso::DeviceSettings::recordLengthChanged, this,
            [recordLengthComboBox](unsigned recordLengthId) {
                QSignalBlocker blocker(recordLengthComboBox);
                recordLengthComboBox->setCurrentIndex((int)recordLengthId);
            });

    connect(scope, &Settings::Scope::frequencybaseChanged, this,
            [frequencybaseSiSpinBox](const Settings::Scope *scope) {
                QSignalBlocker blocker(frequencybaseSiSpinBox);
                frequencybaseSiSpinBox->setValue(scope->frequencybase());
            });
    connect(scope, &Settings::Scope::formatChanged, this, [formatComboBox](const Settings::Scope *scope) {
        QSignalBlocker blocker(formatComboBox);
        formatComboBox->setCurrentIndex((int)scope->format());
    });
}

/// \brief Don't close the dock, just hide it.
/// \param event The close event that should be handled.
void HorizontalDock::closeEvent(QCloseEvent *event) {
    hide();
    event->accept();
}
