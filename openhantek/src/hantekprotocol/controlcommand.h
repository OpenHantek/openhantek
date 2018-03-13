// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <inttypes.h>
#include <vector>
#include "codes.h"


class ControlCommand : public std::vector<uint8_t> {
protected:
    ControlCommand(HantekE::ControlCode code, unsigned size);
public:
    bool pending = false;
    uint8_t code;
    uint8_t value = 0;
    ControlCommand* next = nullptr;
};
