// SPDX-License-Identifier: GPL-2.0+

#include <QApplication>
#include <QColor>
#include <QDebug>
#include <QSettings>

#include "settings.h"

#include "dsowidget.h"

/// \brief Set the number of channels.
/// \param channels The new channel count, that will be applied to lists.
DsoSettings::DsoSettings(const Dso::ControlSpecification* deviceSpecification) {
    // Add new channels to the list
    while (scope.spectrum.size() < deviceSpecification->channels) {
        // Spectrum
        DsoSettingsScopeSpectrum newSpectrum;
        newSpectrum.name = QApplication::tr("SP%1").arg(scope.spectrum.size()+1);
        scope.spectrum.push_back(newSpectrum);

        // Voltage
        DsoSettingsScopeVoltage newVoltage;
        newVoltage.name = QApplication::tr("CH%1").arg(scope.voltage.size()+1);
        scope.voltage.push_back(newVoltage);

        view.screen.voltage.push_back(QColor::fromHsv((int)(scope.spectrum.size()-1) * 60, 0xff, 0xff));
        view.screen.spectrum.push_back(view.screen.voltage.back().lighter());
        view.print.voltage.push_back(view.screen.voltage.back().darker(120));
        view.print.spectrum.push_back(view.screen.voltage.back().darker());
    }

    DsoSettingsScopeSpectrum newSpectrum;
    newSpectrum.name = QApplication::tr("SPM");
    scope.spectrum.push_back(newSpectrum);

    DsoSettingsScopeVoltage newVoltage;
    newVoltage.couplingOrMathIndex = (unsigned)Dso::MathMode::ADD_CH1_CH2;
    newVoltage.name = QApplication::tr("MATH");
    scope.voltage.push_back(newVoltage);

    view.screen.voltage.push_back(QColor(0x7f, 0x7f, 0x7f, 0xff));
    view.screen.spectrum.push_back(view.screen.voltage.back().lighter());
    view.print.voltage.push_back(view.screen.voltage.back());
    view.print.spectrum.push_back(view.print.voltage.back().darker());


    load();
}

bool DsoSettings::setFilename(const QString &filename) {
    std::unique_ptr<QSettings> local = std::unique_ptr<QSettings>(new QSettings(filename, QSettings::IniFormat));
    if (local->status() != QSettings::NoError) {
        qWarning() << "Could not change the settings file to " << filename;
        return false;
    }
    store.swap(local);
    return true;
}

