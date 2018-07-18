// SPDX-License-Identifier: GPL-2.0+

#include <cmath>

#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QTimer>

#include "dsowidget.h"

#include "post/graphgenerator.h"
#include "post/postprocessingsettings.h"
#include "post/ppresult.h"

#include "settings/scopesettings.h"
#include "settings/viewsettings.h"

#include "hantekdso/devicesettings.h"
#include "hantekdso/dsocontrol.h"
#include "hantekdso/modelspecification.h"
#include "scopeview/glscope.h"
#include "utils/printutils.h"
#include "viewconstants.h"
#include "widgets/levelslider.h"

DsoWidget::DsoWidget(Settings::Scope *scope, Settings::View *view, DsoControl *dsoControl, QWidget *parent)
    : QWidget(parent), m_scope(scope), m_view(view), m_spec(dsoControl->deviceSettings()->spec),
      m_deviceSettings(dsoControl->deviceSettings().get()),
      m_mainScope(new GlScopeWindow(&view->zoomviews, view, &view->screen)) {

    // The offset sliders for all possible channels
    mainSliders.offsetSlider = new LevelSlider(Qt::RightArrow);
    mainSliders.triggerLevelSlider = new LevelSlider(Qt::LeftArrow);
    mainSliders.triggerPositionSlider = new LevelSlider(Qt::DownArrow);
    mainSliders.triggerPositionSlider->addSlider(0);
    mainSliders.triggerPositionSlider->setLimits(0, 0.0, 1.0);
    mainSliders.triggerPositionSlider->setStep(0, 0.2 / (double)DIVS_TIME);
    mainSliders.triggerPositionSlider->setValue(0, dsoControl->deviceSettings()->trigger.position());
    mainSliders.triggerPositionSlider->setIndexVisible(0, true);

    connect(&view->zoomviews, &Settings::ZoomViewSettings::markerChanged, this,
            [this](int activeMarker) { updateMarkerDetails(activeMarker); });
    connect(&view->zoomviews, &Settings::ZoomViewSettings::activeMarkerChanged, this,
            [this](int activeMarker) { updateMarkerDetails(activeMarker); });
    connect(&view->zoomviews, &Settings::ZoomViewSettings::markerDataChanged, this,
            [this](int activeMarker) { updateMarkerDetails(activeMarker); });

    connect(m_mainScope->signalEmitter(), &GlScopeSignalEmitter::requestStatusText, this,
            &DsoWidget::requestStatusText);

    // The table for the settings
    settingsTriggerLabel = new QLabel(this);
    settingsTriggerLabel->setMinimumWidth(160);
    settingsTriggerLabel->setIndent(5);
    settingsRecordLengthLabel = new QLabel(this);
    settingsRecordLengthLabel->setAlignment(Qt::AlignRight);
    settingsSamplerateLabel = new QLabel(this);
    settingsSamplerateLabel->setAlignment(Qt::AlignRight);
    settingsTimebaseLabel = new QLabel(this);
    settingsTimebaseLabel->setAlignment(Qt::AlignRight);
    settingsFrequencybaseLabel = new QLabel(this);
    settingsFrequencybaseLabel->setAlignment(Qt::AlignRight);
    swTriggerStatus = new QLabel(this);
    swTriggerStatus->setMinimumWidth(30);
    swTriggerStatus->setText(tr("TR"));
    swTriggerStatus->setAlignment(Qt::AlignCenter);
    swTriggerStatus->setAutoFillBackground(true);
    swTriggerStatus->setVisible(false);

    QHBoxLayout *settingsLayout = new QHBoxLayout; ///< The table for the settings info
    settingsLayout->addWidget(swTriggerStatus);
    settingsLayout->addWidget(settingsTriggerLabel);
    settingsLayout->addWidget(settingsRecordLengthLabel, 1);
    settingsLayout->addWidget(settingsSamplerateLabel, 1);
    settingsLayout->addWidget(settingsTimebaseLabel, 1);
    settingsLayout->addWidget(settingsFrequencybaseLabel, 1);

    // The table for the marker details
    markerInfoLabel = new QLabel(this);
    markerInfoLabel->setMinimumWidth(160);
    markerTimeLabel = new QLabel(this);
    markerTimeLabel->setAlignment(Qt::AlignRight);
    markerFrequencyLabel = new QLabel(this);
    markerFrequencyLabel->setAlignment(Qt::AlignRight);
    markerTimebaseLabel = new QLabel(this);
    markerTimebaseLabel->setAlignment(Qt::AlignRight);
    markerFrequencybaseLabel = new QLabel(this);
    markerFrequencybaseLabel->setAlignment(Qt::AlignRight);

    QHBoxLayout *markerLayout = new QHBoxLayout; ///< The table for the marker details
    markerLayout->addWidget(markerInfoLabel);
    markerLayout->addWidget(markerTimeLabel, 1);
    markerLayout->addWidget(markerFrequencyLabel, 1);
    markerLayout->addWidget(markerTimebaseLabel, 1);
    markerLayout->addWidget(markerFrequencybaseLabel, 1);

    // The layout for the widgets
    QGridLayout *mainLayout = new QGridLayout; ///< The main layout for this widget
    setLayout(mainLayout);
    mainLayout->setColumnStretch(2, 1); // Scopes increase their size
    // Bars around the scope, needed because the slider-drawing-area is outside
    // the scope at min/max
    mainLayout->setColumnMinimumWidth(1, mainSliders.triggerPositionSlider->preMargin());
    mainLayout->setColumnMinimumWidth(3, mainSliders.triggerPositionSlider->postMargin());
    mainLayout->setSpacing(0);
    int row = 0;
    mainLayout->addLayout(settingsLayout, row++, 0, 1, 5);
    // 5x5 box for mainScope & mainSliders
    mainLayout->setRowMinimumHeight(row + 1, mainSliders.offsetSlider->preMargin());
    mainLayout->setRowMinimumHeight(row + 3, mainSliders.offsetSlider->postMargin());
    mainLayout->setRowStretch(row + 2, 1);
    mainLayout->addWidget(mainSliders.offsetSlider, row + 1, //
                          0, 3, 2, Qt::AlignRight);
    mainLayout->addWidget(mainSliders.triggerPositionSlider, row, //
                          1, 2, 3, Qt::AlignBottom);
    mainLayout->addWidget(QWidget::createWindowContainer(m_mainScope), row + 2, //
                          2);
    mainLayout->addWidget(mainSliders.triggerLevelSlider, row + 1, //
                          3, 3, 2, Qt::AlignLeft);
    row += 4;
    // Separators and markerLayout
    mainLayout->setRowMinimumHeight(row++, 4);
    mainLayout->addLayout(markerLayout, row++, 0, 1, 5);
    mainLayout->setRowMinimumHeight(row++, 4);
    row += 5;
    // Separator and embedded measurementLayout
    mainLayout->setRowMinimumHeight(row++, 8);
    measurementLayout = new QGridLayout;
    mainLayout->addLayout(measurementLayout, row++, 0, 1, 5);

    createChannelWidgets(palette());
    connect(scope, &Settings::Scope::mathChannelAdded, this, [this]() { createChannelWidgets(palette()); });

    // The widget itself
    setBackgroundRole(QPalette::Background);
    setAutoFillBackground(true);

    // Connect change-signals of sliders
    connect(mainSliders.offsetSlider, &LevelSlider::valueChanged, this,
            [this, scope, dsoControl](LevelSlider::IndexType index, double value) {
                if (index >= 0) {
                    if (m_scope->channel((ChannelID)index)->isMathChannel()) {
                        Settings::MathChannel *mc = (Settings::MathChannel *)m_scope->channel((ChannelID)index).get();
                        mc->setOffset(value);
                    } else
                        dsoControl->setOffset((ChannelID)index, value);
                } else {
                    m_scope->channel((ChannelID)-index - 1)->spectrum()->setOffset(value);
                }
            });

    connect(mainSliders.triggerPositionSlider, &LevelSlider::valueChanged, this,
            [this, dsoControl](int, double value) { dsoControl->setPretriggerPosition(value); });

    connect(mainSliders.triggerLevelSlider, &LevelSlider::valueChanged, this,
            [this, dsoControl](ChannelID channel, double value) {
                dsoControl->setTriggerOffset(channel, value);
                updateTriggerDetails();
            });

    // Connect signals to DSO controller and widget
    connect(scope, &Settings::Scope::frequencybaseChanged, this, &DsoWidget::updateHorizontalDetails);

    connect(m_deviceSettings, &Dso::DeviceSettings::samplerateChanged, this, &DsoWidget::updateHorizontalDetails);
    connect(m_deviceSettings, &Dso::DeviceSettings::recordLengthChanged, this, &DsoWidget::updateHorizontalDetails);
    connect(m_deviceSettings, &Dso::DeviceSettings::availableRecordLengthsChanged, this,
            &DsoWidget::updateHorizontalDetails);

    connect(&m_deviceSettings->trigger, &Dso::Trigger::positionChanged, this, [this](double position) {
        QSignalBlocker b(mainSliders.triggerPositionSlider);
        mainSliders.triggerPositionSlider->setValue(0, position);
    });
    connect(&m_deviceSettings->trigger, &Dso::Trigger::modeChanged, this, &DsoWidget::updateTriggerDetails);
    connect(&m_deviceSettings->trigger, &Dso::Trigger::sourceChanged, this, &DsoWidget::updateTriggerSource);
    connect(&m_deviceSettings->trigger, &Dso::Trigger::slopeChanged, this, &DsoWidget::updateTriggerDetails);

    // Palette for this widget

    applyColors();
    QObject::connect(view->screen.observer(), &Observer::changed, this, &DsoWidget::applyColors);

    // Apply settings and update measured values
    updateTriggerDetails();
    updateTriggerSource();
    updateHorizontalDetails();
    updateMarkerDetails(m_view->zoomviews.activeMarker());
}

