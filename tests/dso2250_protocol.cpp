// SPDX-License-Identifier: GPL-2.0+
// Golden packets recovered from the official SDK with REA, plus a saved
// capture-state pointer. These tests use the production controller and USB
// wrapper against fake libusb C functions; no physical USB library is linked.
#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QMap>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "hantekdso/hantekdsocontrol.h"
#include "hantekdso/models/modelDSO2090.h"
#include "hantekdso/models/modelDSO2250.h"
#include "scopesettings.h"
#include "usb/dso2250firmware.h"
#include "usb/usbdevice.h"

struct FakeUsb;
struct libusb_device {
    FakeUsb *fixture;
};
struct libusb_device_handle {
    FakeUsb *fixture;
};

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

QByteArray bytes(const std::vector<unsigned char> *packet) {
    check(packet != nullptr, "Missing protocol command");
    return QByteArray(reinterpret_cast<const char *>(packet->data()), static_cast<int>(packet->size()));
}

void golden(const std::vector<unsigned char> *packet, const char *hex, const char *name) {
    const QByteArray actual = bytes(packet);
    const QByteArray expected = QByteArray::fromHex(hex);
    if (actual != expected)
        throw std::runtime_error(std::string(name) + ": expected " + expected.toHex().constData() + ", got " +
                                 actual.toHex().constData());
}

void ok(Dso::ErrorCode error, const char *name) { check(error == Dso::ErrorCode::NONE, name); }
} // namespace

struct FakeUsb {
    libusb_device raw{this};
    libusb_device_handle handle{this};
    libusb_endpoint_descriptor endpoints[2]{};
    libusb_interface_descriptor alternate{};
    libusb_interface interfaceDescriptor{};
    libusb_config_descriptor configuration{};
    unsigned char revision = 0;
    uint16_t product = 0x2250;
    uint16_t vendor = 0x04b5;
    int queryError = 0;
    unsigned char lastCommand = 0;
    size_t dataPosition = 0;
    int opened = 0;
    int claimed = 0;
    int references = 1; // The USB backend's device-list reference.
    int wrapperRefs = 0;
    int wrapperUnrefs = 0;
    int closes = 0;
    int configurationError = 0;
    int claimError = 0;
    int controlError = 0;
    int bulkError = 0;
    std::vector<QByteArray> bulkOut;
    std::vector<QByteArray> beginCommands;
    QByteArray calibration =
        QByteArray::fromHex("006b00ee006100e4005e00e1006b00ee006000e4005e00e1006b00ee006000e4005e00e1"
                            "005f00e5005700dc005500da005f00e4005700dc005500da005f00e4005700dc005500da");
    std::vector<unsigned char> frame = std::vector<unsigned char>(20480);

    explicit FakeUsb(unsigned char rev = 0, uint16_t productId = 0x2250, uint16_t vendorId = 0x04b5)
        : revision(rev), product(productId), vendor(vendorId) {
        endpoints[0].bEndpointAddress = 0x02;
        endpoints[1].bEndpointAddress = 0x86;
        for (auto &endpoint : endpoints) {
            endpoint.bmAttributes = LIBUSB_TRANSFER_TYPE_BULK;
            endpoint.wMaxPacketSize = 512;
        }
        alternate.bInterfaceClass = LIBUSB_CLASS_VENDOR_SPEC;
        alternate.bNumEndpoints = 2;
        alternate.endpoint = endpoints;
        interfaceDescriptor.altsetting = &alternate;
        interfaceDescriptor.num_altsetting = 1;
        configuration.bNumInterfaces = 1;
        configuration.bConfigurationValue = 1;
        configuration.interface = &interfaceDescriptor;
        for (size_t i = 0; i < 10240; ++i) {
            frame[2 * i] = 128;
            frame[2 * i + 1] = static_cast<unsigned char>(i);
        }
    }
};

