// SPDX-License-Identifier: GPL-2.0+

#include "hantekdso/modelspecification.h"

Dso::ModelSpec::ModelSpec(unsigned channels) : channels(channels) { calibration.resize(channels); }