void DsoWidget::updateMarkerDetails(int activeMarker) {
    if (activeMarker < 0) {
        markerTimebaseLabel->setText(QString());
        markerFrequencybaseLabel->setText(QString());
        markerInfoLabel->setText(QString());
        markerTimeLabel->setText(QString());
        markerFrequencyLabel->setText(QString());
        return;
    }
    QRectF &marker = m_view->zoomviews.get((unsigned)activeMarker).markerRect;

    const double time = marker.width() * m_deviceSettings->samplerate().timebase;

    QString infoLabelPrefix(tr("Markers"));
    infoLabelPrefix = tr("Zoom x%L1").arg(DIVS_TIME / marker.width(), -1, 'g', 3);
    markerTimebaseLabel->setText(valueToString(time / DIVS_TIME, Unit::SECONDS, 3) + tr("/div"));
    markerFrequencybaseLabel->setText(
        valueToString(marker.width() * m_scope->frequencybase() / DIVS_TIME, Unit::HERTZ, 4) + tr("/div"));
    markerInfoLabel->setText(
        infoLabelPrefix.append(":  %1  %2")
            .arg(valueToString(0.5 + marker.x() / DIVS_TIME - m_deviceSettings->trigger.position(), Unit::SECONDS, 4))
            .arg(valueToString(0.5 + (marker.x() + marker.width()) / DIVS_TIME - m_deviceSettings->trigger.position(),
                               Unit::SECONDS, 4)));

    markerTimeLabel->setText(valueToString(time, Unit::SECONDS, 4));
    markerFrequencyLabel->setText(valueToString(1.0 / time, Unit::HERTZ, 4));
}

