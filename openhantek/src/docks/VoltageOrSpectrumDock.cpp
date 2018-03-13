// SPDX-License-Identifier: GPL-2.0+

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>

#include <cmath>

#include "VoltageOrSpectrumDock.h"
#include "dockwindows.h"

#include "hantekdso/dsocontrol.h"
#include "scopesettings.h"
#include "utils/enumhelper.h"
#include "utils/printutils.h"
#include "widgets/sispinbox.h"

template <typename... Args> struct SELECT {
    template <typename C, typename R> static constexpr auto OVERLOAD_OF(R (C::*pmf)(Args...)) -> decltype(pmf) {
        return pmf;
    }
};

VoltageOrSpectrumDock::VoltageOrSpectrumDock(bool isSpectrum, Settings::Scope *scope, DsoControl *dsocontrol,
                                             QWidget *parent, Qt::WindowFlags flags)
    : QDockWidget(isSpectrum ? tr("Spectrum") : tr("Voltage"), parent, flags), m_isSpectrum(isSpectrum) {

    const Dso::DeviceSettings *deviceSettings = dsocontrol->deviceSettings().get();

    // Initialize lists for comboboxes
    for (DsoE::Coupling c : deviceSettings->spec->couplings) couplingStrings.append(DsoE::couplingString(c));
    for (auto e : Enum<PostProcessingE::MathMode>()) { modeStrings.append(PostProcessingE::mathModeString(e)); }
    magnitudeSteps = {1e0, 2e0, 3e0, 6e0, 1e1, 2e1, 3e1, 6e1, 1e2, 2e2, 3e2, 6e2};
    for (const auto &magnitude : magnitudeSteps) magnitudeStrings << valueToString(magnitude, Unit::DECIBEL, 0);

    QWidget *dockWidget = new QWidget();
    dockLayout = new QVBoxLayout();
    SetupDockWidget(this, dockWidget, dockLayout);

    // Create widgets for each channel and connect to math channels changed signal to recreate if necessary
    // Initialize elements
    for (Settings::Channel *channel : *scope) {
        if (!channel->isMathChannel()) createChannelWidgets(scope, dsocontrol, deviceSettings, channel);
    }

    auto btnAdd = new QPushButton(tr("Add math channel"), this);
    connect(btnAdd, &QPushButton::clicked, this, [scope, deviceSettings, dsocontrol]() {
        scope->addMathChannel(dsocontrol->channelUsage(), deviceSettings);
    });
    dockLayout->addWidget(btnAdd);

    for (Settings::Channel *channel : *scope) {
        if (channel->isMathChannel()) createChannelWidgets(scope, dsocontrol, deviceSettings, channel);
    }

    connect(scope, &Settings::Scope::mathChannelAdded, this,
            [this, scope, dsocontrol, deviceSettings](Settings::Channel *channel) {
                createChannelWidgets(scope, dsocontrol, deviceSettings, channel);
            });
}

/// \brief Don't close the dock, just hide it
/// \param event The close event that should be handled.
void VoltageOrSpectrumDock::closeEvent(QCloseEvent *event) {
    hide();
    event->accept();
}

void VoltageOrSpectrumDock::setMagnitude(QComboBox *magnitudeComboBox, double magnitude) {
    QSignalBlocker blocker(magnitudeComboBox);

    auto indexIt = std::find(magnitudeSteps.begin(), magnitudeSteps.end(), magnitude);
    if (indexIt == magnitudeSteps.end()) return;
    int index = (int)std::distance(magnitudeSteps.begin(), indexIt);
    magnitudeComboBox->setCurrentIndex(index);
}

void VoltageOrSpectrumDock::fillGainBox(QComboBox *gainComboBox, Settings::Scope *scope, DsoControl *dsocontrol,
                                        Settings::Channel *channel) {
    QSignalBlocker b(gainComboBox);
    gainComboBox->clear();
    if (scope->useHardwareGainSteps()) {
        for (auto &gainStep : dsocontrol->specification()->gain)
            gainComboBox->addItem(valueToString(gainStep.gain, Unit::VOLTS, 0), gainStep.gainIdentificator);
        gainComboBox->setCurrentIndex((int)channel->voltage()->gainStepIndex());
    } else {
        int index = -1;
        for (unsigned i = 0; i < gainValue.size(); ++i) {
            if (channel->gain() >= gainValue[i]) index = (int)i;
            gainComboBox->addItem(valueToString(gainValue[i], Unit::VOLTS, 0));
        }
        gainComboBox->setToolTip(
            tr("Hardware Gain Index: %1").arg(findMatchingHardwareGainId(channel->gain(), dsocontrol)));
        gainComboBox->setCurrentIndex(index);
    }
}

