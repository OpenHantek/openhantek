#include "modelDSO2090.h"
#include "dsocommandqueue.h"
#include "modelspecification.h"

using namespace Hantek;

static ModelDSO2090 modelInstance;
static ModelDSO2090A modelInstance2;

void _applyRequirements(DsoCommandQueue *commandQueue) {
    commandQueue->addCommand(new BulkForceTrigger(), false);
    commandQueue->addCommand(new BulkCaptureStart(), false);
    commandQueue->addCommand(new BulkTriggerEnabled(), false);
    commandQueue->addCommand(new BulkGetData(), false);
    commandQueue->addCommand(new BulkGetCaptureState(), false);
    commandQueue->addCommand(new BulkSetGain(), false);

    commandQueue->addCommand(new BulkSetTriggerAndSamplerate(), false);
    commandQueue->addCommand(new ControlSetOffset(), false);
    commandQueue->addCommand(new ControlSetRelays(), false);
}

void initSpecifications(Dso::ModelSpec *specification) {
    specification->cmdSetRecordLength = HantekE::BulkCode::SETTRIGGERANDSAMPLERATE;
    specification->cmdSetChannels = HantekE::BulkCode::SETTRIGGERANDSAMPLERATE;
    specification->cmdSetSamplerate = HantekE::BulkCode::SETTRIGGERANDSAMPLERATE;
    specification->cmdSetTrigger = HantekE::BulkCode::SETTRIGGERANDSAMPLERATE;
    specification->cmdSetPretrigger = HantekE::BulkCode::SETTRIGGERANDSAMPLERATE;

    specification->normalSamplerate.base = 50e6;
    specification->normalSamplerate.max = 50e6;
    specification->normalSamplerate.maxDownsampler = 131072;
    specification->normalSamplerate.recordLengths = {{UINT_MAX, 1000}, {10240, 1}, {32768, 1}};
    specification->fastrateSamplerate.base = 100e6;
    specification->fastrateSamplerate.max = 100e6;
    specification->fastrateSamplerate.maxDownsampler = 131072;
    specification->fastrateSamplerate.recordLengths = {{UINT_MAX, 1000}, {20480, 1}, {65536, 1}};
    specification->calibration[0] = {{0x0000, 0xffff, 255}, {0x0000, 0xffff, 255}, {0x0000, 0xffff, 255},
                                     {0x0000, 0xffff, 255}, {0x0000, 0xffff, 255}, {0x0000, 0xffff, 255},
                                     {0x0000, 0xffff, 255}, {0x0000, 0xffff, 255}, {0x0000, 0xffff, 255}};
    specification->calibration[1] = {{0x0000, 0xffff, 255}, {0x0000, 0xffff, 255}, {0x0000, 0xffff, 255},
                                     {0x0000, 0xffff, 255}, {0x0000, 0xffff, 255}, {0x0000, 0xffff, 255},
                                     {0x0000, 0xffff, 255}, {0x0000, 0xffff, 255}, {0x0000, 0xffff, 255}};
    specification->gain = {{0, 0.08}, {1, 0.16}, {2, 0.40},  {0, 0.80}, {1, 1.60},
                           {2, 4.00}, {0, 8.00}, {1, 16.00}, {2, 40.00}};
    specification->sampleSize = 8;
    specification->specialTriggerChannels = {{"EXT", -2}, {"EXT/10", -3}};
}

ModelDSO2090::ModelDSO2090()
    : DSOModel(ID, 0x04b5, 0x2090, 0x04b4, 0x2090, "dso2090x86", "DSO-2090", new Dso::ModelSpec(2)) {
    initSpecifications(specification);
}

void ModelDSO2090::applyRequirements(DsoCommandQueue *dsoControl) const { _applyRequirements(dsoControl); }

ModelDSO2090A::ModelDSO2090A()
    : DSOModel(ID, 0x04b5, 0x2090, 0x04b4, 0x8613, "dso2090x86", "DSO-2090", new Dso::ModelSpec(2)) {
    initSpecifications(specification);
}

void ModelDSO2090A::applyRequirements(DsoCommandQueue *dsoControl) const { _applyRequirements(dsoControl); }
