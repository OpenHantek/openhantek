// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QMetaType>
#include <QString>
namespace Dso {
Q_NAMESPACE

/// \enum ChannelMode
/// \brief The channel display modes.
enum class ChannelMode {
    Voltage, ///< Standard voltage view
    Spectrum ///< Spectrum view
};
Q_ENUM_NS(ChannelMode)

/// \enum GraphFormat
/// \brief The possible viewing formats for the graphs on the scope.
enum class GraphFormat {
    TY, ///< The standard mode
    XY  ///< CH1 on X-axis, CH2 on Y-axis
};
Q_ENUM_NS(GraphFormat)

/// \enum Coupling
/// \brief The coupling modes for the channels.
enum class Coupling {
    AC, ///< Offset filtered out by condensator
    DC, ///< No filtering
    GND ///< Channel is grounded
};
Q_ENUM_NS(Coupling)

/// \enum TriggerMode
/// \brief The different triggering modes.
enum class TriggerMode {
    HARDWARE_SOFTWARE, ///< Normal hardware trigger (or software trigger) mode
    WAIT_FORCE,        ///< Automatic without trigger event
    SINGLE             ///< Stop after the first trigger event
};
Q_ENUM_NS(TriggerMode)

/// \enum Slope
/// \brief The slope that causes a trigger.
enum class Slope : uint8_t {
    Positive = 0, ///< From lower to higher voltage
    Negative = 1  ///< From higher to lower voltage
};
Q_ENUM_NS(Slope)

/// \enum InterpolationMode
/// \brief The different interpolation modes for the graphs.
enum class InterpolationMode {
    OFF = 0, ///< Just dots for each sample
    LINEAR,  ///< Sample dots connected by lines
    SINC,    ///< Smooth graph through the dots
};
Q_ENUM_NS(InterpolationMode)

QString channelModeString(ChannelMode mode);
QString graphFormatString(GraphFormat format);
QString couplingString(Coupling coupling);
QString triggerModeString(TriggerMode mode);
QString slopeString(Slope slope);
QString interpolationModeString(InterpolationMode interpolation);
}