/// \brief Update the label about the trigger settings
void DsoWidget::updateTriggerDetails() {
    const ChannelID c = m_deviceSettings->trigger.source();

    // Update the trigger details
    QPalette tablePalette = palette();
    tablePalette.setColor(QPalette::WindowText, m_view->screen.voltage(c));
    settingsTriggerLabel->setPalette(tablePalette);
    const Settings::Channel *channel = m_scope->channel(c).get();
    const double triggerlevel = channel->voltage()->triggerLevel() * channel->gain();
    QString levelString = valueToString(triggerlevel, Unit::VOLTS, 3);
    QString pretriggerString = tr("%L1%").arg((int)(m_deviceSettings->trigger.position() * 100 + 0.5));
    settingsTriggerLabel->setText(
        tr("%1  %2  %3  %4")
            .arg(channel->name(), DsoE::slopeString(m_deviceSettings->trigger.slope()), levelString, pretriggerString));

    /// \todo This won't work for special trigger sources
}

/// \brief Handles timebaseChanged signal from the horizontal dock.
/// \param timebase The timebase used for displaying the trace.
void DsoWidget::updateHorizontalDetails() {
    settingsRecordLengthLabel->setText(valueToString(m_deviceSettings->getRecordLength(), Unit::SAMPLES, 4));
    settingsFrequencybaseLabel->setText(valueToString(m_scope->frequencybase(), Unit::HERTZ, 4) + tr("/div"));
    settingsSamplerateLabel->setText(valueToString(m_deviceSettings->samplerate().samplerate, Unit::SAMPLES, 4) +
                                     tr("/s"));
    settingsTimebaseLabel->setText(valueToString(m_deviceSettings->samplerate().timebase, Unit::SECONDS, 4) +
                                   tr("/div"));
    updateMarkerDetails(m_view->zoomviews.activeMarker());
}

