// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <inttypes.h>
#include <vector>
#include "codes.h"

class BulkCommand : public std::vector<uint8_t> {
protected:
    BulkCommand(HantekE::BulkCode code, unsigned size);
public:
    HantekE::BulkCode code;
    bool pending = false;
    BulkCommand* next = nullptr;
};
