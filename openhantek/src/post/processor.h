// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "ppresult.h"

namespace PostProcessing {

class Processor {
public:
    virtual void process(PPresult*) = 0;
};

}
