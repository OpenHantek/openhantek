// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QMutex>

#include "hantekprotocol/bulkStructs.h"
#include "hantekprotocol/controlStructs.h"

class DsoControl;
class USBDevice;
namespace Dso {
struct ModelSpec;
}

/**
 * Maintains a usb-bulk and usb-control command queue. To prevent hot-path runtime allocations,
 * you need to add the necessary commands first (use addCommand(...)), before using them with
 * getCommand() or modifyCommand().
 */
class DsoCommandQueue : public QObject {
  public:
    DsoCommandQueue(const Dso::ModelSpec *spec, USBDevice *device,DsoControl *control);
    ~DsoCommandQueue();

    /**
     * \brief Add a supported command. This is usually called from a model class within "models/..".
     *
     * If you do not add a command object and access the command via the command-code later on,
     * the application will crash!
     *
     * @param newCommand A command object
     * @param pending If true, the command will be send as soon as the dso sample-fetch loop starts.
     */
    void addCommand(BulkCommand *newCommand, bool pending = false);

    template <class T> T *modifyCommand(Hantek::BulkCode code) {
        T *t = static_cast<T *>(command[(uint8_t)code]);
        if (t) t->pending = true;
        return t;
    }

    inline const BulkCommand *getCommand(Hantek::BulkCode code) const { return command[(uint8_t)code]; }

    void addCommand(ControlCommand *newCommand, bool pending = false);

    template <class T> T *modifyCommand(Hantek::ControlCode code) {
        T *t = static_cast<T *>(control[(uint8_t)code]);
        if (t) t->pending = true;
        return t;
    }

    inline bool isCommandSupported(Hantek::ControlCode code) const { return control[(uint8_t)code]; }
    inline bool isCommandSupported(Hantek::BulkCode code) const { return command[(uint8_t)code]; }

    const ControlCommand *getCommand(Hantek::ControlCode code) const { return control[(uint8_t)code]; }

    /// Send all pending control and bulk commands. Issued by the run() loop.
    bool sendPendingCommands();

    /// \brief Send a bulk command to the oscilloscope.
    /// The hantek protocol requires to send a special control command first, this is handled by this method.
    ///
    /// \param command The command, that should be sent.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \return Number of sent bytes on success, libusb error code on error.
    int bulkCommand(const std::vector<unsigned char> *command, int attempts = HANTEK_ATTEMPTS) const;
  public slots:
    /// \brief Sends bulk/control commands directly.
    /// \param data The command bytes.
    /// \return See ::Dso::ErrorCode.
    void manualCommand(bool isBulk, Hantek::BulkCode bulkCode, Hantek::ControlCode controlCode, const QByteArray &data);

  protected:
    QMutex m_commandMutex; ///< Makes command/control set-methods and enumerations thread-safe
  private:
    /// Pointers to bulk/control commands
    BulkCommand *command[255] = {0};
    BulkCommand *firstBulkCommand = nullptr;
    ControlCommand *control[255] = {0};
    ControlCommand *firstControlCommand = nullptr;
    const bool m_useControlNoBulk;

    DsoControl *m_control;
    USBDevice *m_device; ///< The USB device for the oscilloscope
};