void DsoSettings::load() {
    // General options
    store->beginGroup("options");
    if (store->contains("alwaysSave")) alwaysSave = store->value("alwaysSave").toBool();
    store->endGroup();

    store->beginGroup("exporting");
    if (store->contains("imageSize")) exporting.imageSize = store->value("imageSize").toSize();
    store->endGroup();

    // Oscilloscope settings
    store->beginGroup("scope");
    // Horizontal axis
    store->beginGroup("horizontal");
    if (store->contains("format")) scope.horizontal.format = (Dso::GraphFormat)store->value("format").toInt();
    if (store->contains("frequencybase"))
        scope.horizontal.frequencybase = store->value("frequencybase").toDouble();
    for (int marker = 0; marker < MARKER_COUNT; ++marker) {
        QString name;
        name = QString("marker%1").arg(marker);
        if (store->contains(name)) scope.setMarker(marker, store->value(name).toDouble());
    }
    if (store->contains("timebase")) scope.horizontal.timebase = store->value("timebase").toDouble();
    if (store->contains("recordLength")) scope.horizontal.recordLength = store->value("recordLength").toUInt();
    if (store->contains("samplerate")) scope.horizontal.samplerate = store->value("samplerate").toDouble();
    if (store->contains("samplerateSet")) scope.horizontal.samplerateSource = (DsoSettingsScopeHorizontal::SamplerateSource)store->value("samplerateSet").toInt();
    store->endGroup();
    // Trigger
    store->beginGroup("trigger");
    if (store->contains("mode")) scope.trigger.mode = (Dso::TriggerMode)store->value("mode").toUInt();
    if (store->contains("position")) scope.trigger.position = store->value("position").toDouble();
    if (store->contains("slope")) scope.trigger.slope = (Dso::Slope)store->value("slope").toUInt();
    if (store->contains("source")) scope.trigger.source = store->value("source").toUInt();
    if (store->contains("special")) scope.trigger.special = store->value("special").toBool();
    store->endGroup();
    // Spectrum
    for (ChannelID channel = 0; channel < scope.spectrum.size(); ++channel) {
        store->beginGroup(QString("spectrum%1").arg(channel));
        if (store->contains("magnitude"))
            scope.spectrum[channel].magnitude = store->value("magnitude").toDouble();
        if (store->contains("offset")) scope.spectrum[channel].offset = store->value("offset").toDouble();
        if (store->contains("used")) scope.spectrum[channel].used = store->value("used").toBool();
        store->beginGroup("cursor");
        if (store->contains("shape")) scope.spectrum[channel].cursor.shape =
                DsoSettingsScopeCursor::CursorShape(store->value("shape").toUInt());
        for (int marker = 0; marker < MARKER_COUNT; ++marker) {
            QString name;
            name = QString("x%1").arg(marker);
            if (store->contains(name)) scope.spectrum[channel].cursor.pos[marker].setX(store->value(name).toDouble());
            name = QString("y%1").arg(marker);
            if (store->contains(name)) scope.spectrum[channel].cursor.pos[marker].setY(store->value(name).toDouble());
        }
        store->endGroup();
        store->endGroup();
    }
    // Vertical axis
    for (ChannelID channel = 0; channel < scope.voltage.size(); ++channel) {
        store->beginGroup(QString("vertical%1").arg(channel));
        if (store->contains("gainStepIndex")) scope.voltage[channel].gainStepIndex = store->value("gainStepIndex").toUInt();
        if (store->contains("couplingOrMathIndex")) scope.voltage[channel].couplingOrMathIndex =
                store->value("couplingOrMathIndex").toUInt();
        if (store->contains("inverted")) scope.voltage[channel].inverted = store->value("inverted").toBool();
        if (store->contains("offset")) scope.voltage[channel].offset = store->value("offset").toDouble();
        if (store->contains("trigger")) scope.voltage[channel].trigger = store->value("trigger").toDouble();
        if (store->contains("used")) scope.voltage[channel].used = store->value("used").toBool();
        store->beginGroup("cursor");
        if (store->contains("shape")) scope.voltage[channel].cursor.shape =
                DsoSettingsScopeCursor::CursorShape(store->value("shape").toUInt());
        for (int marker = 0; marker < MARKER_COUNT; ++marker) {
            QString name;
            name = QString("x%1").arg(marker);
            if (store->contains(name)) scope.voltage[channel].cursor.pos[marker].setX(store->value(name).toDouble());
            name = QString("y%1").arg(marker);
            if (store->contains(name)) scope.voltage[channel].cursor.pos[marker].setY(store->value(name).toDouble());
        }
        store->endGroup();
        store->endGroup();
    }

    // Post processing
    if (store->contains("spectrumLimit")) post.spectrumLimit = store->value("spectrumLimit").toDouble();
    if (store->contains("spectrumReference"))
        post.spectrumReference = store->value("spectrumReference").toDouble();
    if (store->contains("spectrumWindow"))
        post.spectrumWindow = (Dso::WindowFunction)store->value("spectrumWindow").toInt();
    store->endGroup();

    // View
    store->beginGroup("view");
    // Colors
    store->beginGroup("color");
    DsoSettingsColorValues *colors;
    for (int mode = 0; mode < 2; ++mode) {
        if (mode == 0) {
            colors = &view.screen;
            store->beginGroup("screen");
        } else {
            colors = &view.print;
            store->beginGroup("print");
        }

        if (store->contains("axes")) colors->axes = store->value("axes").value<QColor>();
        if (store->contains("background")) colors->background = store->value("background").value<QColor>();
        if (store->contains("border")) colors->border = store->value("border").value<QColor>();
        if (store->contains("grid")) colors->grid = store->value("grid").value<QColor>();
        if (store->contains("markers")) colors->markers = store->value("markers").value<QColor>();
        for (ChannelID channel = 0; channel < scope.spectrum.size(); ++channel) {
            QString key = QString("spectrum%1").arg(channel);
            if (store->contains(key)) colors->spectrum[channel] = store->value(key).value<QColor>();
        }
        if (store->contains("text")) colors->text = store->value("text").value<QColor>();
        for (ChannelID channel = 0; channel < scope.voltage.size(); ++channel) {
            QString key = QString("voltage%1").arg(channel);
            if (store->contains(key)) colors->voltage[channel] = store->value(key).value<QColor>();
        }
        store->endGroup();
    }
    store->endGroup();
    // Other view settings
    if (store->contains("digitalPhosphor")) view.digitalPhosphor = store->value("digitalPhosphor").toBool();
    if (store->contains("interpolation"))
        view.interpolation = (Dso::InterpolationMode)store->value("interpolation").toInt();
    if (store->contains("screenColorImages")) view.screenColorImages = store->value("screenColorImages").toBool();
    if (store->contains("zoom")) view.zoom = store->value("zoom").toBool();
    if (store->contains("cursorGridPosition"))
        view.cursorGridPosition = (Qt::ToolBarArea)store->value("cursorGridPosition").toUInt();
    if (store->contains("cursorsVisible")) view.cursorsVisible = store->value("cursorsVisible").toBool();
    store->endGroup();

    store->beginGroup("window");
    mainWindowGeometry = store->value("geometry").toByteArray();
    mainWindowState = store->value("state").toByteArray();
    store->endGroup();
}

