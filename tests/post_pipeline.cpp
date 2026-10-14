// SPDX-License-Identifier: GPL-2.0+
// Exercise the production post-processing chain and the queued signal path
// with captured-data-shaped input, without USB or an OpenGL context.
#include <QCoreApplication>
#include <QEventLoop>
#include <QThread>
#include <QTimer>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

#include "hantekdso/dsosamples.h"
#include "post/graphgenerator.h"
#include "post/mathchannelgenerator.h"
#include "post/postprocessing.h"
#include "post/spectrumgenerator.h"
#include "scopesettings.h"

Q_DECLARE_METATYPE(DSOsamples *)

class FakeSampleSource : public QObject {
    Q_OBJECT
  public:
    void publish(const DSOsamples *frame) { emit samplesAvailable(frame); }
  signals:
    void samplesAvailable(const DSOsamples *frame);
};

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void testHardwareFrames(QCoreApplication &app) {
    DsoSettingsScope scope;
    scope.voltage.resize(3);
    scope.spectrum.resize(3);
    scope.voltage[0].used = true;
    scope.voltage[0].gainStepIndex = 7; // screenshot configuration:2V/div
    scope.horizontal.timebase = 1e-3;
    scope.horizontal.samplerate = 1e6;
    scope.horizontal.recordLength = 10240;
    DsoSettingsPostProcessing postSettings;
    MathChannelGenerator math(&scope, 2);
    SpectrumGenerator spectrum(&scope, &postSettings);
    GraphGenerator graphs(&scope, false); // 2250 hardware trigger
    PostProcessing post(3);
    post.registerProcessor(&math);
    post.registerProcessor(&spectrum);
    post.registerProcessor(&graphs);

    DSOsamples frame;
    frame.samplerate = 1e6;
    frame.data.resize(2);
    frame.data[0].resize(10240);
    frame.data[1].resize(10240, 0.0);
    const double pi = std::acos(-1.0);
    for (size_t i = 0; i < frame.data[0].size(); ++i) frame.data[0][i] = 0.2 * std::sin(2.0 * pi * i / 1000.0);

    // Match the application's registration and typed queued connections.
    qRegisterMetaType<DSOsamples *>();
    FakeSampleSource source;
    QThread worker;
    post.moveToThread(&worker);
    QObject::connect(&source, &FakeSampleSource::samplesAvailable, &post, &PostProcessing::input, Qt::QueuedConnection);
    std::shared_ptr<PPresult> received;
    QEventLoop loop;
    QObject::connect(
        &post, &PostProcessing::processingFinished, &app,
        [&](std::shared_ptr<PPresult> result) {
            received = result;
            loop.quit();
        },
        Qt::QueuedConnection);
    worker.start();
    try {
        for (auto mode :
             {Dso::TriggerMode::HARDWARE_SOFTWARE, Dso::TriggerMode::WAIT_FORCE, Dso::TriggerMode::SINGLE}) {
            scope.trigger.mode = mode;
            received.reset();
            QTimer timeout;
            timeout.setSingleShot(true);
            QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
            timeout.start(5000);
            source.publish(&frame);
            loop.exec();
            check(received != nullptr, "Captured frame did not cross the queued post-processing path");
            check(received->sampleCount() == 10240, "Post processing lost the short record");
            check(!received->softwareTriggerTriggered, "Hardware frame unexpectedly ran the software trigger");
            check(received->vaChannelVoltage.size() == 3 && received->vaChannelVoltage[0].size() == 10240,
                  "Hardware-trigger mode produced an empty voltage graph");
            check(received->vaChannelVoltage[1].empty() && received->vaChannelVoltage[2].empty(),
                  "Disabled channels unexpectedly produced graph vertices");
            const auto &vertices = received->vaChannelVoltage[0];
            check(std::abs(vertices.front().x() + 5.0f) < 1e-6 && std::abs(vertices[250].y() - 0.1f) < 1e-5,
                  "Time/gain scaling moved the captured trace outside the expected graph");
            check(std::abs(received->data(0)->frequency - 1000.0) < 20,
                  "The normal pipeline did not measure the synthetic1kHz calibration signal");
            check(graphs.isReady(), "Graph generation never became ready");
        }
    } catch (...) {
        worker.quit();
        worker.wait();
        throw;
    }
    worker.quit();
    worker.wait();
}
} // namespace

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        testHardwareFrames(app);
        std::cout << "Queued hardware frames generated10240vertices in normal,wait/force andsingle modes; no "
                     "software-trigger gate\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

#include "post_pipeline.moc"