int VoltageOrSpectrumDock::findMatchingHardwareGainId(double gain, DsoControl *dsocontrol) {
    int matchingHardwareIndex = 0;
    auto hwGains = dsocontrol->specification()->gain;
    for (unsigned hIndex = 0; hIndex < hwGains.size(); ++hIndex) {
        if (gain < hwGains[hIndex].gain) { break; }
        matchingHardwareIndex = hIndex;
    }
    return matchingHardwareIndex;
}

void VoltageOrSpectrumDock::createChannelWidgets(Settings::Scope *scope, DsoControl *dsocontrol,
                                                 const Dso::DeviceSettings *deviceSettings,
                                                 Settings::Channel *channel) {

    // Create a common parent, that is deleted when the dock is deleted (due to having the dock as parent)
    // as well as getting deleted when the corresponding channel vanishes.
    QGroupBox *channelParent = new QGroupBox(this);
    channelParent->setTitle(channel->name());
    channelParentWidgets.push_back(channelParent);
    channelParent->setCheckable(true);
    connect(channel, &QObject::destroyed, channelParent, &QObject::deleteLater);
    dockLayout->addWidget(channelParent);

    auto layout = new QVBoxLayout(channelParent);
    channelParent->setLayout(layout);

    // Spectrum properties
    if (isSpectrum()) {
        channelParent->setChecked(channel->spectrum()->visible());

        QComboBox *magnitudeComboBox = new QComboBox(channelParent);
        magnitudeComboBox->addItems(magnitudeStrings);
        layout->addWidget(magnitudeComboBox);
        setMagnitude(magnitudeComboBox, channel->spectrum()->magnitude());

        // Connect widgets --> settings
        connect(channelParent, &QGroupBox::toggled,
                [scope, channel](bool checked) { channel->setSpectrumVisible(checked); });

        connect(
            magnitudeComboBox, SELECT<int>::OVERLOAD_OF(&QComboBox::currentIndexChanged), channelParent,
            [scope, channel, this](unsigned index) { channel->spectrum()->setMagnitude(magnitudeSteps.at(index)); });
        // Connect settings --> widgets
        connect(channel->spectrum(), &Settings::Spectrum::magnitudeChanged, channelParent,
                [this, magnitudeComboBox](const Settings::Spectrum *spectrum) {
                    setMagnitude(magnitudeComboBox, spectrum->magnitude());
                });

        connect(channel->spectrum(), &Settings::Spectrum::visibleChanged, channelParent, [channelParent](bool visible) {
            QSignalBlocker blocker(channelParent);
            channelParent->setChecked(visible);
        });
        return;
    }

    // Voltage properties

    channelParent->setChecked(channel->visible());
    // Connect widgets --> settings
    connect(channelParent, &QGroupBox::toggled, channelParent,
            [channel, dsocontrol](bool checked) { channel->setVoltageVisible(checked); });
    // Connect settings --> widgets
    connect(channel, &Settings::Channel::visibleChanged, channelParent, [channelParent](bool used) {
        QSignalBlocker blocker(channelParent);
        channelParent->setChecked(used);
    });

    auto sublayout = new QHBoxLayout; // Invert + Gain + coupling next to each other
    layout->addLayout(sublayout);

    /////// Invert ///////
    QCheckBox *invertCheckBox = new QCheckBox(tr("INV"), channelParent);
    invertCheckBox->setToolTip(tr("Invert channel on x-axes"));
    invertCheckBox->setChecked(channel->inverted());
    sublayout->addWidget(invertCheckBox);
    connect(invertCheckBox, &QAbstractButton::toggled, channelParent,
            [this, channel](bool checked) { channel->setInverted(checked); });
    connect(channel, &Settings::Channel::invertedChanged, channelParent, [invertCheckBox](bool inverted) {
        QSignalBlocker blocker(invertCheckBox);
        invertCheckBox->setChecked(inverted);
    });

    /////// The voltage gain steps in V ///////
    QComboBox *gainComboBox = new QComboBox(channelParent);
    fillGainBox(gainComboBox, scope, dsocontrol, channel);
    connect(scope, &Settings::Scope::useHardwareGainChanged, this, [this, gainComboBox, scope, dsocontrol, channel]() {
        fillGainBox(gainComboBox, scope, dsocontrol, channel);
    });
    connect(gainComboBox, SELECT<int>::OVERLOAD_OF(&QComboBox::currentIndexChanged), channelParent,
            [channel, dsocontrol, scope, this, gainComboBox](int index) {
                double newGain;
                if (scope->useHardwareGainSteps()) {
                    newGain = dsocontrol->specification()->gain[(unsigned)index].gain;
                    dsocontrol->setGain(channel->channelID(), (unsigned)index);
                } else {
                    newGain = gainValue[index];
                    int matchingHardwareIndex = findMatchingHardwareGainId(newGain, dsocontrol);
                    gainComboBox->setToolTip(tr("Hardware Gain Index: %1").arg(matchingHardwareIndex));
                    dsocontrol->setGain(channel->channelID(), (unsigned)matchingHardwareIndex);
                }
                channel->setGain(newGain);
            });
    connect(channel->voltage(), &Dso::Channel::gainStepIndexChanged, channelParent,
            [gainComboBox, scope](unsigned gainId) {
                if (!scope->useHardwareGainSteps()) return; ///< Do nothing if we are not using hardware gain steps
                QSignalBlocker blocker(gainComboBox);
                gainComboBox->setCurrentIndex((int)gainId);
            });
    sublayout->addWidget(gainComboBox);

    if (channel->isMathChannel()) {
        Settings::MathChannel *mathChannel = static_cast<Settings::MathChannel *>(channel);
        auto mathlayout = new QHBoxLayout; // Channel selection and mathmode next to each other
        layout->addLayout(mathlayout);

        QComboBox *mathChannel1 = new QComboBox(channelParent);
        mathlayout->addWidget(mathChannel1);
        QComboBox *mathModeComboBox = new QComboBox(channelParent);
        mathlayout->addWidget(mathModeComboBox);
        QComboBox *mathChannel2 = new QComboBox(channelParent);
        mathlayout->addWidget(mathChannel2);

        for (Settings::Channel *c : *scope) {
            if (c->isMathChannel()) continue;
            mathChannel1->addItem(c->name(), c->channelID());
            mathChannel2->addItem(c->name(), c->channelID());
            if (mathChannel->firstID() == c->channelID()) mathChannel1->setCurrentIndex(mathChannel1->count() - 1);
            if (mathChannel->secondID() == c->channelID()) mathChannel2->setCurrentIndex(mathChannel2->count() - 1);
        }

        mathModeComboBox->addItems(modeStrings);
        mathModeComboBox->setCurrentIndex((int)mathChannel->mathMode());

        // Connect widgets --> settings
        connect(mathModeComboBox, SELECT<int>::OVERLOAD_OF(&QComboBox::currentIndexChanged), channelParent,
                [mathChannel](int index) { mathChannel->setMathMode((PostProcessingE::MathMode)index); });
        connect(mathChannel1, SELECT<int>::OVERLOAD_OF(&QComboBox::currentIndexChanged), channelParent,
                [mathChannel, deviceSettings](int index) {
                    mathChannel->setFirstChannel((unsigned)index, deviceSettings->voltage[(unsigned)index]);
                });
        connect(mathChannel2, SELECT<int>::OVERLOAD_OF(&QComboBox::currentIndexChanged), channelParent,
                [mathChannel, deviceSettings](int index) {
                    mathChannel->setSecondChannel((unsigned)index, deviceSettings->voltage[(unsigned)index]);
                });
        // Connect settings --> widgets
        connect(mathChannel, &Settings::MathChannel::mathModeChanged, channelParent,
                [mathModeComboBox](const Settings::Channel *channel) {
                    QSignalBlocker blocker(mathModeComboBox);
                    mathModeComboBox->setCurrentIndex((int)((Settings::MathChannel *)channel)->mathMode());
                });
        connect(mathChannel, &Settings::MathChannel::firstChannelChanged, channelParent,
                [mathChannel1](ChannelID channel) {
                    if (channel == UINT_MAX) return;
                    QSignalBlocker blocker(mathChannel1);
                    mathChannel1->setCurrentIndex((int)channel);
                });
        connect(mathChannel, &Settings::MathChannel::firstChannelChanged, channelParent,
                [mathChannel2](ChannelID channel) {
                    if (channel == UINT_MAX) return;
                    QSignalBlocker blocker(mathChannel2);
                    mathChannel2->setCurrentIndex((int)channel);
                });

        auto btnRemove = new QPushButton(tr("Remove"), channelParent);
        connect(btnRemove, &QPushButton::clicked, channelParent,
                [channel, scope]() { scope->removeMathChannel(channel->channelID()); });
        layout->addWidget(btnRemove);
        connect(channelParent, &QGroupBox::toggled, this, [btnRemove](bool) { btnRemove->setEnabled(true); });
        btnRemove->setEnabled(true);
    } else {
        QComboBox *couplingComboBox = new QComboBox(channelParent);
        couplingComboBox->addItems(couplingStrings);
        couplingComboBox->setCurrentIndex((int)channel->voltage()->couplingIndex());
        // Connect widgets --> settings
        connect(
            couplingComboBox, SELECT<int>::OVERLOAD_OF(&QComboBox::currentIndexChanged), channelParent,
            [channel, dsocontrol](int index) { dsocontrol->setCoupling(channel->channelID(), (DsoE::Coupling)index); });
        // Connect settings --> widgets
        connect(channel->voltage(), &Dso::Channel::couplingIndexChanged, channelParent,
                [couplingComboBox](unsigned couplingIndex) {
                    QSignalBlocker blocker(couplingComboBox);
                    couplingComboBox->setCurrentIndex((int)couplingIndex);
                });
        sublayout->addWidget(couplingComboBox);
    }
}