void DsoWidget::updateSpectrumUsed(Settings::Channel *channel, bool used) {
    mainSliders.offsetSlider->setIndexVisible(-1 - (int)channel->channelID(), used);
}

/// \brief Handles sourceChanged signal from the trigger dock.
void DsoWidget::updateTriggerSource() {
    // Change the colors of the trigger sliders
    if (m_deviceSettings->trigger.special() || m_deviceSettings->trigger.source() >= m_spec->channels) {
        mainSliders.triggerPositionSlider->setColor(0, m_view->screen.border());
    } else {
        mainSliders.triggerPositionSlider->setColor(0, m_view->screen.voltage(m_deviceSettings->trigger.source()));
    }

    for (ChannelID channel = 0; channel < m_spec->channels; ++channel) {
        QColor color = (!m_deviceSettings->trigger.special() && channel == m_deviceSettings->trigger.source())
                           ? m_view->screen.voltage(channel)
                           : m_view->screen.voltage(channel).darker();
        mainSliders.triggerLevelSlider->setColor((int)channel, color);
    }

    updateTriggerDetails();
}

/// \brief Handles usedChanged signal from the voltage dock.
/// \param channel The channel whose used-state was changed.
/// \param used The new used-state for the channel.
void DsoWidget::updateVoltageUsed(Settings::Channel *channel, bool used) {
    mainSliders.offsetSlider->setIndexVisible((int)channel->channelID(), used);
    mainSliders.triggerLevelSlider->setIndexVisible((int)channel->channelID(), used);
}

void DsoWidget::createChannelWidgets(const QPalette &palette) {
    // The table for the measurements
    QPalette tablePalette = palette;

    mainSliders.offsetSlider->removeAll();
    mainSliders.triggerLevelSlider->removeAll();

    for (ChannelWidgets *c : channelWidgets) delete c;
    channelWidgets.clear();

    int rowIndex = 0;
    for (Settings::Channel *channel : *m_scope) {
        ChannelID channelId = channel->channelID();
        mainSliders.offsetSlider->addSlider((int)channelId, channel->name());
        mainSliders.offsetSlider->setColor((int)channelId, m_view->screen.voltage(channelId));
        mainSliders.offsetSlider->setLimits((int)channelId, -1, 1);
        mainSliders.offsetSlider->setValue((int)channelId, channel->voltage()->offset());

        /// A Level slider widget can contain multiple sliders and uses an index value to identify a slider.
        /// Spectrum channels are inserted with a negative index (-1 to avoid the double 0).
        mainSliders.offsetSlider->addSlider(-1 - (int)channelId, channel->name());
        mainSliders.offsetSlider->setColor(-1 - (int)channelId, m_view->screen.spectrum(channelId));
        mainSliders.offsetSlider->setLimits(-1 - (int)channelId, -1, 1);
        mainSliders.offsetSlider->setValue(-1 - (int)channelId, channel->spectrum()->offset());

        if (channel->isMathChannel()) {
            /// device knows nothing about our math channels. Use one tick for a step size.
            double vstep = (1 - -1) / DIVS_VOLTAGE / DIVS_SUB;
            mainSliders.offsetSlider->setStep((int)channelId, vstep);
            mainSliders.offsetSlider->setStep(-1 - (int)channelId, vstep);
        } else {
            mainSliders.offsetSlider->setStep((int)channelId, m_deviceSettings->offsetAdjustStep(channelId));
            mainSliders.offsetSlider->setStep(-1 - (int)channelId, m_deviceSettings->offsetAdjustStep(channelId));

            mainSliders.triggerLevelSlider->addSlider((int)channelId);
            mainSliders.triggerLevelSlider->setColor(
                (int)channelId,
                (!m_deviceSettings->trigger.special() && channelId == m_deviceSettings->trigger.source())
                    ? m_view->screen.voltage(channelId)
                    : m_view->screen.voltage(channelId).darker());
            mainSliders.triggerLevelSlider->setLimits((int)channelId, -1, 1);
            mainSliders.triggerLevelSlider->setStep((int)channelId, m_deviceSettings->offsetAdjustStep(channelId));
            mainSliders.triggerLevelSlider->setValue((int)channelId, channel->voltage()->triggerLevel());
        }

        ChannelWidgets *cw = new ChannelWidgets(channel, m_view, m_spec, this);
        channelWidgets.push_back(cw);
        measurementLayout->addWidget(cw, rowIndex, 0);

        updateVoltageUsed(channel, channel->visible());
        // Wire slots to the ChannelWidgets instance so that they are automatically disconnected
        // when cw is destroyed.
        connect(channel, &Settings::Channel::visibleChanged, cw, 
                [this, channel](bool visible) { updateVoltageUsed(channel, visible); });

        updateSpectrumUsed(channel, channel->spectrum()->visible());
        connect(channel->spectrum(), &Settings::Spectrum::visibleChanged, cw,
                [this, channel](bool visible) { updateSpectrumUsed(channel, visible); });

        ++rowIndex;
    }
}

