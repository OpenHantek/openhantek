#include "modelDSO5200.h"
#include "dsocommandqueue.h"
#include "modelspecification.h"

using namespace Hantek;

static ModelDSO5200 modelInstance;
static ModelDSO5200A modelInstance2;

static void initSpecifications(Dso::ModelSpec *specification) {
    specification->cmdSetRecordLength = HantekE::BulkCode::DSETBUFFER;
    specification->cmdSetChannels = HantekE::BulkCode::ESETTRIGGERORSAMPLERATE;
    specification->cmdSetSamplerate = HantekE::BulkCode::CSETTRIGGERORSAMPLERATE;
    specification->cmdSetTrigger = HantekE::BulkCode::ESETTRIGGERORSAMPLERATE;
    specification->cmdSetPretrigger = HantekE::BulkCode::ESETTRIGGERORSAMPLERATE;

    specification->normalSamplerate.base = 100e6;
    specification->normalSamplerate.max = 125e6;
    specification->normalSamplerate.maxDownsampler = 131072;
    specification->normalSamplerate.recordLengths = {{UINT_MAX, 1000}, {10240, 1}, {14336, 1}};
    specification->fastrateSamplerate.base = 200e6;
    specification->fastrateSamplerate.max = 250e6;
    specification->fastrateSamplerate.maxDownsampler = 131072;
    specification->fastrateSamplerate.recordLengths = {{UINT_MAX, 1000}, {20480, 1}, {28672, 1}};
    specification->calibration[0] = {{0x0000, 0xffff, 368}, {0x0000, 0xffff, 454}, {0x0000, 0xffff, 908},
                                     {0x0000, 0xffff, 368}, {0x0000, 0xffff, 454}, {0x0000, 0xffff, 908},
                                     {0x0000, 0xffff, 368}, {0x0000, 0xffff, 454}, {0x0000, 0xffff, 908}};
    specification->calibration[1] = {{0x0000, 0xffff, 368}, {0x0000, 0xffff, 454}, {0x0000, 0xffff, 908},
                                     {0x0000, 0xffff, 368}, {0x0000, 0xffff, 454}, {0x0000, 0xffff, 908},
                                     {0x0000, 0xffff, 368}, {0x0000, 0xffff, 454}, {0x0000, 0xffff, 908}};
    specification->gain = {{1, 0.16}, {0, 0.40}, {0, 0.80}, {1, 1.60}, {0, 4.00},
                           {0, 8.00}, {1, 16.0}, {0, 40.0}, {0, 80.0}};
    specification->sampleSize = 10;
    specification->specialTriggerChannels = {{"EXT", -2}, {"EXT/10", -3}}; // 3, 4
}

static void _applyRequirements(DsoCommandQueue *dsoControl) {
    dsoControl->addCommand(new BulkForceTrigger(), false);
    dsoControl->addCommand(new BulkCaptureStart(), false);
    dsoControl->addCommand(new BulkTriggerEnabled(), false);
    dsoControl->addCommand(new BulkGetData(), false);
    dsoControl->addCommand(new BulkGetCaptureState(), false);
    dsoControl->addCommand(new BulkSetGain(), false);

    // Instantiate additional commands for the DSO-5200
    dsoControl->addCommand(new BulkSetSamplerate5200(), false);
    dsoControl->addCommand(new BulkSetBuffer5200(), false);
    dsoControl->addCommand(new BulkSetTrigger5200(), false);
    dsoControl->addCommand(new ControlSetOffset(), false);
    dsoControl->addCommand(new ControlSetRelays(), false);
}

ModelDSO5200::ModelDSO5200()
    : DSOModel(ID, 0x04b5, 0x5200, 0x04b4, 0x5200, "dso5200x86", "DSO-5200", new Dso::ModelSpec(2)) {
    initSpecifications(specification);
}

void ModelDSO5200::applyRequirements(DsoCommandQueue *dsoControl) const { _applyRequirements(dsoControl); }

ModelDSO5200A::ModelDSO5200A()
    : DSOModel(ID, 0x04b5, 0x520a, 0x04b4, 0x520a, "dso5200ax86", "DSO-5200A", new Dso::ModelSpec(2)) {
    initSpecifications(specification);
}

void ModelDSO5200A::applyRequirements(DsoCommandQueue *dsoControl) const { _applyRequirements(dsoControl); }
