// SPDX-License-Identifier: GPL-2.0+

#include "postprocessingsettings.h"

#include "settings/scopesettings.h"
#include <QCoreApplication>
#include <QString>

namespace PostProcessing {

QString mathModeString(MathMode mode, const ::Settings::Channel *first, const ::Settings::Channel *second) {
    switch (mode) {
    case MathMode::ADD:
        return QCoreApplication::tr("%1 + %2").arg(first->name(), second->name());
    case MathMode::SUBSTRACT:
        return QCoreApplication::tr("%1 - %2").arg(first->name(), second->name());
    case MathMode::MULTIPLY:
        return QCoreApplication::tr("%1 * %2").arg(first->name(), second->name());
    }
    return QString();
}

QString mathModeString(MathMode mode) {
    switch (mode) {
    case MathMode::ADD:
        return "+";
    case MathMode::SUBSTRACT:
        return "-";
    case MathMode::MULTIPLY:
        return "*";
    }
    return QString();
}

QString windowFunctionString(WindowFunction window) {
    switch (window) {
    case WindowFunction::RECTANGULAR:
        return QCoreApplication::tr("Rectangular");
    case WindowFunction::HAMMING:
        return QCoreApplication::tr("Hamming");
    case WindowFunction::HANN:
        return QCoreApplication::tr("Hann");
    case WindowFunction::COSINE:
        return QCoreApplication::tr("Cosine");
    case WindowFunction::LANCZOS:
        return QCoreApplication::tr("Lanczos");
    case WindowFunction::BARTLETT:
        return QCoreApplication::tr("Bartlett");
    case WindowFunction::TRIANGULAR:
        return QCoreApplication::tr("Triangular");
    case WindowFunction::GAUSS:
        return QCoreApplication::tr("Gauss");
    case WindowFunction::BARTLETTHANN:
        return QCoreApplication::tr("Bartlett-Hann");
    case WindowFunction::BLACKMAN:
        return QCoreApplication::tr("Blackman");
    // case WindowFunction::WINDOW_KAISER:
    //	return QCoreApplication::tr("Kaiser");
    case WindowFunction::NUTTALL:
        return QCoreApplication::tr("Nuttall");
    case WindowFunction::BLACKMANHARRIS:
        return QCoreApplication::tr("Blackman-Harris");
    case WindowFunction::BLACKMANNUTTALL:
        return QCoreApplication::tr("Blackman-Nuttall");
    case WindowFunction::FLATTOP:
        return QCoreApplication::tr("Flat top");
    }
    return QString();
}
} // end namespace PostProcessing