extern "C" {
libusb_device *LIBUSB_CALL libusb_ref_device(libusb_device *dev) {
    ++dev->fixture->references;
    ++dev->fixture->wrapperRefs;
    return dev;
}
void LIBUSB_CALL libusb_unref_device(libusb_device *dev) {
    --dev->fixture->references;
    ++dev->fixture->wrapperUnrefs;
}
int LIBUSB_CALL libusb_get_port_numbers(libusb_device *, uint8_t *ports, int length) {
    if (length < 1) return LIBUSB_ERROR_OVERFLOW;
    ports[0] = 1;
    return 1;
}
int LIBUSB_CALL libusb_get_device_descriptor(libusb_device *dev, libusb_device_descriptor *descriptor) {
    std::memset(descriptor, 0, sizeof(*descriptor));
    descriptor->idVendor = dev->fixture->vendor;
    descriptor->idProduct = dev->fixture->product;
    return LIBUSB_SUCCESS;
}
uint8_t LIBUSB_CALL libusb_get_bus_number(libusb_device *) { return 1; }
uint8_t LIBUSB_CALL libusb_get_device_address(libusb_device *) { return 1; }
int LIBUSB_CALL libusb_open(libusb_device *dev, libusb_device_handle **handle) {
    *handle = &dev->fixture->handle;
    ++dev->fixture->opened;
    ++dev->fixture->references; // Each open handle retains the device.
    return LIBUSB_SUCCESS;
}
void LIBUSB_CALL libusb_close(libusb_device_handle *handle) {
    --handle->fixture->opened;
    --handle->fixture->references;
    ++handle->fixture->closes;
}
int LIBUSB_CALL libusb_get_config_descriptor(libusb_device *dev, uint8_t, libusb_config_descriptor **config) {
    if (dev->fixture->configurationError) return dev->fixture->configurationError;
    *config = &dev->fixture->configuration;
    return LIBUSB_SUCCESS;
}
void LIBUSB_CALL libusb_free_config_descriptor(libusb_config_descriptor *) {}
int LIBUSB_CALL libusb_claim_interface(libusb_device_handle *handle, int) {
    if (handle->fixture->claimError) return handle->fixture->claimError;
    ++handle->fixture->claimed;
    return LIBUSB_SUCCESS;
}
int LIBUSB_CALL libusb_release_interface(libusb_device_handle *handle, int) {
    --handle->fixture->claimed;
    return LIBUSB_SUCCESS;
}
int LIBUSB_CALL libusb_get_configuration(libusb_device_handle *, int *value) {
    *value = 1;
    return LIBUSB_SUCCESS;
}
int LIBUSB_CALL libusb_set_configuration(libusb_device_handle *, int) { return LIBUSB_SUCCESS; }
int LIBUSB_CALL libusb_set_interface_alt_setting(libusb_device_handle *, int, int) { return LIBUSB_SUCCESS; }

int LIBUSB_CALL libusb_control_transfer(libusb_device_handle *handle, uint8_t type, uint8_t request, uint16_t value,
                                        uint16_t index, unsigned char *data, uint16_t length, unsigned int) {
    auto &fixture = *handle->fixture;
    if (fixture.controlError) return fixture.controlError;
    if ((type & LIBUSB_ENDPOINT_IN) != 0) {
        std::memset(data, 0, length);
        if (request == 0xa2 && value == 8) {
            check(index == 0 && length == 72, "A2 calibration request differs from SDK fixture");
            std::memcpy(data, fixture.calibration.constData(), length);
        } else if (request == 0xb2 && length) {
            data[0] = 1;
        } else return LIBUSB_ERROR_PIPE;
    } else if (request == 0xb3) {
        fixture.beginCommands.emplace_back(reinterpret_cast<char *>(data), length);
    }
    return length;
}

int LIBUSB_CALL libusb_bulk_transfer(libusb_device_handle *handle, unsigned char endpoint, unsigned char *data,
                                     int length, int *transferred, unsigned int) {
    auto &fixture = *handle->fixture;
    *transferred = 0;
    if (fixture.bulkError) return fixture.bulkError;
    if (endpoint == 0x02) {
        check(length >= 2, "Incomplete bulk command");
        fixture.lastCommand = data[0];
        fixture.dataPosition = 0;
        fixture.bulkOut.emplace_back(reinterpret_cast<char *>(data), length);
        *transferred = length;
        return LIBUSB_SUCCESS;
    }
    if (endpoint != 0x86) return LIBUSB_ERROR_PIPE;
    if (fixture.lastCommand == 9) {
        if (fixture.queryError) return fixture.queryError;
        std::memset(data, 0, length);
        if (length > 1) data[1] = fixture.revision;
    } else if (fixture.lastCommand == 6) {
        check(length == 512, "Capture-state read changed from the verified 512-byte transfer");
        std::memset(data, 0, length);
        const unsigned char state[] = {2, 0, 0xb9, 0x17}; // captured gray0x17b9 ->RAM6865
        std::memcpy(data, state, sizeof(state));
    } else if (fixture.lastCommand == 5) {
        if (fixture.dataPosition + static_cast<size_t>(length) > fixture.frame.size()) return LIBUSB_ERROR_OVERFLOW;
        std::memcpy(data, fixture.frame.data() + fixture.dataPosition, length);
        fixture.dataPosition += length;
    } else return LIBUSB_ERROR_PIPE;
    *transferred = length;
    return LIBUSB_SUCCESS;
}
}

