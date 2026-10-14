// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QByteArray>
#include <QString>

/// Prepare the audited DSO-2250 image for RAM upload. On failure, hex is unchanged.
/// The bundled factory firmware is retained; this modifies its temporary copy.
bool prepareDso2250Firmware(QByteArray &hex, QString &error);
