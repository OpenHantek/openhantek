// SPDX-License-Identifier: GPL-2.0+

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QSignalBlocker>

#include <cmath>

#include "VoltageDock.h"
#include "dockwindows.h"

#include "settings.h"
#include "sispinbox.h"
#include "utils/dsoStrings.h"
#include "utils/printutils.h"

template<typename... Args> struct SELECT {
    template<typename C, typename R>
    static constexpr auto OVERLOAD_OF( R (C::*pmf)(Args...) ) -> decltype(pmf) {
        return pmf;
    }
};

VoltageDock::VoltageDock(DsoSettingsScope *scope, const Dso::ControlSpecification *spec, QWidget *parent, Qt::WindowFlags flags)
    : QDockWidget(tr("Voltage"), parent, flags), scope(scope), spec(spec) {

    // Initialize lists for comboboxes
    for (Dso::Coupling c: spec->couplings)
        couplingStrings.append(Dso::couplingString(c));

    for( auto e: Dso::MathModeEnum ) {
        modeStrings.append(Dso::mathModeString(e));
    }

    for (double gainStep: scope->gainSteps)
        gainStrings << valueToString(gainStep, UNIT_VOLTS, 0);

    dockLayout = new QGridLayout();
    dockLayout->setColumnMinimumWidth(0, 64);
    dockLayout->setColumnStretch(1, 1);

    // Initialize elements
    for (ChannelID channel = 0; channel < scope->voltage.size(); ++channel) {
        ChannelBlock b;

        b.miscComboBox=(new QComboBox());
        b.gainComboBox=(new QComboBox());
        b.invertCheckBox=(new QCheckBox(tr("Invert")));
        b.usedCheckBox=(new QCheckBox(scope->voltage[channel].name));
        b.probeGainCombobox=(new QComboBox());

        channelBlocks.push_back(std::move(b));

        if (channel < spec->channels) {
            b.miscComboBox->addItems(couplingStrings);
            QStringList probeGainStrings;
            for(double probe_gain: scope->voltage[channel].probeGainSteps)
                probeGainStrings << valueToString(probe_gain, UNIT_TIMES, 0);
            b.probeGainCombobox->addItems(probeGainStrings);

        }
        else
            b.miscComboBox->addItems(modeStrings);

        b.gainComboBox->addItems(gainStrings);

        dockLayout->addWidget(b.usedCheckBox, (int)channel * 4, 0);
        dockLayout->addWidget(b.gainComboBox, (int)channel * 4, 1);
        dockLayout->addWidget(b.miscComboBox, (int)channel * 4 + 1, 1);

        if(channel < spec->channels){
            dockLayout->addWidget(b.probeGainCombobox, (int) channel * 4 + 2, 1);
            dockLayout->addWidget(b.invertCheckBox, (int)channel * 4 + 3, 1);
        }
        else {
            dockLayout->addWidget(b.invertCheckBox, (int) channel * 4 + 2, 1);
        }

        if (channel < spec->channels) {
            setCoupling(channel, scope->voltage[channel].couplingIndex);
            setProbeGain(channel, scope->voltage[channel].probe_gain);
        }
        else
            setMode(scope->voltage[channel].math);
        setGain(channel, scope->voltage[channel].gainStepIndex);
        setUsed(channel, scope->voltage[channel].used);

        connect(b.gainComboBox, SELECT<int>::OVERLOAD_OF(&QComboBox::currentIndexChanged), [this,channel](int index) {
            this->scope->voltage[channel].gainStepIndex = (unsigned)index;
            emit gainChanged(channel, this->scope->gain(channel));
        });
        connect(b.invertCheckBox, &QAbstractButton::toggled, [this,channel](bool checked) {
            this->scope->voltage[channel].inverted = checked;
        });
        connect(b.miscComboBox, SELECT<int>::OVERLOAD_OF(&QComboBox::currentIndexChanged), [this,channel,spec,scope](int index){
            if (channel < spec->channels) {
                this->scope->voltage[channel].couplingIndex = (unsigned)index;
                emit couplingChanged(channel, scope->coupling(channel, spec));
            } else {
                this->scope->voltage[channel].math = (Dso::MathMode) index;
                emit modeChanged(this->scope->voltage[channel].math);
            }
        });
        connect(b.usedCheckBox, &QAbstractButton::toggled, [this,channel](bool checked) {
            this->scope->voltage[channel].used = checked;
            emit usedChanged(channel, checked);
        });
        connect(b.probeGainCombobox, SELECT<int>::OVERLOAD_OF(&QComboBox::currentIndexChanged), [this,channel,scope](int index){

            this->scope->voltage[channel].probeStepIndex = (unsigned)index;
            this->scope->voltage[channel].probe_gain = this->scope->voltage[channel].probeGainSteps[index];
            emit probeGainChanged(channel, this->scope->voltage[channel].probe_gain);

        });



    }

    dockWidget = new QWidget();
    SetupDockWidget(this, dockWidget, dockLayout);
}

