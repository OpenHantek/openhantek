#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <algorithm>

#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QStringBuilder>
#include <QSurfaceFormat>

#include "docks/DebugDock.h"
#include "docks/HorizontalDock.h"
#include "docks/TriggerDock.h"
#include "docks/VoltageOrSpectrumDock.h"
#include "docks/dockwindows.h"
#include "docks/gainAdjustDock.h"

#include "configdialog/configdialog.h"

#include "exporting/exporterinterface.h"
#include "exporting/exporterregistry.h"

#include "hantekdso/dsocontrol.h"
#include "hantekdso/dsoloop.h"
#include "hantekdso/dsomodel.h"

#include "iconfont/QtAwesome.h"
#include "settings/settings.h"
#include "usb/usbdevice.h"
#include "viewconstants.h"
#include "widgets/dsowidget.h"

using namespace std;

MainWindow::MainWindow(DsoControl *dsoControl, Settings::DsoSettings *settings, Exporter::Registry *exporterRegistry,
                       SelfCalibration *selfCalibration, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), mSettings(settings), exporterRegistry(exporterRegistry) {
    ui->setupUi(this);
    ui->actionSave->setIcon(iconFont->icon(fa::save));
    ui->actionAbout->setIcon(iconFont->icon(fa::questioncircle));
    ui->actionOpen->setIcon(iconFont->icon(fa::folderopen));
    ui->actionSampling->setIcon(iconFont->icon(fa::pause,
                                               {std::make_pair("text-selected-off", QChar(fa::play)),
                                                std::make_pair("text-off", QChar(fa::play)),
                                                std::make_pair("text-active-off", QChar(fa::play))}));
    ui->actionSettings->setIcon(iconFont->icon(fa::gear));
    ui->actionDigital_phosphor->setIcon(QIcon(":/images/digitalphosphor.svg"));
    ui->actionAddMarker->setIcon(iconFont->icon(fa::searchplus));
    ui->actionRemoveMarker->setIcon(iconFont->icon(fa::trash));
    ui->actionReport_an_issue->setIcon(iconFont->icon(fa::bug));

    // Window title
    setWindowIcon(QIcon(":openhantek.png"));
#ifdef DEBUG
    const char *titleText = "OpenHantek (Debug Mode) - Device %1 - Renderer %2";
#else
    const char *titleText = "OpenHantek - Device %1 - Renderer %2";
#endif
    const QString modelName = QString::fromStdString(dsoControl->getDevice()->getModel()->name);
    const QString rendererName =
        QSurfaceFormat::defaultFormat().renderableType() == QSurfaceFormat::OpenGL ? "OpenGL" : "OpenGL ES";
    setWindowTitle(tr(titleText).arg(modelName).arg(rendererName));

    setDockOptions(dockOptions() | QMainWindow::GroupedDragging);

    for (auto *exporter : *exporterRegistry) {
        QAction *action = new QAction(exporter->icon(), exporter->name(), this);
        action->setShortcut(exporter->shortcut());
        action->setCheckable(exporter->type() == Exporter::ExporterInterface::Type::ContinousExport);
        connect(action, &QAction::triggered, [exporter, exporterRegistry](bool checked) {
            if (exporter->type() == Exporter::ExporterInterface::Type::ContinousExport && !checked) {
                exporterRegistry->stopContinous(exporter);
            } else
                exporterRegistry->exportNow(exporter);
        });
        ui->menuExport->addAction(action);
        ui->toolBar->insertAction(ui->toolBar->actions()[0], action);
    }

    Settings::Scope *scope = &(mSettings->scope);
    Settings::View *view = &(mSettings->view);

    registerDockMetaTypes();

    // Docking windows
    // Create dock windows before the dso widget, they fix messed up settings
    GainAdjustDock *gainAdjustDock = new GainAdjustDock(dsoControl, selfCalibration, this);
    DebugDock *debugDock = new DebugDock(dsoControl, this);
    HorizontalDock *horizontalDock = new HorizontalDock(scope, dsoControl, this);
    TriggerDock *triggerDock = new TriggerDock(scope, dsoControl, this);
    VoltageOrSpectrumDock *spectrumDock = new VoltageOrSpectrumDock(true, scope, dsoControl, this);
    VoltageOrSpectrumDock *voltageDock = new VoltageOrSpectrumDock(false, scope, dsoControl, this);

    addDockWidget(Qt::RightDockWidgetArea, horizontalDock);
    addDockWidget(Qt::RightDockWidgetArea, triggerDock);
    addDockWidget(Qt::RightDockWidgetArea, voltageDock);
    addDockWidget(Qt::RightDockWidgetArea, spectrumDock);
    addDockWidget(Qt::LeftDockWidgetArea, debugDock);
    addDockWidget(Qt::LeftDockWidgetArea, gainAdjustDock);

    gainAdjustDock->hide();

    ui->actionDocks->setMenu(createPopupMenu());

#ifdef DEBUG
    debugDock->show();
#else
    debugDock->hide();
#endif

    restoreGeometry(mSettings->mainWindowGeometry);
    restoreState(mSettings->mainWindowState);

    // Central oszilloscope widget
    dsoWidget = new DsoWidget(&mSettings->scope, &mSettings->view, dsoControl, this);
    setCentralWidget(dsoWidget);
    connect(dsoWidget, &DsoWidget::requestStatusText, statusBar(),
            [this](const QString &text) { statusBar()->showMessage(text, 1200); });

    // Started/stopped signals from oscilloscope
    connect(dsoControl, &DsoControl::samplingStatusChanged, [this, dsoControl](bool enabled) {
        QSignalBlocker blocker(this->ui->actionSampling);
        if (enabled) {
            this->ui->actionSampling->setText(tr("&Stop"));
            this->ui->actionSampling->setStatusTip(tr("Stop the oscilloscope"));
        } else {
            this->ui->actionSampling->setText(tr("&Start"));
            this->ui->actionSampling->setStatusTip(tr("Start the oscilloscope"));
        }
        this->ui->actionSampling->setChecked(enabled);
    });
    connect(this->ui->actionSampling, &QAction::triggered, dsoControl->loopControl(), &DsoLoop::enableSampling);
    this->ui->actionSampling->setChecked(dsoControl->loopControl()->isSampling());

    connect(ui->actionOpen, &QAction::triggered, [this, dsoControl]() {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open file"), "", tr("Settings (*.ini)"));
        if (!fileName.isEmpty()) {
            if (mSettings->setFilename(fileName)) { mSettings->load(dsoControl->channelUsage()); }
        }
    });

    connect(ui->actionSave, &QAction::triggered, [this]() {
        mSettings->mainWindowGeometry = saveGeometry();
        mSettings->mainWindowState = saveState();
        mSettings->save();
    });

    connect(ui->actionSave_as, &QAction::triggered, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save settings"), "", tr("Settings (*.ini)"));
        if (fileName.isEmpty()) return;
        mSettings->mainWindowGeometry = saveGeometry();
        mSettings->mainWindowState = saveState();
        mSettings->setFilename(fileName);
        mSettings->save();
    });

    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

    connect(ui->actionSettings, &QAction::triggered, [this]() {
        mSettings->mainWindowGeometry = saveGeometry();
        mSettings->mainWindowState = saveState();

        DsoConfigDialog *configDialog = new DsoConfigDialog(this->mSettings, this);
        configDialog->setModal(true);
        configDialog->show();
    });

    connect(this->ui->actionDigital_phosphor, &QAction::toggled, [this](bool enabled) {
        mSettings->view.setDigitalPhosphor(enabled, mSettings->view.digitalPhosphorDepth());

        if (mSettings->view.digitalPhosphor())
            this->ui->actionDigital_phosphor->setStatusTip(tr("Disable fading of previous graphs"));
        else
            this->ui->actionDigital_phosphor->setStatusTip(tr("Enable fading of previous graphs"));
    });
    this->ui->actionDigital_phosphor->setChecked(mSettings->view.digitalPhosphor());

    connect(ui->actionAddMarker, &QAction::triggered, [this, view]() {
        Settings::MarkerAndZoom v;
        v.markerRect = QRectF(-1.0, -DIVS_VOLTAGE / 2, 2.0, DIVS_VOLTAGE);
        Settings::ZoomViewSettings &z = view->zoomviews;
        unsigned markerID = 0;
        if (z.size())
            // Use highest unique id and add 1.

            markerID = max_element(z.begin(), z.end(),
                                   [](Settings::ZoomViewSettings::value_type &a,
                                      Settings::ZoomViewSettings::value_type &b) { return a.first < b.first; })
                           ->first +
                       1;
        z.insert(markerID, v);
    });

    connect(ui->actionRemoveMarker, &QAction::triggered,
            [this, view]() { view->zoomviews.removeMarker(view->zoomviews.activeMarker()); });
    connect(&view->zoomviews, &Settings::ZoomViewSettings::activeMarkerChanged, this,
            [this](int activeMarker) { ui->actionRemoveMarker->setEnabled(activeMarker != -1); });
    connect(&view->zoomviews, &Settings::ZoomViewSettings::markerChanged, this,
            [this](int activeMarker) { ui->actionRemoveMarker->setEnabled(activeMarker != -1); });

    connect(ui->actionReport_an_issue, &QAction::triggered, [this]() {
        QMessageBox::about(
            this, tr("Report an issue - V%1").arg(VERSION),
            tr("<p>Please remember, this is a non-paid open source software.</p>"
               "<p>Help us by providing meaningful bug-reports. Don't forget to mention your operating system, version "
               "and as much details as possible<br>"
               "<a href='https://github.com/OpenHantek/openhantek'>https://github.com/OpenHantek/openhantek</a></p>"));

    });

    connect(ui->actionAbout, &QAction::triggered, [this]() {
        QMessageBox::about(
            this, tr("About OpenHantek %1").arg(VERSION),
            tr("<p>This is a open source software for Hantek USB oscilloscopes.</p>"
               "<p>Copyright &copy; 2010, 2011 Oliver Haag<br><a "
               "href='mailto:oliver.haag@gmail.com'>oliver.haag@gmail.com</a></p>"
               "<p>Copyright &copy; 2012-2017 OpenHantek community<br>"
               "<a href='https://github.com/OpenHantek/openhantek'>https://github.com/OpenHantek/openhantek</a></p>"));

    });
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::showNewData(std::shared_ptr<PPresult> data) { dsoWidget->showNew(data); }

void MainWindow::exporterStatusChanged(const QString &exporterName, const QString &status) {
    ui->statusbar->showMessage(tr("%1: %2").arg(exporterName).arg(status));
}

/// \brief Save the settings before exiting.
/// \param event The close event that should be handled.
void MainWindow::closeEvent(QCloseEvent *event) {
    if (mSettings->alwaysSave) {
        mSettings->mainWindowGeometry = saveGeometry();
        mSettings->mainWindowState = saveState();
        mSettings->save();
    }

    QMainWindow::closeEvent(event);
}