void DsoSettings::save() {
    // Main window layout and other general options
    store->beginGroup("options");
    store->setValue("alwaysSave", alwaysSave);
    store->endGroup();

    store->beginGroup("exporting");
    store->setValue("imageSize", exporting.imageSize);
    store->endGroup();

    // Oszilloskope settings
    store->beginGroup("scope");
    // Horizontal axis
    store->beginGroup("horizontal");
    store->setValue("format", scope.horizontal.format);
    store->setValue("frequencybase", scope.horizontal.frequencybase);
    for (int marker = 0; marker < MARKER_COUNT; ++marker)
        store->setValue(QString("marker%1").arg(marker), scope.getMarker(marker));
    store->setValue("timebase", scope.horizontal.timebase);
    store->setValue("recordLength", scope.horizontal.recordLength);
    store->setValue("samplerate", scope.horizontal.samplerate);
    store->setValue("samplerateSet", (int)scope.horizontal.samplerateSource);
    store->endGroup();
    // Trigger
    store->beginGroup("trigger");
    store->setValue("mode", (unsigned)scope.trigger.mode);
    store->setValue("position", scope.trigger.position);
    store->setValue("slope", (unsigned)scope.trigger.slope);
    store->setValue("source", scope.trigger.source);
    store->setValue("special", scope.trigger.special);
    store->endGroup();
    // Spectrum
    for (ChannelID channel = 0; channel < scope.spectrum.size(); ++channel) {
        store->beginGroup(QString("spectrum%1").arg(channel));
        store->setValue("magnitude", scope.spectrum[channel].magnitude);
        store->setValue("offset", scope.spectrum[channel].offset);
        store->setValue("used", scope.spectrum[channel].used);
        store->beginGroup("cursor");
        store->setValue("shape", scope.spectrum[channel].cursor.shape);
        for (int marker = 0; marker < MARKER_COUNT; ++marker) {
            QString name;
            name = QString("x%1").arg(marker);
            store->setValue(name, scope.spectrum[channel].cursor.pos[marker].x());
            name = QString("y%1").arg(marker);
            store->setValue(name, scope.spectrum[channel].cursor.pos[marker].y());
        }
        store->endGroup();
        store->endGroup();
    }
    // Vertical axis
    for (ChannelID channel = 0; channel < scope.voltage.size(); ++channel) {
        store->beginGroup(QString("vertical%1").arg(channel));
        store->setValue("gainStepIndex", scope.voltage[channel].gainStepIndex);
        store->setValue("couplingOrMathIndex", scope.voltage[channel].couplingOrMathIndex);
        store->setValue("inverted", scope.voltage[channel].inverted);
        store->setValue("offset", scope.voltage[channel].offset);
        store->setValue("trigger", scope.voltage[channel].trigger);
        store->setValue("used", scope.voltage[channel].used);
        store->beginGroup("cursor");
        store->setValue("shape", scope.voltage[channel].cursor.shape);
        for (int marker = 0; marker < MARKER_COUNT; ++marker) {
            QString name;
            name = QString("x%1").arg(marker);
            store->setValue(name, scope.voltage[channel].cursor.pos[marker].x());
            name = QString("y%1").arg(marker);
            store->setValue(name, scope.voltage[channel].cursor.pos[marker].y());
        }
        store->endGroup();
        store->endGroup();
    }

    // Post processing
    store->setValue("spectrumLimit", post.spectrumLimit);
    store->setValue("spectrumReference", post.spectrumReference);
    store->setValue("spectrumWindow", (int)post.spectrumWindow);
    store->endGroup();

    // View
    store->beginGroup("view");
    // Colors

    store->beginGroup("color");
    DsoSettingsColorValues *colors;
    for (int mode = 0; mode < 2; ++mode) {
        if (mode == 0) {
            colors = &view.screen;
            store->beginGroup("screen");
        } else {
            colors = &view.print;
            store->beginGroup("print");
        }

        store->setValue("axes", colors->axes.name(QColor::HexArgb));
        store->setValue("background", colors->background.name(QColor::HexArgb));
        store->setValue("border", colors->border.name(QColor::HexArgb));
        store->setValue("grid", colors->grid.name(QColor::HexArgb));
        store->setValue("markers", colors->markers.name(QColor::HexArgb));
        for (ChannelID channel = 0; channel < scope.spectrum.size(); ++channel)
            store->setValue(QString("spectrum%1").arg(channel), colors->spectrum[channel].name(QColor::HexArgb));
        store->setValue("text", colors->text.name(QColor::HexArgb));
        for (ChannelID channel = 0; channel < scope.voltage.size(); ++channel)
            store->setValue(QString("voltage%1").arg(channel), colors->voltage[channel].name(QColor::HexArgb));
        store->endGroup();
    }
    store->endGroup();

    // Other view settings
    store->setValue("digitalPhosphor", view.digitalPhosphor);
    store->setValue("interpolation", view.interpolation);
    store->setValue("screenColorImages", view.screenColorImages);
    store->setValue("zoom", view.zoom);
    store->setValue("cursorGridPosition", view.cursorGridPosition);
    store->setValue("cursorsVisible", view.cursorsVisible);
    store->endGroup();

    store->beginGroup("window");
    store->setValue("geometry", mainWindowGeometry);
    store->setValue("state", mainWindowState);
    store->endGroup();
}
