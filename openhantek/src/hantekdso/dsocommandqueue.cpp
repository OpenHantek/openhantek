#include "dsocommandqueue.h"
#include "dsocontrol.h"
#include "usb/usbdevice.h"
#include "utils/debugnotify.h"
#include "utils/printutils.h"
#include <QDebug>

#ifdef DEBUG
#define DBGNOTIFY(x, y) emit m_control->debugMessage(x, y)
#else
#define DBGNOTIFY(x, y)
#endif

DsoCommandQueue::DsoCommandQueue(const Dso::ModelSpec *spec, USBDevice *device, DsoControl *control)
    : m_commandMutex(QMutex::Recursive), m_useControlNoBulk(spec->useControlNoBulk), m_control(control),
      m_device(device) {}

DsoCommandQueue::~DsoCommandQueue() {
    while (firstBulkCommand) {
        BulkCommand *t = firstBulkCommand->next;
        delete firstBulkCommand;
        firstBulkCommand = t;
    }
    while (firstControlCommand) {
        ControlCommand *t = firstControlCommand->next;
        delete firstControlCommand;
        firstControlCommand = t;
    }
}

void DsoCommandQueue::addCommand(BulkCommand *newCommand, bool pending) {
    newCommand->pending = pending;
    command[(uint8_t)newCommand->code] = newCommand;
    newCommand->next = firstBulkCommand;
    firstBulkCommand = newCommand;
}

void DsoCommandQueue::addCommand(ControlCommand *newCommand, bool pending) {
    newCommand->pending = pending;
    control[newCommand->code] = newCommand;
    newCommand->next = firstControlCommand;
    firstControlCommand = newCommand;
}

int DsoCommandQueue::bulkCommand(const std::vector<unsigned char> *command, int attempts) const {
    // Send BeginCommand control command
    int errorCode = m_device->controlWrite(&m_control->m_specification->beginCommandControl);
    if (errorCode < 0) return errorCode;

    // Send bulk command
    return m_device->bulkWrite(command->data(), command->size(), attempts);
}

bool DsoCommandQueue::sendPendingCommands() {
    int errorCode;
    QMutexLocker l(&m_commandMutex);

    // Send all pending control bulk commands
    BulkCommand *command = m_useControlNoBulk ? nullptr : firstBulkCommand;
    while (command) {
        if (command->pending) {
            DBGNOTIFY(QString("%1, %2")
                          .arg(QMetaEnum::fromType<HantekE::BulkCode>().valueToKey((int)command->code))
                          .arg(hexDump(command->data(), command->size())),
                      Debug::NotificationType::DeviceCommandSend);

            errorCode = bulkCommand(command);
            if (errorCode < 0) {
                qWarning() << "Sending bulk command failed: " << libUsbErrorString(errorCode);
                emit m_control->communicationError();
                return false;
            } else
                command->pending = false;
        }
        command = command->next;
    }

    // Send all pending control commands
    ControlCommand *controlCommand = firstControlCommand;
    while (controlCommand) {
        if (controlCommand->pending) {
            DBGNOTIFY(QString("%1, %2")
                          .arg(QMetaEnum::fromType<HantekE::ControlCode>().valueToKey((int)controlCommand->code))
                          .arg(hexDump(controlCommand->data(), controlCommand->size())),
                      Debug::NotificationType::DeviceCommandSend);

            errorCode = m_device->controlWrite(controlCommand);
            if (errorCode < 0) {
                qWarning("Sending control command %2x failed: %s", (uint8_t)controlCommand->code,
                         libUsbErrorString(errorCode).toLocal8Bit().data());

                if (errorCode == LIBUSB_ERROR_NO_DEVICE) {
                    emit m_control->communicationError();
                    return false;
                }
            } else
                controlCommand->pending = false;
        }
        controlCommand = controlCommand->next;
    }
    return true;
}

void DsoCommandQueue::manualCommand(bool isBulk, HantekE::BulkCode bulkCode, HantekE::ControlCode controlCode,
                                    const QByteArray &data) {
    if (!m_device->isConnected()) return;
    QMutexLocker l(&m_commandMutex);

    if (isBulk) {
        BulkCommand *c = modifyCommand<BulkCommand>(bulkCode);
        if (!c) return;
        memcpy(c->data(), data.data(), std::min((size_t)data.size(), c->size()));
    } else {
        ControlCommand *c = modifyCommand<ControlCommand>(controlCode);
        if (!c) return;
        memcpy(c->data(), data.data(), std::min((size_t)data.size(), c->size()));
    }
}