/// \brief Prints analyzed data.
void DsoWidget::showNew(std::shared_ptr<PPresult> data) {
    m_mainScope->showData(data);

    if (m_spec->isSoftwareTriggerDevice) {
        QPalette triggerLabelPalette = palette();
        triggerLabelPalette.setColor(QPalette::WindowText, Qt::black);
        triggerLabelPalette.setColor(QPalette::Background, data->softwareTriggerTriggered ? Qt::green : Qt::red);
        swTriggerStatus->setPalette(triggerLabelPalette);
        swTriggerStatus->setVisible(true);
    }

    settingsRecordLengthLabel->setText(valueToString(data.get()->sampleCount(), Unit::SAMPLES, 4));

    for (ChannelWidgets *widget : channelWidgets) {
        const DataChannel *sampleData = data.get()->data(widget->channel->channelID());
        if (!widget->channel->visible() || !sampleData) continue;

        // Amplitude string representation (4 significant digits)
        widget->measurementAmplitudeLabel->setText(valueToString(sampleData->amplitude(), Unit::VOLTS, 4));
        // Frequency string representation (5 significant digits)
        widget->measurementFrequencyLabel->setText(valueToString(sampleData->frequency, Unit::HERTZ, 5));
    }
}

void DsoWidget::applyColors() {
    QPalette palette;
    palette.setColor(QPalette::Background, m_view->screen.background());
    palette.setColor(QPalette::WindowText, m_view->screen.text());
    setPalette(palette);
    settingsRecordLengthLabel->setPalette(palette);
    settingsSamplerateLabel->setPalette(palette);
    settingsTimebaseLabel->setPalette(palette);
    settingsFrequencybaseLabel->setPalette(palette);
    markerTimeLabel->setPalette(palette);
    markerFrequencyLabel->setPalette(palette);
    markerTimebaseLabel->setPalette(palette);
    markerFrequencybaseLabel->setPalette(palette);
    markerInfoLabel->setPalette(palette);
}

