// SPDX-License-Identifier: GPL-2.0+
// Manual hardware test. Run only with a DSO-2250 and CH1 connected to CAL.
#include "hantekdso/hantekdsocontrol.h"
#include "hantekdso/models/modelDSO2250.h"
#include "usb/uploadFirmware.h"
#include "usb/usbdevice.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QReadLocker>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>

static std::unique_ptr<USBDevice> findScope(libusb_context *context, ModelDSO2250 &model) {
    libusb_device **list = nullptr;
    const ssize_t count = libusb_get_device_list(context, &list);
    if (count < 0) return nullptr;
    std::unique_ptr<USBDevice> scope;
    bool ambiguous = false;
    for (ssize_t i = 0; i < count; ++i) {
        libusb_device_descriptor descriptor;
        if (libusb_get_device_descriptor(list[i], &descriptor) != 0) continue;
        if (descriptor.idProduct != 0x2250 || (descriptor.idVendor != 0x04b4 && descriptor.idVendor != 0x04b5))
            continue;
        if (scope) {
            ambiguous = true;
            break;
        }
        scope.reset(new USBDevice(&model, list[i], 0));
    }
    libusb_free_device_list(list, 1);
    return ambiguous ? nullptr : std::move(scope);
}

static double frequency(const std::vector<double> &samples, double rate) {
    if (samples.size() < 16) return 0;
    auto sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    const double low = sorted[sorted.size() / 10], high = sorted[sorted.size() * 9 / 10];
    if (high - low < 0.02) return 0;
    bool up = false;
    std::vector<double> edges;
    for (size_t i = 0; i < samples.size(); ++i) {
        if (!up && samples[i] >= low + (high - low) * 0.7) {
            edges.push_back(i);
            up = true;
        } else if (up && samples[i] <= low + (high - low) * 0.3) up = false;
    }
    std::vector<double> periods;
    for (size_t i = 1; i < edges.size(); ++i) periods.push_back(edges[i] - edges[i - 1]);
    if (periods.size() < 2) return 0;
    std::sort(periods.begin(), periods.end());
    return rate / periods[periods.size() / 2];
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if (argc != 2) {
        std::cerr << "Usage: dso2250_hardware report.json\n";
        return 2;
    }
    libusb_context *context = nullptr;
    if (libusb_init(&context) != 0) return 2;
    ModelDSO2250 model;
    auto device = findScope(context, model);
    if (!device) {
        std::cerr << "Connect exactly one DSO-2250.\n";
        libusb_exit(context);
        return 2;
    }
    bool cold = device->needsFirmware();
    if (cold) {
        UploadFirmware upload;
        if (!upload.startUpload(device.get())) {
            std::cerr << upload.getErrorMessage().toStdString() << '\n';
            return 2;
        }
        device.reset();
        for (int i = 0; i < 50; ++i) {
            QThread::msleep(100);
            device = findScope(context, model);
            if (device && !device->needsFirmware()) break;
            device.reset();
        }
    }
    QString error;
    if (!device || !device->connectDevice(error)) {
        std::cerr << error.toStdString() << '\n';
        return 2;
    }
    int result = 1;
    {
        HantekDsoControl scope(device.get());
        QThread acquisitionThread;
        scope.setChannelUsed(0, true);
        scope.setChannelUsed(1, true);
        scope.setGain(0, 0.8);
        scope.setGain(1, 0.8);
        scope.setCoupling(0, Dso::Coupling::DC);
        scope.setCoupling(1, Dso::Coupling::DC);
        scope.setOffset(0, 0.5);
        scope.setOffset(1, 0.5);
        scope.setSamplerate(1e6);
        scope.setRecordLength(1);
        scope.setPretriggerPosition(0.00512);
        scope.setTriggerSource(false, 0);
        scope.setTriggerSlope(Dso::Slope::Positive);
        scope.setTriggerLevel(0, 0.1);
        scope.setTriggerMode(Dso::TriggerMode::WAIT_FORCE);
        QJsonArray captures;
        QObject::connect(
            &scope, &HantekDsoControl::samplesAvailable, &app, [&scope, &app, &captures](const DSOsamples *samples) {
                QReadLocker lock(&samples->lock);
                if (samples->data.size() != 2 || samples->data[0].size() != 10240 || samples->data[1].size() != 10240) {
                    app.quit();
                    return;
                }
                auto sorted = samples->data[0];
                std::sort(sorted.begin(), sorted.end());
                const double pp = (sorted[sorted.size() * 9 / 10] - sorted[sorted.size() / 10]) * 10;
                const double hz = frequency(samples->data[0], samples->samplerate);
                captures.append(QJsonObject{{"samples_per_channel", static_cast<int>(sorted.size())},
                                            {"frequency_Hz", hz},
                                            {"plateau_Vpp_probe10", pp},
                                            {"low_plateau_V_probe10", sorted[sorted.size() / 10] * 10},
                                            {"sample_rate", samples->samplerate}});
                if (captures.size() == 24) {
                    app.quit();
                }
            });
        QObject::connect(&scope, &HantekDsoControl::communicationError, &app, &QCoreApplication::quit);
        QTimer::singleShot(20000, &app, &QCoreApplication::quit);
        scope.enableSampling(true);
        scope.moveToThread(&acquisitionThread);
        QObject::connect(&acquisitionThread, &QThread::started, &scope, &HantekDsoControl::run);
        acquisitionThread.start();
        app.exec();
        acquisitionThread.quit();
        acquisitionThread.wait();
        result = captures.size() == 24 ? 0 : 1;
        for (const auto &capture : captures) {
            const auto frame = capture.toObject();
            if (std::abs(frame["frequency_Hz"].toDouble() - 1000) > 20 ||
                std::abs(frame["plateau_Vpp_probe10"].toDouble() - 2.0) > 0.4)
                result = 1;
        }
        QFile report(QString::fromLocal8Bit(argv[1]));
        if (!report.open(QIODevice::WriteOnly)) result = 2;
        else
            report.write(QJsonDocument(QJsonObject{{"cold_start", cold},
                                                   {"protocol", scope.getSpecification()->cmdSetSamplerate ==
                                                                        Hantek::BulkCode::SETTRIGGERANDSAMPLERATE
                                                                    ? "legacy"
                                                                    : "split"},
                                                   {"passed", result == 0},
                                                   {"captures", captures}})
                             .toJson());
        std::cout << "Captured " << captures.size() << " frames; result " << result << '\n';
    }
    device.reset();
    libusb_exit(context);
    return result;
}
