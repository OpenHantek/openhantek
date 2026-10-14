// SPDX-License-Identifier: GPL-2.0+

#include "dso2250firmware.h"

#include <QCryptographicHash>
#include <QMap>

namespace {
using Image = QMap<int, unsigned char>;

bool fail(QString &error, const char *message) {
    error = QString::fromLatin1(message);
    return false;
}

bool parseHex(const QByteArray &hex, Image &image, QString &error) {
    bool eof = false;
    for (const QByteArray &rawLine : hex.split('\n')) {
        const QByteArray line = rawLine.trimmed();
        if (line.isEmpty()) continue;
        if (eof) return fail(error, "Data after the firmware end record.");
        if (line.at(0) != ':' || line.size() < 11 || (line.size() - 1) % 2 != 0)
            return fail(error, "Invalid firmware record.");
        for (int i = 1; i < line.size(); ++i) {
            const char c = line.at(i);
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
                return fail(error, "Invalid firmware hexadecimal digit.");
        }
        const QByteArray record = QByteArray::fromHex(line.mid(1));
        const int count = static_cast<unsigned char>(record.at(0));
        if (record.size() != count + 5) return fail(error, "Invalid firmware record length.");
        unsigned int sum = 0;
        for (char byte : record) sum += static_cast<unsigned char>(byte);
        if (sum % 256 != 0) return fail(error, "Invalid firmware checksum.");
        const int address = (static_cast<unsigned char>(record.at(1)) << 8) | static_cast<unsigned char>(record.at(2));
        const int type = static_cast<unsigned char>(record.at(3));
        if (type == 1) {
            if (count != 0 || address != 0) return fail(error, "Invalid firmware end record.");
            eof = true;
            continue;
        }
        if (type != 0 || address + count > 0x1800) return fail(error, "Unsupported firmware addressing.");
        for (int i = 0; i < count; ++i) {
            if (image.contains(address + i)) return fail(error, "Overlapping firmware records.");
            image.insert(address + i, static_cast<unsigned char>(record.at(4 + i)));
        }
    }
    if (!eof) return fail(error, "Missing firmware end record.");
    return true;
}

bool replace(Image &image, int address, const QByteArray &expected, const QByteArray &replacement, QString &error) {
    if (expected.size() != replacement.size()) return fail(error, "Invalid firmware patch size.");
    for (int i = 0; i < expected.size(); ++i) {
        if (!image.contains(address + i) || image.value(address + i) != static_cast<unsigned char>(expected.at(i)))
            return fail(error, "Unsupported firmware instruction layout.");
    }
    for (int i = 0; i < replacement.size(); ++i) image[address + i] = static_cast<unsigned char>(replacement.at(i));
    return true;
}

QByteArray jump(int destination, int length) {
    QByteArray bytes(length, '\0');
    bytes[0] = 0x02;
    bytes[1] = static_cast<char>(destination >> 8);
    bytes[2] = static_cast<char>(destination & 0xff);
    return bytes;
}

QByteArray serializeHex(const Image &image) {
    QByteArray hex;
    auto it = image.constBegin();
    while (it != image.constEnd()) {
        const int address = it.key();
        QByteArray data;
        while (it != image.constEnd() && data.size() < 16 && it.key() == address + data.size()) {
            data.append(static_cast<char>(it.value()));
            ++it;
        }
        QByteArray record;
        record.append(static_cast<char>(data.size()));
        record.append(static_cast<char>(address >> 8));
        record.append(static_cast<char>(address & 0xff));
        record.append('\0');
        record.append(data);
        unsigned int sum = 0;
        for (char byte : record) sum += static_cast<unsigned char>(byte);
        record.append(static_cast<char>((0u - sum) & 0xff));
        hex.append(':');
        hex.append(record.toHex().toUpper());
        hex.append('\n');
    }
    hex.append(":00000001FF\n");
    return hex;
}
} // namespace

bool prepareDso2250Firmware(QByteArray &hex, QString &error) {
    error.clear();
    Image image;
    if (!parseHex(hex, image, error)) return false;
    if (image.size() != 4620 || image.isEmpty() || image.lastKey() != 0x1279)
        return fail(error, "Unsupported DSO-2250 firmware size.");
    QByteArray original(4730, '\0');
    for (auto it = image.constBegin(); it != image.constEnd(); ++it) original[it.key()] = static_cast<char>(it.value());
    const QByteArray digest = QCryptographicHash::hash(original, QCryptographicHash::Sha256).toHex();
    if (digest != "1d9b3a902caa8a9b92e8f1ba718060113ea7578f6a19dcea88656c51e1455c1b")
        return fail(error, "Unrecognized DSO-2250 firmware image.");

    // Initialize the synchronous FIFOs under a valid internal clock, normalize
    // retained REVCTL state, and preserve the stock ACC/DPTR before resuming.
    // C3 and 43 both keep IFCLKOE=0, so the FX2 never drives the IFCLK pin.
    const QByteArray startup = QByteArray::fromHex("90e60174c3f000000090e60b7400f000000090e60174c3020cc4");
    // Save PSW/ACC and R6/R7 from the current register bank. The 200 x255 DJNZ
    // loop waits about 25.7ms at 24MHz before selecting the external FPGA clock.
    // Finish with the exact displaced I2CTL update and RET.
    const QByteArray external = QByteArray::fromHex("c0d0c0e0eec0e0efc0e07ec87fffdffedefad0e0ffd0e0fed0e0d0d0"
                                                    "90e6017443f000000090e67ae04401f022");
    // B2 reports USBCS.HSM directly; the original software cache is cleared on
    // warm CPU restart even though USB hardware remains configured.
    const QByteArray speed = QByteArray::fromHex("90e680e0235401c302016b");
    const int start = 0x1280;
    const int late = start + startup.size();
    const int liveSpeed = late + external.size();
    const QByteArray code = startup + external + speed;
    if (start + code.size() > 0x1800) return fail(error, "Firmware patch exceeds code memory.");
    for (int i = 0; i < code.size(); ++i) {
        if (image.contains(start + i)) return fail(error, "Firmware patch overlaps existing code.");
    }
    if (!replace(image, 0x0cbe, QByteArray::fromHex("90e6017443f0"), jump(start, 6), error) ||
        !replace(image, 0x0d57, QByteArray::fromHex("90e67ae04401f022"), jump(late, 8), error) ||
        !replace(image, 0x0167, QByteArray::fromHex("a20ce433"), jump(liveSpeed, 4), error))
        return false;
    for (int i = 0; i < code.size(); ++i) image.insert(start + i, static_cast<unsigned char>(code.at(i)));
    hex = serializeHex(image);
    return true;
}
