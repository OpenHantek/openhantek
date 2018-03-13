// SPDX-License-Identifier: GPL-2.0+

#include <QApplication>
#include <QColor>
#include <QDebug>
#include <QSettings>

#include "settings.h"
#include "viewconstants.h"

void Settings::ViewIO::readColor(QSettings *store, Settings::Colors *colors, const Settings::Scope *scope) {
    colors->_axes = store->value("axes", colors->_axes).value<QColor>();
    colors->_background = store->value("background", colors->_background).value<QColor>();
    colors->_border = store->value("border", colors->_border).value<QColor>();
    colors->_grid = store->value("grid", colors->_grid).value<QColor>();
    colors->_markers = store->value("markers", colors->_markers).value<QColor>();
    colors->_markerActive = store->value("markerActive", colors->_markerActive).value<QColor>();
    colors->_markerHover = store->value("markerHover", colors->_markerHover).value<QColor>();
    colors->_markerSelected = store->value("markerSelected", colors->_markerSelected).value<QColor>();
    colors->_zoomBackground = store->value("zoomBackground", colors->_zoomBackground).value<QColor>();
    colors->_zoomActive = store->value("zoomActive", colors->_zoomActive).value<QColor>();
    colors->_zoomHover = store->value("zoomHover", colors->_zoomHover).value<QColor>();
    colors->_zoomSelected = store->value("zoomSelected", colors->_zoomSelected).value<QColor>();
    colors->_text = store->value("text", colors->_text).value<QColor>();

    QString key;
    for (auto *c : *scope) {
        key = QString("spectrum%1").arg(c->channelID());
        colors->_spectrum[c->channelID()] = store->value(key, colors->spectrum(c->channelID())).value<QColor>();
        key = QString("voltage%1").arg(c->channelID());
        colors->_voltage[c->channelID()] = store->value(key, colors->voltage(c->channelID())).value<QColor>();
    }
}

void Settings::ViewIO::read(QSettings *store, View &view, const Settings::Scope *scope) {
    syncChannels(view, scope);
    QObject::connect(scope, &Settings::Scope::mathChannelAdded, &view, [&view, scope]() { syncChannels(view, scope); });
    // View
    store->beginGroup("view");
    // Colors
    store->beginGroup("color");

    store->beginGroup("screen");
    readColor(store, &view.screen, scope);
    store->endGroup();

    store->beginGroup("print");
    readColor(store, &view.print, scope);
    store->endGroup();

    store->endGroup(); // color

    unsigned markerSize = (unsigned)store->beginReadArray("markers");
    for (unsigned i = 0; i < markerSize; ++i) {
        store->setArrayIndex((int)i);
        MarkerAndZoom z;
        z.zoomRect = store->value("zoomview", z.zoomRect).toRectF();
        z.visible = store->value("visible", z.visible).toBool();
        z.markerRect = store->value("pos", z.markerRect).toRectF();
        view.zoomviews.insert(i, z);
    }
    store->endArray();

    // Other view settings
    view.m_digitalPhosphor = store->value("digitalPhosphor", view.m_digitalPhosphor).toBool();
    view.m_digitalPhosphorDepth =
        std::max((unsigned)2, store->value("digitalPhosphorDepth", view.m_digitalPhosphorDepth).toUInt());
    view.m_interpolation = (DsoE::InterpolationMode)store->value("interpolation", (int)view.m_interpolation).toInt();
    view.screenColorImages = store->value("screenColorImages", view.screenColorImages).toBool();
    store->endGroup();
}

void Settings::ViewIO::write(QSettings *store, const View &view) {
    // View
    store->beginGroup("view");
    // Colors

    store->beginGroup("color");
    const Colors *colors;
    for (int mode = 0; mode < 2; ++mode) {
        if (mode == 0) {
            colors = &view.screen;
            store->beginGroup("screen");
        } else {
            colors = &view.print;
            store->beginGroup("print");
        }

        store->setValue("axes", colors->axes());
        store->setValue("background", colors->background());
        store->setValue("border", colors->border());
        store->setValue("grid", colors->grid());

        store->setValue("markers", colors->markers());
        store->setValue("markerActive", colors->markerActive());
        store->setValue("markerHover", colors->markerHover());
        store->setValue("markerSelected", colors->markerSelected());

        store->setValue("zoomBackground", colors->zoomBackground());
        store->setValue("zoomActive", colors->zoomActive());
        store->setValue("zoomHover", colors->zoomHover());
        store->setValue("zoomSelected", colors->zoomSelected());

        store->setValue("text", colors->text());

        // As soon as we store channels and channel colors, we renumber the indices to [0,n]
        int newI = 0;
        for (auto &c : colors->_spectrum) store->setValue(QString("spectrum%1").arg(newI++), c.second);
        newI = 0;
        for (auto &c : colors->_voltage) store->setValue(QString("voltage%1").arg(newI++), c.second);
        store->endGroup();
    }
    store->endGroup();

    store->beginWriteArray("markers", (int)view.zoomviews.size());
    auto it = view.zoomviews.begin();
    for (unsigned i = 0; i < view.zoomviews.size(); ++i) {
        const MarkerAndZoom &v = it->second;
        store->setArrayIndex((int)i);
        store->setValue("zoomview", QVariant::fromValue(v.zoomRect));
        store->setValue("pos", QVariant::fromValue(v.markerRect));
        store->setValue("visible", v.visible);
        ++it;
    }
    store->endArray();

    // Other view settings
    store->setValue("digitalPhosphor", view.m_digitalPhosphor);
    store->setValue("interpolation", (unsigned)view.m_interpolation);
    store->setValue("screenColorImages", view.screenColorImages);
    store->endGroup();
}

void Settings::ViewIO::syncChannels(Settings::Colors &c, const Settings::Scope *scope) {
    // Create copy
    auto voltageCopy = c._voltage;
    auto spectrumCopy = c._spectrum;
    // Clear original
    c._voltage.clear();
    c._spectrum.clear();
    // Fill original with existing colors if available or a default color
    for (const Settings::Channel *channel : *scope) {
        const QColor voltageDefault = QColor::fromHsv((int)(c._voltage.size() - 1) * 60, 0xff, 0xff);
        c._voltage.insert(
            std::make_pair(channel->channelID(), GetWithDef(voltageCopy, channel->channelID(), voltageDefault)));
        c._spectrum.insert(std::make_pair(channel->channelID(),
                                          GetWithDef(spectrumCopy, channel->channelID(), voltageDefault.lighter())));
    }
}

void Settings::ViewIO::syncChannels(Settings::View &view, const Settings::Scope *scope) {
    syncChannels(view.screen, scope);
    syncChannels(view.print, scope);
}

void Settings::View::setInterpolation(DsoE::InterpolationMode mode) {
    m_interpolation = mode;
    emit interpolationChanged(this);
}

void Settings::View::setDigitalPhosphor(bool enable, unsigned historyDepth) {
    m_digitalPhosphor = enable;
    m_digitalPhosphorDepth = std::max((unsigned)2, historyDepth);
    emit digitalPhosphorChanged(this);
}
