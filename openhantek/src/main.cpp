// SPDX-License-Identifier: GPL-2.0+

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QLibraryInfo>
#include <QLocale>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QTranslator>

#include <iostream>
#include <libusb-1.0/libusb.h>
#include <memory>

// Settings
#include "settings.h"
#include "viewconstants.h"

// DSO core logic
#include "dsocontrol.h"
#include "dsoloop.h"
#include "dsomodel.h"
#include "usb/usbdevice.h"

// Post processing
#include "post/graphgenerator.h"
#include "post/mathchannelgenerator.h"
#include "post/postprocessing.h"
#include "post/selfcalibration.h"
#include "post/spectrumgenerator.h"

// Exporter
#include "exporting/exportcsv.h"
#include "exporting/exporterregistry.h"
#include "exporting/exportimage.h"
#include "exporting/exportprint.h"

// GUI
#include "iconfont/QtAwesome.h"
#include "mainwindow.h"
#include "selectdevice/selectsupporteddevice.h"

#ifndef VERSION
#error "You need to run the cmake buildsystem!"
#endif

using namespace Hantek;

/// \brief Initialize resources and translations and show the main window.
int main(int argc, char *argv[]) {
    //////// Set application information ////////
    QCoreApplication::setOrganizationName("OpenHantek");
    QCoreApplication::setOrganizationDomain("www.openhantek.org");
    QCoreApplication::setApplicationName("OpenHantek");
    QCoreApplication::setApplicationVersion(VERSION);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, false);

    bool useGles = false;
    {
        QCoreApplication parserApp(argc, argv);
        QCommandLineParser p;
        p.addHelpOption();
        p.addVersionOption();
        QCommandLineOption useGlesOption("useGLES", QCoreApplication::tr("Use OpenGL ES instead of OpenGL"));
        p.addOption(useGlesOption);
        p.process(parserApp);
        useGles = p.isSet(useGlesOption);
    }
    // Prefer full desktop OpenGL without fixed pipeline
    QSurfaceFormat format;
    format.setSamples(4); // Antia-Aliasing, Multisampling
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setProfile(QSurfaceFormat::CoreProfile);
    if (useGles || QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGLES) {
        format.setVersion(2, 0);
        format.setRenderableType(QSurfaceFormat::OpenGLES);
        QCoreApplication::setAttribute(Qt::AA_UseOpenGLES, true);
    } else {
        format.setVersion(3, 2);
        format.setRenderableType(QSurfaceFormat::OpenGL);
        QCoreApplication::setAttribute(Qt::AA_UseOpenGLES, false);
    }
    QSurfaceFormat::setDefaultFormat(format);

    QApplication openHantekApplication(argc, argv);

    //////// Load translations ////////
    QTranslator qtTranslator;
    if (qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
        openHantekApplication.installTranslator(&qtTranslator);

    QTranslator openHantekTranslator;
    if (openHantekTranslator.load(QLocale(), QLatin1String("openhantek"), QLatin1String("_"),
                                  QLatin1String(":/translations"))) {
        openHantekApplication.installTranslator(&openHantekTranslator);
    }

    //////// Find matching usb devices ////////
    libusb_context *context = nullptr;
    int error = libusb_init(&context);
    if (error) {
        SelectSupportedDevice().showLibUSBFailedDialogModel(error);
        return -1;
    }
    std::unique_ptr<USBDevice> device = SelectSupportedDevice().showSelectDeviceModal(context);

    QString errorMessage;
    if (device == nullptr || !device->connectDevice(errorMessage)) {
        libusb_exit(context);
        return -1;
    }

    //////// Create settings object ////////
    Settings::DsoSettings settings(device->getModel()->spec());

    //////// Create DSO control object and move it to a separate thread ////////
    QThread dsoControlThread;
    dsoControlThread.setObjectName("dsoControlThread");
    DsoControl dsoControl(device.get(), settings.deviceSettings);
    settings.load(dsoControl.channelUsage());
    dsoControl.moveToThread(&dsoControlThread);
    QObject::connect(&dsoControlThread, &QThread::started, &dsoControl, &DsoControl::start);
    QObject::connect(&dsoControl, &DsoControl::communicationError, QCoreApplication::instance(),
                     &QCoreApplication::quit);
    QObject::connect(device.get(), &USBDevice::deviceDisconnected, QCoreApplication::instance(),
                     &QCoreApplication::quit);

    SelfCalibration selfCalibration(&dsoControl);

    //////// Create exporters ////////
    Exporter::Registry exportRegistry(device->getModel()->spec(), &settings);

    Exporter::CSV exporterCSV;
    Exporter::Image exportImage;
    Exporter::Print exportPrint;

    exportRegistry.registerExporter(&exporterCSV);
    exportRegistry.registerExporter(&exportImage);
    exportRegistry.registerExporter(&exportPrint);

    //////// Create post processing objects ////////
    QThread postProcessingThread;
    postProcessingThread.setObjectName("postProcessingThread");
    PostProcessing::Executor postProcessing(&settings.scope);

    PostProcessing::SpectrumGenerator spectrumGenerator(&settings.scope, &settings.post);
    PostProcessing::MathChannelGenerator mathchannelGenerator(&settings.scope);
    PostProcessing::GraphGenerator graphGenerator(&settings.scope, settings.deviceSettings.get(),
                                                  dsoControl.channelUsage());

    postProcessing.registerProcessor(&selfCalibration);
    postProcessing.registerProcessor(&mathchannelGenerator);
    postProcessing.registerProcessor(&spectrumGenerator);
    postProcessing.registerProcessor(&graphGenerator);

    postProcessing.moveToThread(&postProcessingThread);
    QObject::connect(&dsoControl, &DsoControl::samplesAvailable, &postProcessing, &PostProcessing::Executor::input);
    QObject::connect(&postProcessing, &PostProcessing::Executor::processingFinished, &exportRegistry,
                     &Exporter::Registry::input, Qt::DirectConnection);

    //////// Create main window ////////
    iconFont->initFontAwesome();
    MainWindow openHantekMainWindow(&dsoControl, &settings, &exportRegistry, &selfCalibration);
    QObject::connect(&postProcessing, &PostProcessing::Executor::processingFinished, &openHantekMainWindow,
                     &MainWindow::showNewData);
    QObject::connect(&exportRegistry, &Exporter::Registry::exporterStatusChanged, &openHantekMainWindow,
                     &MainWindow::exporterStatusChanged);
    openHantekMainWindow.show();

    //////// Start DSO thread and go into GUI main loop
    dsoControl.loopControl()->enableSampling(true);
    postProcessingThread.start();
    dsoControlThread.start();
    int res = openHantekApplication.exec();

    //////// Clean up ////////
    qWarning() << "Finishing dso control thread";
    dsoControlThread.quit();
    dsoControlThread.wait(2000);

    qWarning() << "Finishing post processing thread";
    postProcessingThread.quit();
    postProcessingThread.wait(2000);

    if (context && device != nullptr) {
        qWarning() << "Release usb device";
        device.reset();
        qWarning() << "Release usb library";
        libusb_exit(context);
    }

    return res;
}
