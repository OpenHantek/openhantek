// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "exporterinterface.h"

namespace Exporter {
class Print : public ExporterInterface {
  public:
    Print();
    virtual bool exportNow(Registry *registry) override;
    virtual QIcon icon() override;
    virtual QString name() override;
    virtual Type type() override;
    virtual float samples(const std::shared_ptr<PPresult> data) override;
    virtual QKeySequence shortcut() override;
};
}
