// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include <chrono>
#include <inttypes.h>
using namespace std::literals::chrono_literals;

typedef unsigned RecordLengthID;
typedef unsigned ChannelID;

using Samples = double;
Samples operator""_S(long double); ///< User literal for samples

using Voltage = double;

using Seconds = std::chrono::duration<double>;
