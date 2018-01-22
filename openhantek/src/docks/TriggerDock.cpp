// SPDX-License-Identifier: GPL-2.0+

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QSignalBlocker>

#include <cmath>

#include "TriggerDock.h"
#include "dockwindows.h"

#include "hantekdso/devicesettings.h"
#include "hantekdso/dsocontrol.h"
#include "hantekdso/modelspecification.h"
#include "settings.h"
#include "utils/enumhelper.h"
#include "utils/printutils.h"
#include "widgets/sispinbox.h"

TriggerDock::TriggerDock(Settings::Scope *scope, DsoControl *dsocontrol, QWidget *parent, Qt::WindowFlags flags)
    : QDockWidget(tr("Trigger"), parent, flags) {

    QGridLayout *dockLayout;   ///< The main layout for the dock window
    QWidget *dockWidget;       ///< The main widget for the dock window
    QLabel *modeLabel;         ///< The label for the trigger mode combobox
    QLabel *sourceLabel;       ///< The label for the trigger source combobox
    QLabel *slopeLabel;        ///< The label for the trigger slope combobox
    QComboBox *modeComboBox;   ///< Select the triggering mode
    QComboBox *sourceComboBox; ///< Select the source for triggering
    QComboBox *slopeComboBox;  ///< Select the slope that causes triggering

    auto spec = dsocontrol->deviceSettings()->spec;

    // Initialize elements
    modeLabel = new QLabel(tr("Mode"));
    modeComboBox = new QComboBox();
    for (Dso::TriggerMode mode : spec->triggerModes) modeComboBox->addItem(Dso::triggerModeString(mode));

    slopeLabel = new QLabel(tr("Slope"));
    slopeComboBox = new QComboBox();
    for (Dso::Slope slope : Enum<Dso::Slope>()) slopeComboBox->addItem(Dso::slopeString(slope));

    sourceLabel = new QLabel(tr("Source"));
    sourceComboBox = new QComboBox();
    for (auto *c : *scope)
        if (!c->isMathChannel()) sourceComboBox->addItem(tr("CH%1").arg(c->channelID() + 1), (int)c->channelID());
    int specialID = -1; // Assign negative (beginning with -1) ids for special channels
    for (const Dso::SpecialTriggerChannel &specialTrigger : spec->specialTriggerChannels)
        sourceComboBox->addItem(QString::fromStdString(specialTrigger.name), (int)specialID--);

    dockLayout = new QGridLayout();
    dockLayout->setColumnMinimumWidth(0, 64);
    dockLayout->setColumnStretch(1, 1);
    dockLayout->addWidget(modeLabel, 0, 0);
    dockLayout->addWidget(modeComboBox, 0, 1);
    dockLayout->addWidget(sourceLabel, 1, 0);
    dockLayout->addWidget(sourceComboBox, 1, 1);
    dockLayout->addWidget(slopeLabel, 2, 0);
    dockLayout->addWidget(slopeComboBox, 2, 1);

    dockWidget = new QWidget();
    SetupDockWidget(this, dockWidget, dockLayout);

    const Dso::DeviceSettings *devicesettings = dsocontrol->deviceSettings().get();

    // Set values
    modeComboBox->setCurrentIndex(spec->indexOfTriggerMode(devicesettings->trigger.mode()));
    slopeComboBox->setCurrentIndex((int)devicesettings->trigger.slope());
    // A special channel is after all real channels
    sourceComboBox->setCurrentIndex(devicesettings->trigger.special() ? (int)spec->channels
                                                                      : 0 + (int)devicesettings->trigger.source());

    // Connect widgets --> settings
    connect(modeComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [spec, dsocontrol, modeComboBox, devicesettings](int index) {
                dsocontrol->setTriggerMode(spec->triggerModes[(unsigned)index]);
                QSignalBlocker blocker(modeComboBox);
                modeComboBox->setCurrentIndex(spec->indexOfTriggerMode(devicesettings->trigger.mode()));
            });
    connect(slopeComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [dsocontrol, slopeComboBox, devicesettings](int index) {
                dsocontrol->setTriggerSlope((Dso::Slope)index);
                QSignalBlocker blocker(slopeComboBox);
                slopeComboBox->setCurrentIndex((int)devicesettings->trigger.slope());
            });
    connect(sourceComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [sourceComboBox, devicesettings, dsocontrol, spec](int index) {
                int channelIndex = sourceComboBox->itemData(index, Qt::UserRole).toInt();
                dsocontrol->setTriggerSource(channelIndex < 0,
                                             channelIndex < 0 ? (unsigned)(1 + -channelIndex) : (unsigned)channelIndex);
                QSignalBlocker blocker(sourceComboBox);
                sourceComboBox->setCurrentIndex(devicesettings->trigger.special()
                                                    ? (int)spec->channels
                                                    : 0 + (int)devicesettings->trigger.source());
            });
    // Connect settings --> widgets
    connect(&devicesettings->trigger, &Dso::Trigger::modeChanged, this, [modeComboBox, spec](Dso::TriggerMode mode) {
        QSignalBlocker blocker(modeComboBox);
        modeComboBox->setCurrentIndex(spec->indexOfTriggerMode(mode));
    });
    connect(&devicesettings->trigger, &Dso::Trigger::sourceChanged, this,
            [this, sourceComboBox, spec](bool special, unsigned int id) {
                QSignalBlocker blocker(sourceComboBox);
                // A special channel is after all real channels
                sourceComboBox->setCurrentIndex(special ? (int)spec->channels : 0 + (int)id);
            });
    connect(&devicesettings->trigger, &Dso::Trigger::slopeChanged, this, [slopeComboBox](Dso::Slope slope) {
        QSignalBlocker blocker(slopeComboBox);
        slopeComboBox->setCurrentIndex((int)slope);
    });
}

/// \brief Don't close the dock, just hide it
/// \param event The close event that should be handled.
void TriggerDock::closeEvent(QCloseEvent *event) {
    hide();

    event->accept();
}
