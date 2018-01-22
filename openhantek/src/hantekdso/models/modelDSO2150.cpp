#include "modelDSO2150.h"
#include "dsocommandqueue.h"
#include "modelspecification.h"

using namespace Hantek;

static ModelDSO2150 modelInstance;

ModelDSO2150::ModelDSO2150()
    : DSOModel(ID, 0x04b5, 0x2150, 0x04b4, 0x2150, "dso2150x86", "DSO-2150", new Dso::ModelSpec(2)) {
    specification->cmdSetRecordLength = BulkCode::SETTRIGGERANDSAMPLERATE;
    specification->cmdSetChannels = BulkCode::SETTRIGGERANDSAMPLERATE;
    specification->cmdSetSamplerate = BulkCode::SETTRIGGERANDSAMPLERATE;
    specification->cmdSetTrigger = BulkCode::SETTRIGGERANDSAMPLERATE;
    specification->cmdSetPretrigger = BulkCode::SETTRIGGERANDSAMPLERATE;

    specification->normalSamplerate.base = 50e6;
    specification->normalSamplerate.max = 75e6;
    specification->normalSamplerate.maxDownsampler = 131072;
    specification->normalSamplerate.recordLengths = {{UINT_MAX, 1000}, {10240, 1}, {32768, 1}};
    specification->fastrateSamplerate.base = 100e6;
    specification->fastrateSamplerate.max = 150e6;
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

void ModelDSO2150::applyRequirements(DsoCommandQueue *commandQueue) const {
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