ChannelWidgets::ChannelWidgets(Settings::Channel *channel, Settings::View *view, const Dso::ModelSpec *spec, QWidget *parent)
        : QWidget(parent), m_view(view), channel(channel), m_spec(spec) {
    QPalette tablePalette = palette();
    ChannelID channelId = channel->channelID();

    tablePalette.setColor(QPalette::WindowText, m_view->screen.voltage(channelId));
    measurementNameLabel->setText(channel->name());
    measurementNameLabel->setPalette(tablePalette);
    measurementMiscLabel->setPalette(tablePalette);
    measurementGainLabel->setPalette(tablePalette);
    measurementGainLabel->setAlignment(Qt::AlignRight);

    tablePalette.setColor(QPalette::WindowText, m_view->screen.spectrum(channelId));
    measurementMagnitudeLabel->setPalette(tablePalette);
    measurementMagnitudeLabel->setAlignment(Qt::AlignRight);
    measurementAmplitudeLabel->setPalette(tablePalette);
    measurementAmplitudeLabel->setAlignment(Qt::AlignRight);
    measurementFrequencyLabel->setPalette(tablePalette);
    measurementFrequencyLabel->setAlignment(Qt::AlignRight);

    layout->setMargin(0);

    layout->setColumnMinimumWidth(0, 64);
    layout->setColumnMinimumWidth(1, 32);
    layout->setColumnStretch(2, 2);
    layout->setColumnStretch(3, 2);
    layout->setColumnStretch(4, 3);
    layout->setColumnStretch(5, 3);

    layout->addWidget(measurementNameLabel, 0, 0);
    layout->addWidget(measurementMiscLabel, 0, 1);
    layout->addWidget(measurementGainLabel, 0, 2);
    layout->addWidget(measurementMagnitudeLabel, 0, 3);
    layout->addWidget(measurementAmplitudeLabel, 0, 4);
    layout->addWidget(measurementFrequencyLabel, 0, 5);

    setMeasurementVisible();

    if (!channel->isMathChannel()) {
        updateVoltageCoupling();
        connect(channel->voltage(), &Dso::Channel::couplingIndexChanged, this, &ChannelWidgets::updateVoltageCoupling);
    } else {
        updateMathMode();
        connect((Settings::MathChannel *)channel, &Settings::MathChannel::mathModeChanged, this, &ChannelWidgets::updateMathMode);
    }
    updateVoltageUsed();
    updateSpectrumDetails();

    connect(channel, &Settings::Channel::gainChanged, this, &ChannelWidgets::updateVoltageDetails);
    connect(channel, &Settings::Channel::visibleChanged, this, &ChannelWidgets::updateVoltageUsed);

    connect(channel->spectrum(), &Settings::Spectrum::magnitudeChanged, this, &ChannelWidgets::updateSpectrumDetails);
    connect(channel->spectrum(), &Settings::Spectrum::visibleChanged, this, &ChannelWidgets::updateSpectrumDetails);
}

/// \brief Show/Hide a line of the measurement table.
void ChannelWidgets::setMeasurementVisible() {
    bool visible = channel->visible() || channel->spectrum()->visible();

    measurementNameLabel->setVisible(visible);
    measurementMiscLabel->setVisible(visible);

    measurementAmplitudeLabel->setVisible(visible);
    measurementFrequencyLabel->setVisible(visible);
    if (!visible) {
        measurementGainLabel->setText(QString());
        measurementAmplitudeLabel->setText(QString());
        measurementFrequencyLabel->setText(QString());
    }

    measurementGainLabel->setVisible(channel->visible());
    if (!channel->visible()) { measurementGainLabel->setText(QString()); }

    measurementMagnitudeLabel->setVisible(channel->spectrum()->visible());
    if (!channel->spectrum()->visible()) {
        measurementMagnitudeLabel->setText(QString());
    }
}

/// \brief Handles couplingChanged signal from the voltage dock.
/// \param channel The channel whose coupling was changed.
void ChannelWidgets::updateVoltageCoupling() {
    measurementMiscLabel->setText(DsoE::couplingString(channel->voltage()->coupling(m_spec)));
}

/// \brief Handles modeChanged signal from the voltage dock.
void ChannelWidgets::updateMathMode() {
    measurementMiscLabel->setText(
        PostProcessingE::mathModeString(((Settings::MathChannel *)channel)->mathMode()));
}

/// \brief Handles usedChanged signal from the voltage dock.
/// \param channel The channel whose used-state was changed.
/// \param used The new used-state for the channel.
void ChannelWidgets::updateVoltageUsed() {
    setMeasurementVisible();
    updateVoltageDetails();
}

/// \brief Update the label about the trigger settings
void ChannelWidgets::updateVoltageDetails() {
    setMeasurementVisible();

    if (channel->visible()) {
        const double gain = channel->gain();
        measurementGainLabel->setText(valueToString(gain, Unit::VOLTS, 3) + tr("/div"));
    } else
        measurementGainLabel->setText(QString());
}


/// \brief Update the label about the trigger settings
void ChannelWidgets::updateSpectrumDetails() {
    setMeasurementVisible();

    if (channel->spectrum()->visible())
        measurementMagnitudeLabel->setText(
            valueToString(channel->spectrum()->magnitude(), Unit::DECIBEL, 3) + tr("/div"));
    else
        measurementMagnitudeLabel->setText(QString());
}
