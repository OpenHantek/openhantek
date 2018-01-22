// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <list>
#include <string>

class DsoCommandQueue;
namespace Dso {
struct ModelSpec;
}

/**
 * @brief Describes a device
 * This is the central class to describe a hantek compatible DSO. It contains all usb information to find
 * the device on the bus, references to the firmware as well as the user visible name and device specification.
 */
class DSOModel {
  public:
    const int ID;
    const long vendorID;            ///< The USB vendor ID
    const long productID;           ///< The USB product ID
    const long vendorIDnoFirmware;  ///< The USB vendor ID if no firmware is flashed yet
    const long productIDnoFirmware; ///< The USB product ID if no firmware is flashed yet
    /// Firmwares are compiled into the executable with a filename pattern of devicename-firmware.hex and
    /// devicename-loader.hex.
    /// The firmwareToken is the "devicename" of the pattern above.
    std::string firmwareToken;
    std::string name; ///< User visible name. Does not need internationalisation/translation.
  protected:
    Dso::ModelSpec *specification;

  public:
    /// Add available commands to the command queue object
    virtual void applyRequirements(DsoCommandQueue *) const = 0;
    DSOModel(int id, long vendorID, long productID, long vendorIDnoFirmware, long productIDnoFirmware,
             const std::string &firmwareToken, const std::string &name, Dso::ModelSpec *specification);
    virtual ~DSOModel();
    /// Return the device specifications
    inline const Dso::ModelSpec *spec() const { return specification; }
};
