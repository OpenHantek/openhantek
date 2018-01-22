// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QGridLayout>
#include <QLabel>

#include "hantekdso/enums.h"

class SiSpinBox;
namespace Settings {
class Scope;
}
namespace Dso {
struct ModelSpec;
}
class DsoControl;

/// \brief Dock window for the trigger settings.
/// It contains the settings for the trigger mode, source and slope.
class TriggerDock : public QDockWidget {
    Q_OBJECT

  public:
    /// \brief Initializes the trigger settings docking window.
    /// \param settings The target settings object.
    /// \param spec
    /// \param parent The parent widget.
    /// \param flags Flags for the window manager.
    TriggerDock(Settings::Scope *scope, DsoControl *dsocontrol, QWidget *parent, Qt::WindowFlags flags = 0);

  protected:
    void closeEvent(QCloseEvent *event);
};
