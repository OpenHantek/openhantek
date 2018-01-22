// SPDX-License-Identifier: GPL-2.0+

#include <algorithm>
#include <cmath>
#include <memory>

#include <QCoreApplication>
#include <QEventLoop>
#include <QImage>
#include <QPainter>
#include <QTimer>

#include "scopeview/glscope.h"

#include "legacyexportdrawer.h"

#include "hantekdso/modelspecification.h"
#include "post/graphgenerator.h"
#include "post/ppresult.h"
#include "settings.h"
#include "utils/printutils.h"
#include "viewconstants.h"

#define tr(msg) QCoreApplication::translate("Exporter", msg)

namespace Exporter {
bool LegacyExportDrawer::exportSamples(std::shared_ptr<PPresult> result, QPaintDevice *paintDevice,
                                       const Dso::ModelSpec *deviceSpecification, const Settings::DsoSettings *settings,
                                       const Settings::Colors *colorValues) {
    // Create a painter for our device
    QPainter painter;
    painter.begin(paintDevice);

    // Draw grid and graphs with GlScope
    GlScope *scope =
        new GlScope(nullptr, &settings->view, colorValues, QSize(paintDevice->width() - 1, paintDevice->height() - 1));
    scope->initWithoutWindow();
    scope->showData(result);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    { // Discard first frame
        QPointer<Qt3DRender::QRenderCaptureReply> localCapture = scope->capture();
        QObject::connect(localCapture.data(), &Qt3DRender::QRenderCaptureReply::completed, &loop,
                         [&loop]() { loop.quit(); });
        timer.start(300);
        if (!localCapture->isComplete()) loop.exec();
    }
    {
        QPointer<Qt3DRender::QRenderCaptureReply> localCapture = scope->capture();
        QObject::connect(localCapture.data(), &Qt3DRender::QRenderCaptureReply::completed, &loop,
                         [&loop]() { loop.quit(); });
        timer.start(300);
        if (!localCapture->isComplete()) loop.exec();
        painter.drawImage(QPointF(1, 1), localCapture->image());
    }
    delete scope;

    // Get line height
    QFont font;
    QFontMetrics fontMetrics(font, paintDevice);
    double lineHeight = fontMetrics.height();

    painter.setBrush(Qt::SolidPattern);

    // Draw the settings table
    double stretchBase = (double)paintDevice->width() / 5;

    // Print trigger details
    ChannelID triggerSource = settings->deviceSettings->trigger.source();

    painter.setPen(colorValues->voltage(triggerSource));
    QString levelString =
        valueToString(result->data(triggerSource)->channelSettings->voltage()->triggerLevel(), Unit::VOLTS, 3);
    QString pretriggerString = tr("%L1%").arg((int)(settings->deviceSettings->trigger.position() * 100 + 0.5));

    // Print sample count
    painter.setPen(colorValues->text());

    double top = 5;

    painter.drawText(QRectF(0 * stretchBase, top, stretchBase, lineHeight),
                     tr("%1  %2  %3  %4")
                         .arg(settings->scope.channel(settings->deviceSettings->trigger.source())->name(),
                              Dso::slopeString(settings->deviceSettings->trigger.slope()), levelString,
                              pretriggerString));

    painter.drawText(QRectF(1 * stretchBase, top, stretchBase, lineHeight), tr("%1 S").arg(result->sampleCount()),
                     QTextOption(Qt::AlignRight));
    // Print samplerate
    painter.drawText(QRectF(2 * stretchBase, top, stretchBase, lineHeight),
                     valueToString(settings->deviceSettings->samplerate().samplerate, Unit::SAMPLES) + tr("/s"),
                     QTextOption(Qt::AlignRight));
    // Print timebase
    painter.drawText(QRectF(3 * stretchBase, top, stretchBase, lineHeight),
                     valueToString(settings->deviceSettings->samplerate().timebase, Unit::SECONDS, 0) + tr("/div"),
                     QTextOption(Qt::AlignRight));
    // Print frequencybase
    painter.drawText(QRectF(4 * stretchBase, top, stretchBase, lineHeight),
                     valueToString(settings->scope.frequencybase(), Unit::HERTZ, 0) + tr("/div"),
                     QTextOption(Qt::AlignRight));

    // Draw the measurement table
    stretchBase = (double)(paintDevice->width() - lineHeight * 6) / 10;
    unsigned channelCount = result->channelCount();
    for (const DataChannel &c : *result) {
        const Settings::Channel *channelSettings = c.channelSettings.get();
        ChannelID channel = channelSettings->channelID();
        if (!channelSettings->anyVisible() || !result->data(channel)) continue;
        double top = (double)paintDevice->height() - channelCount-- * lineHeight;

        // Print label
        painter.setPen(colorValues->voltage(channel));
        painter.drawText(QRectF(2, top, lineHeight * 4, lineHeight), channelSettings->name());
        // Print coupling/math mode
        if (!channelSettings->isMathChannel())
            painter.drawText(QRectF(lineHeight * 4, top, lineHeight * 2, lineHeight),
                             Dso::couplingString(channelSettings->voltage()->coupling(deviceSpecification)));
        else
            painter.drawText(QRectF(lineHeight * 4, top, lineHeight * 2, lineHeight),
                             PostProcessing::mathModeString(((Settings::MathChannel *)channelSettings)->mathMode()));

        // Print voltage gain
        painter.drawText(QRectF(lineHeight * 6, top, stretchBase * 2, lineHeight),
                         valueToString(channelSettings->gain(), Unit::VOLTS, 0) + tr("/div"),
                         QTextOption(Qt::AlignRight));
        // Print spectrum magnitude
        if (channelSettings->spectrum()->visible()) {
            painter.setPen(colorValues->spectrum(channel));
            painter.drawText(QRectF(lineHeight * 6 + stretchBase * 2, top, stretchBase * 2, lineHeight),
                             valueToString(channelSettings->spectrum()->magnitude(), Unit::DECIBEL, 0) + tr("/div"),
                             QTextOption(Qt::AlignRight));
        }

        // Amplitude string representation (4 significant digits)
        painter.setPen(colorValues->text());
        painter.drawText(QRectF(lineHeight * 6 + stretchBase * 4, top, stretchBase * 3, lineHeight),
                         valueToString(result->data(channel)->amplitude(), Unit::VOLTS, 4), QTextOption(Qt::AlignRight));
        // Frequency string representation (5 significant digits)
        painter.drawText(QRectF(lineHeight * 6 + stretchBase * 7, top, stretchBase * 3, lineHeight),
                         valueToString(result->data(channel)->frequency, Unit::HERTZ, 5), QTextOption(Qt::AlignRight));
    }

    painter.end();

    return true;
}
}
