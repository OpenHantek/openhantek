#pragma once

#include "dsomodel.h"

class DsoControl;

struct ModelDSO5200 : public DSOModel {
    static const int ID = 0x5200;
    ModelDSO5200();
    void applyRequirements(DsoCommandQueue *dsoControl) const override;
};

struct ModelDSO5200A : public DSOModel {
    static const int ID = 0x5200;
    ModelDSO5200A();
    void applyRequirements(DsoCommandQueue* dsoControl) const override;
};
