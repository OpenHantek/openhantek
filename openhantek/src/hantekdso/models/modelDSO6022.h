#pragma once

#include "dsomodel.h"

class DsoControl;

struct ModelDSO6022BE : public DSOModel {
    static const int ID = 0x6022;
    ModelDSO6022BE();
    virtual void applyRequirements(DsoCommandQueue *dsoControl) const override;
};

struct ModelDSO6022BL : public DSOModel {
    static const int ID = 0x6022;
    ModelDSO6022BL();
    virtual void applyRequirements(DsoCommandQueue* dsoControl) const override;
};
