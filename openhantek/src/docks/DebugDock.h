// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QDockWidget>
#include <QGridLayout>

#include "hantekprotocol/codes.h"
#include "utils/debugnotify.h"

namespace Settings {
class Scope;
}
class DsoControl;
class Debug;

/// \brief Dock window with a log view and manual command
/// It contains the settings for the timebase and the display format.
class DebugDock : public QDockWidget {
    Q_OBJECT

  public:
    /// \brief Initializes the horizontal axis docking window.
    /// \param settings The target settings object.
    /// \param parent The parent widget.
    /// \param flags Flags for the window manager.
    DebugDock(DsoControl *dsocontrol, QWidget *parent, Qt::WindowFlags flags = 0);

  protected:
    void closeEvent(QCloseEvent *event);

    Debug *m_model;
  signals:
    void manualCommand(bool isBulk, HantekE::BulkCode bulkCode, HantekE::ControlCode controlCode, const QByteArray &data);
};
