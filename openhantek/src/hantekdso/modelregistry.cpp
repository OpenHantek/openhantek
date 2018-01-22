// SPDX-License-Identifier: GPL-2.0+

#include "modelregistry.h"

ModelRegistry *ModelRegistry::get() {
    static ModelRegistry *instance = new ModelRegistry();
    return instance;
}

void ModelRegistry::add(DSOModel *model) { supportedModels.push_back(model); }

const std::list<DSOModel *> ModelRegistry::models() const { return supportedModels; }
