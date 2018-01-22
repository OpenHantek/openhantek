// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QObject>
#include <QStringList>
#include <libusb-1.0/libusb.h>
#include <memory>

#include "usbdevicedefinitions.h"

class DSOModel;

typedef unsigned long UniqueUSBid;

/// \brief Returns string representation for libusb errors.
/// \param error The error code.
/// \return String explaining the error.
QString libUsbErrorString(int error);

/// \brief This class handles the USB communication with an usb device that has
/// one in and one out endpoint.
class USBDevice : public QObject {
    Q_OBJECT

  public:
    explicit USBDevice(DSOModel *model, libusb_device *device, unsigned findIteration = 0);
    USBDevice(const USBDevice &) = delete;
    ~USBDevice();
    bool connectDevice(QString &errorMessage);
    void disconnectFromDevice();

    /// \brief Check if the oscilloscope is connected.
    /// \return true, if a connection is up.
    bool isConnected();

    /**
     * @return Return true if this device needs a firmware first
     */
    bool needsFirmware();

    /**
     * Keep track of the find iteration on which this device was found
     * @param iteration The new iteration value
     */
    inline void setFindIteration(unsigned iteration) { findIteration = iteration; }
    inline unsigned getFindIteration() const { return findIteration; }

    /// \brief Bulk transfer to/from the oscilloscope.
    /// \param endpoint Endpoint number, also sets the direction of the transfer.
    /// \param data Buffer for the sent/recieved data.
    /// \param length The length of the packet.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \param timeout The timeout in ms.
    /// \return Number of transferred bytes on success, libusb error code on
    /// error.
    int bulkTransfer(unsigned char endpoint, const unsigned char *data, unsigned int length,
                     int attempts = HANTEK_ATTEMPTS, unsigned int timeout = HANTEK_TIMEOUT);

    /// \brief Bulk write to the oscilloscope.
    /// \param data Buffer for the sent/recieved data.
    /// \param length The length of the packet.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \return Number of sent bytes on success, libusb error code on error.
    inline int bulkWrite(const unsigned char *data, unsigned int length, int attempts = HANTEK_ATTEMPTS) {
        return bulkTransfer(HANTEK_EP_OUT, data, length, attempts);
    }

    /// \brief Bulk read from the oscilloscope.
    /// \param data Buffer for the sent/recieved data.
    /// \param length The length of the packet.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \return Number of received bytes on success, libusb error code on error.
    template <class T> inline int bulkRead(const T *command, int attempts = HANTEK_ATTEMPTS) {
        return bulkTransfer(HANTEK_EP_IN, command->data(), command->size(), attempts);
    }

    /// \brief Multi packet bulk read from the oscilloscope.
    /// \param data Buffer for the sent/recieved data.
    /// \param length The length of data contained in the packets.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \return Number of received bytes on success, libusb error code on error.
    int bulkReadMulti(unsigned char *data, unsigned length, int attempts = HANTEK_ATTEMPTS_MULTI);

    /// \brief Control write to the oscilloscope.
    /// \param command Buffer for the sent/recieved data.
    /// \return Number of sent bytes on success, libusb error code on error.
    template <class T> inline int controlWrite(const T *command, int attempts = HANTEK_ATTEMPTS) {
        if (!this->handle) return LIBUSB_ERROR_NO_DEVICE;

        int errorCode = LIBUSB_ERROR_TIMEOUT;
        for (int attempt = 0; (attempt < attempts || attempts == -1) && errorCode == LIBUSB_ERROR_TIMEOUT; ++attempt)
            errorCode =
                libusb_control_transfer(this->handle, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT,
                                        (uint8_t)command->code, (uint16_t)command->value, 0,
                                        (unsigned char *)command->data(), (uint16_t)command->size(), HANTEK_TIMEOUT);

        if (errorCode == LIBUSB_ERROR_NO_DEVICE) disconnectFromDevice();
        return errorCode;
    }

    /// \brief Control read to the oscilloscope.
    /// \param command Buffer for the sent/recieved data.
    /// \return Number of received bytes on success, libusb error code on error.
    template <class T> inline int controlRead(T *command, int attempts = HANTEK_ATTEMPTS) {
        if (!this->handle) return LIBUSB_ERROR_NO_DEVICE;

        int errorCode = LIBUSB_ERROR_TIMEOUT;
        for (int attempt = 0; (attempt < attempts || attempts == -1) && errorCode == LIBUSB_ERROR_TIMEOUT; ++attempt)
            errorCode =
                libusb_control_transfer(this->handle, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN,
                                        (uint8_t)command->code, (uint16_t)command->value, 0,
                                        (unsigned char *)command->data(), (uint16_t)command->size(), HANTEK_TIMEOUT);

        if (errorCode == LIBUSB_ERROR_NO_DEVICE) disconnectFromDevice();
        return errorCode;
    }

    /**
     * @return Returns the raw libusb device
     */
    inline libusb_device *getRawDevice() const { return device; }

    /**
     * @return Return the unique usb device id {@link USBDevice::computeUSBdeviceID()}.
     */
    inline unsigned long getUniqueUSBDeviceID() const { return uniqueUSBdeviceID; }
    /**
     * The USB bus is organised in a tree hierarchy. A device is connected to a port on a bus device,
     * which is connected to a port on another bus device etc up to the root usb device.
     *
     * The USB 3.0 standard allows up to 7 levels with 256 devices on each level (1 Byte). We generate
     * a unique number for the connected device.
     */
    static UniqueUSBid computeUSBdeviceID(libusb_device *device);

    /// \brief Get the oscilloscope model.
    /// \return The ::Model of the connected Hantek DSO.
    inline const DSOModel *getModel() const { return model; }
    /**
     * Usually a maximaum packet length for in and outgoing packets is determined
     * by the underlying implementation and usb specification. E.g. the roll buffer
     * mode uses the maximum in length for transfer. Some devices do not support
     * that much data though and need an artification restriction.
     */
    inline void overwriteInPacketLength(int len) { inPacketLength = len; }

  protected:
    int claimInterface(const libusb_interface_descriptor *interfaceDescriptor, int endpointOut, int endPointIn);

    // Device model data
    DSOModel *model;

    // Libusb specific variables
    struct libusb_device_descriptor descriptor;
    libusb_device *device; ///< The USB handle for the oscilloscope
    libusb_device_handle *handle = nullptr;
    unsigned findIteration;
    const unsigned long uniqueUSBdeviceID;
    int interface;
    int outPacketLength; ///< Packet length for the OUT endpoint
    int inPacketLength;  ///< Packet length for the IN endpoint
  signals:
    void deviceDisconnected(); ///< The device has been disconnected
};
