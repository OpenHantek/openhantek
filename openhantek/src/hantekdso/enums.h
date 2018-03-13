// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QMetaType>
#include <QString>

/// Because we get Q_NAMESPACE/Q_ENUM_NS only with Qt5.8+, we put enums into a gadget class
class DsoE {
    Q_GADGET
  public:
    /// \enum ChannelMode
    /// \brief The channel display modes.
    enum class ChannelMode {
        Voltage, ///< Standard voltage view
        Spectrum ///< Spectrum view
    };
    Q_ENUM(ChannelMode)

    /// \enum GraphFormat
    /// \brief The possible viewing formats for the graphs on the scope.
    enum class GraphFormat {
        TY, ///< The standard mode
        XY  ///< CH1 on X-axis, CH2 on Y-axis
    };
    Q_ENUM(GraphFormat)

    /// \enum Coupling
    /// \brief The coupling modes for the channels.
    enum class Coupling {
        AC, ///< Offset filtered out by condensator
        DC, ///< No filtering
        GND ///< Channel is grounded
    };
    Q_ENUM(Coupling)

    /// \enum TriggerMode
    /// \brief The different triggering modes.
    enum class TriggerMode {
        HARDWARE_SOFTWARE, ///< Normal hardware trigger (or software trigger) mode
        WAIT_FORCE,        ///< Automatic without trigger event
        SINGLE             ///< Stop after the first trigger event
    };
    Q_ENUM(TriggerMode)

    /// \enum Slope
    /// \brief The slope that causes a trigger.
    enum class Slope : uint8_t {
        Positive = 0, ///< From lower to higher voltage
        Negative = 1  ///< From higher to lower voltage
    };
    Q_ENUM(Slope)

    /// \enum InterpolationMode
    /// \brief The different interpolation modes for the graphs.
    enum class InterpolationMode {
        OFF = 0, ///< Just dots for each sample
        LINEAR,  ///< Sample dots connected by lines
        SINC,    ///< Smooth graph through the dots
    };
    Q_ENUM(InterpolationMode)

    static QString channelModeString(ChannelMode mode);
    static QString graphFormatString(GraphFormat format);
    static QString couplingString(Coupling coupling);
    static QString triggerModeString(TriggerMode mode);
    static QString slopeString(Slope slope);
    static QString interpolationModeString(InterpolationMode interpolation);
};
