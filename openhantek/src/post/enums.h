// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QMetaType>

namespace Settings {
class Channel;
}

namespace PostProcessing {
Q_NAMESPACE

/// \enum MathMode
/// \brief The different math modes for the math-channel.
enum class MathMode { ADD, SUBSTRACT, MULTIPLY };
Q_ENUM_NS(MathMode)

/// \enum WindowFunction
/// \brief The supported window functions.
/// These are needed for spectrum analysis and are applied to the sample values
/// before calculating the DFT.
enum class WindowFunction {
    RECTANGULAR,  ///< Rectangular window (aka Dirichlet)
    HAMMING,      ///< Hamming window
    HANN,         ///< Hann window
    COSINE,       ///< Cosine window (aka Sine)
    LANCZOS,      ///< Lanczos window (aka Sinc)
    BARTLETT,     ///< Bartlett window (Endpoints == 0)
    TRIANGULAR,   ///< Triangular window (Endpoints != 0)
    GAUSS,        ///< Gauss window (simga = 0.4)
    BARTLETTHANN, ///< Bartlett-Hann window
    BLACKMAN,     ///< Blackman window (alpha = 0.16)
    // KAISER,                      ///< Kaiser window (alpha = 3.0)
    NUTTALL,         ///< Nuttall window, cont. first deriv.
    BLACKMANHARRIS,  ///< Blackman-Harris window
    BLACKMANNUTTALL, ///< Blackman-Nuttall window
    FLATTOP          ///< Flat top window
};
Q_ENUM_NS(WindowFunction)

/// \brief Return string representation of the given math mode.
/// \param mode The ::MathMode that should be returned as string.
/// \return The string that should be used in labels etc.
QString mathModeString(MathMode mode, const ::Settings::Channel *first, const ::Settings::Channel *second);

/// \brief Return string representation of the given math mode.
/// \param mode The ::MathMode that should be returned as string.
/// \return The string that should be used in labels etc.
QString mathModeString(MathMode mode);

/// \brief Return string representation of the given dft window function.
/// \param window The ::WindowFunction that should be returned as string.
/// \return The string that should be used in labels etc.
QString windowFunctionString(WindowFunction window);
}