namespace {
struct Rig {
    FakeUsb fake;
    USBDevice usb;
    std::unique_ptr<HantekDsoControl> control;
    Rig(DSOModel &model, unsigned char revision = 0, int queryError = 0)
        : fake(revision, model.productID, model.vendorID), usb(&model, &fake.raw) {
        fake.queryError = queryError;
        QString error;
        check(usb.connectDevice(error), "Fake device did not connect");
        control.reset(new HantekDsoControl(&usb));
    }
};

void configure(HantekDsoControl &control, bool ch1 = true, bool ch2 = true) {
    ok(control.setChannelUsed(0, ch1), "Set CH1");
    ok(control.setChannelUsed(1, ch2), "Set CH2");
    ok(control.setGain(0, 0.8), "Set CH1 100mV/div");
    ok(control.setGain(1, 0.8), "Set CH2 100mV/div");
    ok(control.setOffset(0, 0.5), "Center CH1");
    ok(control.setOffset(1, 0.5), "Center CH2");
    ok(control.setCoupling(0, Dso::Coupling::DC), "DC CH1");
    ok(control.setCoupling(1, Dso::Coupling::DC), "DC CH2");
    ok(control.setTriggerSource(false, ch1 ? 0 : 1), "Set internal trigger");
    ok(control.setTriggerSlope(Dso::Slope::Positive), "Set rising trigger");
    ok(control.setSamplerate(1e6), "Set 1MS/s");
    ok(control.setRecordLength(1), "Set short record");
    ok(control.setPretriggerPosition(0.00512), "Set 50 percent pretrigger");
}

void usbLifetime() {
    ModelDSO2250 model;
    FakeUsb fixture;
    int disconnected = 0;
    {
        USBDevice usb(&model, &fixture.raw);
        QObject::connect(&usb, &USBDevice::deviceDisconnected, [&disconnected]() { ++disconnected; });
        QString error;
        check(fixture.references == 2 && fixture.wrapperRefs == 1, "Wrapper did not retain the raw USB device");
        // The GUI selection model opens and closes the device to check access,
        // then main opens this same wrapper again for acquisition.
        for (int probe = 0; probe < 3; ++probe) {
            check(usb.connectDevice(error), "Selection-probe connection failed");
            check(fixture.references == 3 && fixture.opened == 1 && fixture.claimed == 1,
                  "Open handle/device references are inconsistent");
            usb.disconnectFromDevice();
            usb.disconnectFromDevice();
            check(fixture.references == 2 && fixture.wrapperUnrefs == 0 && fixture.opened == 0 &&
                      fixture.claimed == 0 && disconnected == probe + 1,
                  "Selection-probe disconnect released the wrapper reference or disconnected twice");
        }
        check(usb.getRawDevice() == &fixture.raw && usb.connectDevice(error),
              "Device could not reconnect after probing");
        unsigned char data = 0;
        fixture.controlError = LIBUSB_ERROR_NO_DEVICE;
        check(usb.controlTransfer(0x40, 0xb3, &data, 1, 0, 0, 1) == LIBUSB_ERROR_NO_DEVICE,
              "Unplug error was not returned");
        check(!usb.isConnected() && fixture.references == 2 && disconnected == 4,
              "Unplug handling invalidated the wrapper reference");
        usb.disconnectFromDevice();
    }
    check(fixture.references == 1 && fixture.wrapperUnrefs == 1 && fixture.closes == 4 && disconnected == 4,
          "Destruction did not release exactly one raw-device reference");

    FakeUsb active;
    {
        USBDevice usb(&model, &active.raw);
        QString error;
        check(usb.connectDevice(error), "Active device did not open");
    }
    check(active.references == 1 && active.opened == 0 && active.claimed == 0 && active.closes == 1 &&
              active.wrapperUnrefs == 1,
          "Closing an active device leaked or released it twice");

    for (bool failConfiguration : {false, true}) {
        FakeUsb failed;
        {
            USBDevice usb(&model, &failed.raw);
            if (failConfiguration) failed.configurationError = LIBUSB_ERROR_NO_DEVICE;
            else failed.claimError = LIBUSB_ERROR_BUSY;
            QString error;
            check(!usb.connectDevice(error) && !error.isEmpty(), "Failed connection was accepted or lost its error");
            check(!usb.isConnected() && failed.references == 2 && failed.opened == 0 && failed.claimed == 0,
                  "Failed configuration/claim retained an open handle");
        }
        check(failed.references == 1 && failed.wrapperUnrefs == 1 && failed.closes == 1,
              "Failed connection destroyed the raw-device reference twice");
    }
}

void controllerGoldens() {
    ModelDSO2250 sharedModel;
    const auto originalSamplerateCommand = sharedModel.spec()->cmdSetSamplerate;
    Rig old(sharedModel);
    auto &control = *old.control;
    check(control.getSpecification() != sharedModel.spec(), "Model specification was not copied per device");
    check(control.getDeviceSettings()->samplerate.limits == &control.getSpecification()->samplerate.single,
          "Samplerate limits point outside the per-device copy");
    check(control.getSpecification()->cmdSetSamplerate == Hantek::BulkCode::SETTRIGGERANDSAMPLERATE,
          "Revision0 did not select legacy opcode01");
    check((bytes(control.getCommand(Hantek::BulkCode::SETTRIGGERANDSAMPLERATE))[2] & 0x1c) == 0x04,
          "Fresh legacy controller still sends hardware roll mode despite default short record");
    DsoSettingsScopeHorizontal horizontal;
    horizontal.normalizeRecordLength(control.getAvailableRecordLengths());
    check(horizontal.recordLength == 10240, "Initial invalid0 sample count did not select a finite short record");
    horizontal.recordLength = UINT_MAX;
    horizontal.normalizeRecordLength(control.getAvailableRecordLengths());
    check(horizontal.recordLength == UINT_MAX, "Valid saved roll setting was not preserved");
    horizontal.recordLength = 12345;
    horizontal.normalizeRecordLength(control.getAvailableRecordLengths());
    check(horizontal.recordLength == 10240, "Invalid saved sample count did not restore the short record");
    check(sharedModel.spec()->cmdSetSamplerate == originalSamplerateCommand &&
              sharedModel.spec()->samplerate.single.max == 100e6,
          "Legacy detection mutated the shared model");
    check(old.fake.bulkOut.front() == QByteArray::fromHex("0900"), "First command was not the SDK revision query");
    check(old.fake.beginCommands.front() == QByteArray::fromHex("0f030303000000000000"),
          "B3 handshake differs from SDK");
    configure(control);
    golden(control.getCommand(Hantek::BulkCode::SETTRIGGERANDSAMPLERATE), "0100c6069dffffeb00000700",
           "SDK legacy1MS/s2CH50%");
    golden(control.getCommand(Hantek::BulkCode::SETGAIN), "070f300000000000", "SDK gain0 on both ADCs");
    golden(control.getCommand(Hantek::ControlCode::CONTROL_SETRELAYS), "00fb08fddf40ef01000000000000000000",
           "SDK100mV relay bank");
    const auto offset = bytes(control.getCommand(Hantek::ControlCode::CONTROL_SETOFFSET));
    check(offset.mid(0, 4) == QByteArray::fromHex("20ad30a2") && (static_cast<unsigned char>(offset[4]) & 0xf0) == 0x20,
          "SDK B4 channel or trigger bank marker missing");
    ok(control.setPretriggerPosition(0.01024), "Set 100 percent");
    golden(control.getCommand(Hantek::BulkCode::SETTRIGGERANDSAMPLERATE), "0100c6069dfffeff00000700",
           "SDK100%position special case");
    ok(control.setGain(0, 8.0), "Set CH1 1V/div");
    ok(control.setGain(1, 8.0), "Set CH2 1V/div");
    golden(control.getCommand(Hantek::ControlCode::CONTROL_SETRELAYS), "000408fd2040ef01000000000000000000",
           "SDK1V relay bank");

    for (const auto &rate : std::vector<std::pair<double, const char *>>{{100e6, "010086060000ffeb00000700"},
                                                                         {50e6, "0100c606ffffffeb00000700"},
                                                                         {25e6, "0100c606fdffffeb00000700"},
                                                                         {125e6, "0100a6060000ffeb00000700"}}) {
        ok(control.setSamplerate(rate.first), "Set SDK high/slow rate");
        ok(control.setPretriggerPosition(5120.0 / rate.first), "Keep 50 percent at new rate");
        golden(control.getCommand(Hantek::BulkCode::SETTRIGGERANDSAMPLERATE), rate.second, "SDK rate golden");
    }
    configure(control, false, true);
    golden(control.getCommand(Hantek::BulkCode::SETTRIGGERANDSAMPLERATE), "0100c7079dffffeb00000700",
           "SDK CH2-only mapping3");
    configure(control, true, false);
    ok(control.setSamplerate(250e6), "Set 250MS/s on one channel");
    ok(control.setPretriggerPosition(10240.0 / 250e6), "Keep 50 percent at 250MS/s");
    golden(control.getCommand(Hantek::BulkCode::SETTRIGGERANDSAMPLERATE), "0100a6000000ffeb00000700",
           "SDK250MS/sCH1pair address");

    Rig modern(sharedModel, 1);
    configure(*modern.control);
    check(modern.control->getSpecification()->cmdSetSamplerate == Hantek::BulkCode::ESETTRIGGERORSAMPLERATE,
          "Revision1 lost the split protocol");
    check((bytes(modern.control->getCommand(Hantek::BulkCode::DSETBUFFER))[2] & 7) == 1,
          "Fresh revision1 controller did not synchronize the short-record packet");
    check(control.getSpecification()->cmdSetSamplerate == Hantek::BulkCode::SETTRIGGERANDSAMPLERATE,
          "Second device changed the first device protocol");
    Rig unavailable(sharedModel, 0, LIBUSB_ERROR_TIMEOUT);
    check(unavailable.control->getSpecification()->cmdSetSamplerate == Hantek::BulkCode::SETTRIGGERANDSAMPLERATE,
          "Unavailable revision query did not select legacy fallback");

    configure(control);
    ok(control.setRecordTime(0.01024), "Set duration corresponding to1MS/s");
    check(std::abs(control.getDeviceSettings()->samplerate.current - 1e6) < 1,
          "Duration path still selected maximum rate instead of calculating its divisor");
    golden(control.getCommand(Hantek::BulkCode::SETTRIGGERANDSAMPLERATE), "0100c6069dffffeb00000700",
           "SDK duration path1MS/s");
    ok(control.setRecordTime(0.0001), "Set100us duration on2CH");
    check(std::abs(control.getDeviceSettings()->samplerate.current - 100e6) < 1,
          "100us two-channel duration selected125MS/s above the required102.4MS/s ceiling");
    configure(control, true, false);
    ok(control.setRecordTime(0.0001024), "Set102.4us duration on1CH");
    check(control.getDeviceSettings()->samplerate.limits == &control.getSpecification()->samplerate.single &&
              std::abs(control.getDeviceSettings()->samplerate.current - 100e6) < 1,
          "102.4us one-channel duration incorrectly enabled host fast-rate RAM layout");
    ok(control.setPretriggerPosition(0.0000512), "Set50percent at100MS/s");
    golden(control.getCommand(Hantek::BulkCode::SETTRIGGERANDSAMPLERATE), "010086040000ffeb00000700",
           "SDK100MS/sone-channel normalADC layout");
    configure(control);
    control.enableSampling(true);
    control.run(); // ready state2 before starting: discard old frame and arm
    control.run(); // ready state2 after starting: publish ordered frame
    const auto &samples = control.getLastSamples();
    check(samples.data.size() == 2 && samples.data[0].size() == 10240 && samples.data[1].size() == 10240,
          "State2 did not yield the complete short record");
    // Captured pointer17b9 decodes6865. The SDK starts at pair6866; ramp ADC
    // fixture therefore starts at210, not209. Calibration fixture center is
    // CH1=(173-107)/(238-107); range is0.8V/screen for100mV/div.
    check(std::abs(samples.data[0][0] - 0.255770094) < 1e-6, "SDK Gray pivot+1 was not applied");
    ok(control.setRecordLength(0), "Enable roll after normal capture");
    for (int i = 0; i < 4; ++i) control.run();
    const auto &roll = control.getLastSamples();
    check(roll.append && roll.data[0].size() == 256, "Roll did not produce a512-byte ADC packet");
    check(std::abs(roll.data[0][0] + 0.403053435) < 1e-6,
          "Roll incorrectly inherited the previous normal-capture Gray pointer");
}

void unaffected2090Golden() {
    ModelDSO2090 model;
    Rig rig(model);
    configure(*rig.control);
    golden(rig.control->getCommand(Hantek::BulkCode::SETTRIGGERANDSAMPLERATE), "01008502e8ffffeb00000700",
           "DSO2090 protocol regression");
    check(rig.fake.bulkOut.empty(), "A non2250 model received a2250 revision query");
}

void savedRecordLengths() {
    struct SavedRecord {
        unsigned count;
        bool fast;
        unsigned expected;
    };
    ModelDSO2250 model;
    for (const auto &saved : std::vector<SavedRecord>{{524288, false, 524288},
                                                      {1048576, true, 1048576},
                                                      {1048576, false, 524288},
                                                      {524288, true, 1048576},
                                                      {0, false, 10240},
                                                      {0, true, 20480}}) {
        Rig rig(model);
        auto &control = *rig.control;
        const auto *spec = control.getSpecification();
        DsoSettingsScopeHorizontal horizontal;
        horizontal.recordLength = saved.count;
        // MainWindow initially uses the controller's normal-mode list. A saved
        // fast count must map to the same ordinal rather than the short default.
        const unsigned index =
            horizontal.normalizeRecordLength(control.getAvailableRecordLengths(), spec->samplerate.multi.recordLengths);
        check(index == (saved.count == 0 ? 1u : 2u), "Saved record selection lost its normal/fast ordinal");
        // Exercise the actual controller signal connected to the GUI selector.
        // It overwrites the displayed count while the saved index stays intact.
        QObject::connect(&control, &HantekDsoControl::recordLengthChanged,
                         [&horizontal](unsigned count) { horizontal.recordLength = count; });
        ok(control.setChannelUsed(0, true), "Activate saved CH1");
        ok(control.setChannelUsed(1, !saved.fast), "Activate saved CH2");
        ok(control.setSamplerate(saved.fast ? 250e6 : 1e6), "Restore saved samplerate");
        if (saved.fast)
            check(horizontal.recordLength == 20480, "Fixture did not reproduce the GUI's fast-rate count update");
        ok(control.setRecordLength(index), "Restore saved record index after rate-change signals");
        check(control.getAvailableRecordLengths()[control.getDeviceSettings()->recordLengthId] == saved.expected &&
                  horizontal.recordLength == saved.expected,
              "Saved long/default record did not survive startup rate changes");
    }
}

QMap<int, unsigned char> parseFixture(const QByteArray &hex) {
    QMap<int, unsigned char> memory;
    for (const auto &line : hex.split('\n')) {
        if (line.trimmed().isEmpty()) continue;
        const auto record = QByteArray::fromHex(line.trimmed().mid(1));
        unsigned sum = 0;
        for (char byte : record) sum += static_cast<unsigned char>(byte);
        check(sum % 256 == 0, "Patched output has an invalid Intel HEX checksum");
        const int count = static_cast<unsigned char>(record[0]);
        if (static_cast<unsigned char>(record[3]) == 1) continue;
        const int address = (static_cast<unsigned char>(record[1]) << 8) | static_cast<unsigned char>(record[2]);
        for (int i = 0; i < count; ++i) memory[address + i] = static_cast<unsigned char>(record[4 + i]);
    }
    return memory;
}

void firmwareSafety() {
    QFile file(QString::fromUtf8(DSO2250_FACTORY_HEX));
    check(file.open(QIODevice::ReadOnly), "Factory firmware fixture could not be read");
    const QByteArray original = file.readAll();
    QByteArray prepared = original;
    QString error;
    check(prepareDso2250Firmware(prepared, error), "Audited factory firmware was rejected");
    const auto memory = parseFixture(prepared);
    auto block = [&memory](int address, int count) {
        QByteArray data;
        for (int i = 0; i < count; ++i) {
            check(memory.contains(address + i), "Missing code byte");
            data += static_cast<char>(memory.value(address + i));
        }
        return data;
    };
    check(block(0x0cbe, 6) == QByteArray::fromHex("021280000000"),
          "Startup hook did not reach the verified free code area");
    const QByteArray clockGolden = QByteArray::fromHex("90e60174c3f000000090e60b7400f000000090e60174c3020cc4");
    check(block(0x1280, clockGolden.size()) == clockGolden,
          "Startup clock/REVCTL sequence differs from audited sequence");
    const int late = (memory[0x0d58] << 8) | memory[0x0d59];
    check(block(late, 12) == QByteArray::fromHex("c0d0c0e0eec0e0efc0e07ec8"),
          "Startup delay does not preserve the selected register bank");
    const auto externalGolden = QByteArray::fromHex("90e6017443f000000090e67ae04401f022");
    check(block(late + 28, externalGolden.size()) == externalGolden,
          "External clock/I2CTL resume differs from audited sequence");
    const int speed = (memory[0x0168] << 8) | memory[0x0169];
    const auto speedGolden = QByteArray::fromHex("90e680e0235401c302016b");
    check(block(speed, speedGolden.size()) == speedGolden, "B2 does not read live USBCS speed");
    QByteArray corrupt = original;
    const auto firstEnd = corrupt.indexOf('\n');
    int checksumEnd = firstEnd;
    if (corrupt[checksumEnd - 1] == '\r') --checksumEnd;
    corrupt[checksumEnd - 1] = corrupt[checksumEnd - 1] == '0' ? '1' : '0';
    const QByteArray before = corrupt;
    check(!prepareDso2250Firmware(corrupt, error) && corrupt == before,
          "Invalid checksum was accepted or partially modified");
    QByteArray foreign = ":0100000000FF\n:00000001FF\n";
    const auto beforeForeign = foreign;
    check(!prepareDso2250Firmware(foreign, error) && foreign == beforeForeign, "Foreign firmware was patched");
    // Keep the complete factory layout, size and valid checksums, changing only
    // one instruction. This exercises the identity guard rather than parsing.
    auto changedRecord = QByteArray::fromHex(original.left(firstEnd).trimmed().mid(1));
    check(static_cast<unsigned char>(changedRecord[0]) > 0, "Factory fixture first record has no data");
    changedRecord[4] = static_cast<char>(static_cast<unsigned char>(changedRecord[4]) ^ 1);
    unsigned changedSum = 0;
    for (int i = 0; i < changedRecord.size() - 1; ++i) changedSum += static_cast<unsigned char>(changedRecord[i]);
    changedRecord[changedRecord.size() - 1] = static_cast<char>((0u - changedSum) & 0xff);
    QByteArray unknownImage = ":" + changedRecord.toHex() + "\n" + original.mid(firstEnd + 1);
    const auto beforeUnknown = unknownImage;
    check(!prepareDso2250Firmware(unknownImage, error) && unknownImage == beforeUnknown,
          "Unknown checksummed full-size firmware was patched");
    const auto beforePrepared = prepared;
    check(!prepareDso2250Firmware(prepared, error) && prepared == beforePrepared,
          "Already patched firmware was changed again");
}
} // namespace

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        usbLifetime();
        controllerGoldens();
        unaffected2090Golden();
        savedRecordLengths();
        firmwareSafety();
        std::cout << "DSO2250 SDK packet, calibration, revision, RAM-order and firmware safety tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
