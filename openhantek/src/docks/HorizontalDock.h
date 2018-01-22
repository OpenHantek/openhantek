// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QDockWidget>
#include <QGridLayout>

#include <vector>

namespace Settings {
class Scope;
}
class DsoControl;

/// \brief Dock window for the horizontal axis.
/// It contains the settings for the timebase and the display format.
class HorizontalDock : public QDockWidget {
    Q_OBJECT

  public:
    /// \brief Initializes the horizontal axis docking window.
    /// \param settings The target settings object.
    /// \param parent The parent widget.
    /// \param flags Flags for the window manager.
    HorizontalDock(Settings::Scope *scope, DsoControl *dsocontrol, QWidget *parent,
                   Qt::WindowFlags flags = 0);

  protected:
    void closeEvent(QCloseEvent *event);
};