/// \brief Don't close the dock, just hide it
/// \param event The close event that should be handled.
void VoltageDock::closeEvent(QCloseEvent *event) {
    hide();
    event->accept();
}

void VoltageDock::setCoupling(ChannelID channel, unsigned couplingIndex) {
    if (channel >= spec->channels) return;
    if (couplingIndex >= spec->couplings.size()) return;
    QSignalBlocker blocker(channelBlocks[channel].miscComboBox);
    channelBlocks[channel].miscComboBox->setCurrentIndex((int)couplingIndex);
}

void VoltageDock::setGain(ChannelID channel, unsigned gainStepIndex) {
    if (channel >= scope->voltage.size()) return;
    if (gainStepIndex >= scope->gainSteps.size()) return;
    QSignalBlocker blocker(channelBlocks[channel].gainComboBox);
    channelBlocks[channel].gainComboBox->setCurrentIndex((unsigned)gainStepIndex);
}

void VoltageDock::setMode(Dso::MathMode mode) {
    QSignalBlocker blocker(channelBlocks[spec->channels].miscComboBox);
    channelBlocks[spec->channels].miscComboBox->setCurrentIndex((int)mode);
}

void VoltageDock::setUsed(ChannelID channel, bool used) {
    if (channel >= scope->voltage.size()) return;
    QSignalBlocker blocker(channelBlocks[channel].usedCheckBox);
    channelBlocks[channel].usedCheckBox->setChecked(used);
}

int VoltageDock::setProbeGain(ChannelID channel, double probeGain) {
    if (channel < 0 ||channel >= spec->channels) return -1;

    QSignalBlocker blocker(channelBlocks[channel].probeGainCombobox);
    int index = (int) (std::find(scope->voltage[channel].probeGainSteps.begin(),
                                 scope->voltage[channel].probeGainSteps.end(),
                                 probeGain)
                       - scope->voltage[channel].probeGainSteps.begin());
    if(index != -1)
        channelBlocks[channel].probeGainCombobox->setCurrentIndex(index);

    return index;

}

/// \brief Update the combobox with the gain values for the probe
void VoltageDock::probeGainSettingsUpdated() {

    for(unsigned int channel = 0; channel < scope->voltage.size(); channel++) {
        if(channel < spec->channels) {
                QSignalBlocker blocker(channelBlocks[channel].probeGainCombobox);
                //Remove all the old values
                channelBlocks[channel].probeGainCombobox->clear();
                // Rebuild the combobox with the new values
                        QStringList probeGainStrings;
                for(double probe_gain: scope->voltage[channel].probeGainSteps)
                        probeGainStrings << valueToString(probe_gain, UNIT_TIMES, 0);
                channelBlocks[channel].probeGainCombobox->addItems(probeGainStrings);
            }
    }
}