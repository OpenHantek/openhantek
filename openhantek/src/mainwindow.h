#pragma once
#include "post/ppresult.h"
#include <QMainWindow>
#include <memory>

class SpectrumGenerator;
class DsoControl;
namespace Settings {
class DsoSettings;
}
class DsoWidget;
class HorizontalDock;
class TriggerDock;
class SpectrumDock;
class VoltageOrSpectrumDock;
namespace Exporter {
class Registry;
}
namespace Ui {
class MainWindow;
}
class SelfCalibration;

/// \brief The main window of the application.
/// The main window contains the classic oszilloscope-screen and the gui
/// elements used to control the oszilloscope.
class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(DsoControl *dsoControl, Settings::DsoSettings *mSettings,
                        Exporter::Registry *exporterRegistry, SelfCalibration *selfCalibration, QWidget *parent = 0);
    ~MainWindow();
  public slots:
    void showNewData(std::shared_ptr<PPresult> data);
    void exporterStatusChanged(const QString &exporterName, const QString &status);

  protected:
    void closeEvent(QCloseEvent *event) override;

  private:
    Ui::MainWindow *ui;

    // Central widgets
    DsoWidget *dsoWidget;

    // Settings used for the whole program
    Settings::DsoSettings *mSettings;
    Exporter::Registry *exporterRegistry;
};
