#pragma once

#include "dsomodel.h"

class DsoControl;

struct ModelDSO2250 : public DSOModel {
    static const int ID = 0x2250;
    ModelDSO2250();
    void applyRequirements(DsoCommandQueue* dsoControl) const override;
};
