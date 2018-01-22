// SPDX-License-Identifier: GPL-2.0+

#include "dsomodel.h"
#include "modelregistry.h"
#include "hantekdso/modelspecification.h"

DSOModel::DSOModel(int id, long vendorID, long productID, long vendorIDnoFirmware, long productIDnoFirmware,
                   const std::string &firmwareToken, const std::string &name, Dso::ModelSpec *specification)
    : ID(id), vendorID(vendorID), productID(productID), vendorIDnoFirmware(vendorIDnoFirmware),
      productIDnoFirmware(productIDnoFirmware), firmwareToken(firmwareToken), name(name), specification(specification) {
    ModelRegistry::get()->add(this);
}

DSOModel::~DSOModel() { delete specification; }
