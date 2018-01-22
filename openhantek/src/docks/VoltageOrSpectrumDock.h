// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QGridLayout>
#include <QLabel>

#include "hantekdso/modelspecification.h"
#include "post/postprocessingsettings.h"
#include "scopesettings.h"

class SiSpinBox;
class DsoControl;

/// \brief Dock window for the voltage channel settings.
/// It contains the settings for gain and coupling for both channels and
/// allows to enable/disable the channels.
class VoltageOrSpectrumDock : public QDockWidget {
    Q_OBJECT

  public:
    /// \brief Initializes the vertical axis docking window.
    /// \param settings The target settings object.
    /// \param parent The parent widget.
    /// \param flags Flags for the window manager.
    VoltageOrSpectrumDock(bool isSpectrum, Settings::Scope *scope, DsoControl *dsocontrol, QWidget *parent,
                          Qt::WindowFlags flags = 0);
    inline bool isSpectrum() const { return m_isSpectrum; }

  protected:
    void closeEvent(QCloseEvent *event);
    void createChannelWidgets(Settings::Scope *scope, DsoControl *dsocontrol,
                              const Dso::DeviceSettings *deviceSettings, Settings::Channel *channel);
    void setMagnitude(QComboBox *magnitudeComboBox, double magnitude);
    void fillGainBox(QComboBox *gainComboBox, Settings::Scope *scope, DsoControl *dsocontrol,
                     Settings::Channel *channel);
    int findMatchingHardwareGainId(double gain,DsoControl *dsocontrol);

    QVBoxLayout *dockLayout;
    std::list<QWidget *> channelParentWidgets;
    const bool m_isSpectrum;

    /// Selectable voltage gain steps in V, if use-hardwareGainSteps is disabled
    std::vector<double> gainValue = {1e-2, 2e-2, 5e-2, 1e-1, 2e-1, 5e-1, 1e0, 2e0, 5e0};

    std::vector<double> magnitudeSteps; ///< The selectable magnitude steps in dB/div
    QStringList magnitudeStrings;       ///< String representations for the magnitude steps
    QStringList couplingStrings;        ///< The strings for the couplings
    QStringList modeStrings;            ///< The strings for the math mode
};
