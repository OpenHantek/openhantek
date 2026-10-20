// SPDX-License-Identifier: GPL-2.0+

#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <QTemporaryFile>
#include <libusb-1.0/libusb.h>
#include <memory>

#include "dso2250firmware.h"
#include "ezusb.h"
#include "uploadFirmware.h"
#include "usbdevice.h"

#include "dsomodel.h"

#define TR(str) QCoreApplication::translate("UploadFirmware", str)

bool UploadFirmware::startUpload(USBDevice *device) {
    if (device->isConnected() || !device->needsFirmware()) return false;

    // Write firmware from resources to temp files
    QFile firmwareRes(
        QString(":/firmware/%1-firmware.hex").arg(QString::fromStdString(device->getModel()->firmwareToken)));
    std::unique_ptr<QTemporaryFile> temp_firmware_path;
    if (device->getModel()->ID == 0x2250) {
        if (!firmwareRes.open(QIODevice::ReadOnly)) {
            errorMessage = TR("Couldn't read the device firmware: %1").arg(firmwareRes.errorString());
            return false;
        }
        QByteArray hex = firmwareRes.readAll();
        QString preparationError;
        if (!prepareDso2250Firmware(hex, preparationError)) {
            errorMessage = TR("Couldn't prepare DSO-2250 firmware: %1").arg(preparationError);
            return false;
        }
        temp_firmware_path.reset(new QTemporaryFile());
        if (!temp_firmware_path->open() || temp_firmware_path->write(hex) != hex.size() ||
            !temp_firmware_path->flush()) {
            errorMessage = TR("Couldn't write the temporary firmware: %1").arg(temp_firmware_path->errorString());
            return false;
        }
        temp_firmware_path->close();
    } else {
        temp_firmware_path.reset(QTemporaryFile::createNativeFile(firmwareRes));
        if (!temp_firmware_path) {
            errorMessage = TR("Couldn't prepare the device firmware.");
            return false;
        }
    }
    QFile loaderRes(QString(":/firmware/%1-loader.hex").arg(QString::fromStdString(device->getModel()->firmwareToken)));
    auto temp_loader_path = std::unique_ptr<QTemporaryFile>(QTemporaryFile::createNativeFile(loaderRes));
    if (!temp_loader_path) {
        errorMessage = TR("Couldn't prepare the firmware loader.");
        return false;
    }

    // Validate and prepare files before opening USB. Every later return closes
    // the handle, including failures while writing either RAM-upload stage.
    libusb_device_handle *handle = nullptr;
    int errorCode = libusb_open(device->getRawDevice(), &handle);
    if (errorCode != LIBUSB_SUCCESS) {
        errorMessage = TR("Couldn't open device: %1").arg(libUsbErrorString(errorCode));
        return false;
    }
    std::unique_ptr<libusb_device_handle, decltype(&libusb_close)> closeHandle(handle, &libusb_close);

    /* We need to claim the first interface */
    libusb_set_auto_detach_kernel_driver(handle, 1);
    int status = libusb_claim_interface(handle, 0);
    if (status != LIBUSB_SUCCESS) {
        errorMessage = TR("libusb_claim_interface() failed: %1").arg(libusb_error_name(status));
        return false;
    }

    // Write loader
    status = ezusb_load_ram(handle, temp_loader_path->fileName().toUtf8().constData(), FX_TYPE_FX2, 0);

    if (status != LIBUSB_SUCCESS) {
        errorMessage = TR("Writing the loader firmware failed: %1").arg(libusb_error_name(status));
        libusb_release_interface(handle, 0);
        return false;
    }

    // Write firmware
    status = ezusb_load_ram(handle, temp_firmware_path->fileName().toUtf8().constData(), FX_TYPE_FX2, 1);

    if (status != LIBUSB_SUCCESS) {
        errorMessage = TR("Writing the main firmware failed: %1").arg(libusb_error_name(status));
        libusb_release_interface(handle, 0);
        return false;
    }
    libusb_release_interface(handle, 0);

    return status == LIBUSB_SUCCESS;
}

const QString &UploadFirmware::getErrorMessage() const { return errorMessage; }

