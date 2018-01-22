// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QDockWidget>
#include <QGridLayout>

#include "hantekprotocol/codes.h"
#include "utils/debugnotify.h"

namespace Dso {
struct ModelSpec;
}
class DsoControl;
namespace Debug {
class LogModel;
}
class SelfCalibration;

/// \brief Dock window for adjusting the gain factor
/// It contains the settings for the timebase and the display format.
class GainAdjustDock : public QDockWidget {
    Q_OBJECT

  public:
    /// \brief Initializes the dock
    /// \param settings The target settings object.
    /// \param parent The parent widget.
    /// \param flags Flags for the window manager.
    GainAdjustDock(DsoControl *dsocontrol, SelfCalibration *selfCalibration, QWidget *parent,
                   Qt::WindowFlags flags = 0);

  protected:
    void closeEvent(QCloseEvent *event);
  signals:
    void selfCalibrationFinished();
};
