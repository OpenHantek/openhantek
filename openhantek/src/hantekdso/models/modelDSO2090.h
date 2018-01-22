#pragma once

#include "dsomodel.h"

class DsoCommandQueue;

struct ModelDSO2090 : public DSOModel {
    static const int ID = 0x2090;
    ModelDSO2090();
    void applyRequirements(DsoCommandQueue *dsoControl) const override;
};

struct ModelDSO2090A : public DSOModel {
    static const int ID = 0x2090;
    ModelDSO2090A();
    void applyRequirements(DsoCommandQueue* dsoControl) const override;
};

