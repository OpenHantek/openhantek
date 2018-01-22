// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <memory>

#include "levelslider.h"
#include "post/ppresult.h"

class SpectrumGenerator;
namespace Settings {
class Scope;
class View;
class Channel;
}
namespace Dso {
class DeviceSettings;
struct ModelSpec;
}
class GlScopeWindow;
class DsoControl;

class ChannelWidgets : protected QWidget {
  public:
    ChannelWidgets(Settings::Channel *channel, QGridLayout *measurementLayout, QWidget *parent = nullptr)
        : QWidget(parent), measurementLayout(measurementLayout), channel(channel) {}
    ChannelWidgets(const ChannelWidgets &) = default;
    virtual ~ChannelWidgets() {
        delete measurementNameLabel;
        delete measurementMiscLabel;
        delete measurementGainLabel;
        delete measurementMagnitudeLabel;
        delete measurementAmplitudeLabel;
        delete measurementFrequencyLabel;
    }
    inline QWidget *root() { return this; }

  public:
    QGridLayout *measurementLayout;                       ///< The table for the signal details
    QLabel *measurementNameLabel = new QLabel(this);      ///< The name of the channel
    QLabel *measurementMiscLabel = new QLabel(this);      ///< Coupling or math mode
    QLabel *measurementGainLabel = new QLabel(this);      ///< The gain for the voltage (V/div)
    QLabel *measurementMagnitudeLabel = new QLabel(this); ///< The magnitude for the spectrum (dB/div)
    QLabel *measurementAmplitudeLabel = new QLabel(this); ///< Amplitude of the signal (V)
    QLabel *measurementFrequencyLabel = new QLabel(this); ///< Frequency of the signal (Hz)

    Settings::Channel *channel;
};

/// \brief The widget for the oszilloscope-screen
/// This widget contains the scopes and all level sliders.
class DsoWidget : public QWidget {
    Q_OBJECT

    struct Sliders {
        LevelSlider *offsetSlider;          ///< The sliders for the graph offsets
        LevelSlider *triggerPositionSlider; ///< The slider for the pretrigger
        LevelSlider *triggerLevelSlider;    ///< The sliders for the trigger level
    };

  public:
    /// \brief Initializes the components of the oszilloscope-screen.
    /// \param settings The settings object containing the oscilloscope settings.
    /// \param dataAnalyzer The data analyzer that should be used as data source.
    /// \param parent The parent widget.
    /// \param flags Flags for the window manager.
    DsoWidget(Settings::Scope *scope, Settings::View *view, DsoControl *dsoControl, QWidget *parent = 0);

    // Data arrived
    void showNew(std::shared_ptr<PPresult> data);

    inline GlScopeWindow *scopeScreen() { return m_mainScope; }

  protected:
    std::vector<ChannelWidgets *> channelWidgets;

    Sliders mainSliders;
    QGridLayout *measurementLayout = new QGridLayout; ///< The table for the signal details

    QLabel *settingsTriggerLabel;       ///< The trigger details
    QLabel *settingsRecordLengthLabel;  ///< The record length
    QLabel *settingsSamplerateLabel;    ///< The samplerate
    QLabel *settingsTimebaseLabel;      ///< The timebase of the main scope
    QLabel *settingsFrequencybaseLabel; ///< The frequencybase of the main scope

    QLabel *swTriggerStatus; ///< The status of SW trigger

    QLabel *markerInfoLabel;          ///< The info about the zoom factor
    QLabel *markerTimeLabel;          ///< The time period between the markers
    QLabel *markerFrequencyLabel;     ///< The frequency for the time period
    QLabel *markerTimebaseLabel;      ///< The timebase for the zoomed scope
    QLabel *markerFrequencybaseLabel; ///< The frequencybase for the zoomed scope

    Settings::Scope *m_scope;
    Settings::View *m_view;
    const Dso::ModelSpec *m_spec;
    const Dso::DeviceSettings *m_deviceSettings;

    GlScopeWindow *m_mainScope; ///< The scope screen

  private:
    void applyColors();
    void createChannelWidgets(const QPalette &palette);
    void setMeasurementVisible(ChannelWidgets *channelWidgets);
    void updateSpectrumDetails(ChannelWidgets *channelWidgets);
    void updateTriggerDetails();
    void updateVoltageDetails(ChannelWidgets *channelWidgets);

    void updateHorizontalDetails();

    // Trigger
    void updateTriggerSource();

    // Spectrum
    void updateSpectrumMagnitude(ChannelWidgets *channelWidgets);
    void updateSpectrumUsed(ChannelWidgets *channelWidgets, bool used);

    // Vertical axis
    void updateVoltageCoupling(ChannelWidgets *channelWidgets);
    void updateMathMode(ChannelWidgets *channelWidgets);
    void updateVoltageUsed(ChannelWidgets *channelWidgets, bool used);

    /// Update the labels about the active marker measurements
    void updateMarkerDetails(int zoomviewIndex);
  signals:
    /**
     * @brief Request to show a status text message
     * @param text The status text
     */
    void requestStatusText(const QString &text);
};
