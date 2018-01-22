// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "processor.h"

namespace Settings {
class Scope;
}
class PPresult;

namespace PostProcessing {

class MathChannelGenerator : public Processor
{
public:
    MathChannelGenerator(const ::Settings::Scope *scope);
    virtual ~MathChannelGenerator();
    virtual void process(PPresult *) override;
private:
    const ::Settings::Scope *scope;
};

}
