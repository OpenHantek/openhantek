#include "modelDSO2250.h"
#include "dsocommandqueue.h"
#include "modelspecification.h"

using namespace Hantek;

static ModelDSO2250 modelInstance;

ModelDSO2250::ModelDSO2250()
    : DSOModel(ID, 0x04b5, 0x2250, 0x04b4, 0x2250, "dso2250x86", "DSO-2250", new Dso::ModelSpec(2)) {
    specification->cmdSetRecordLength = BulkCode::DSETBUFFER;
    specification->cmdSetChannels = BulkCode::BSETCHANNELS;
    specification->cmdSetSamplerate = BulkCode::ESETTRIGGERORSAMPLERATE;
    specification->cmdSetTrigger = BulkCode::CSETTRIGGERORSAMPLERATE;
    specification->cmdSetPretrigger = BulkCode::FSETBUFFER;

    specification->normalSamplerate.base = 100e6;
    specification->normalSamplerate.max = 100e6;
    specification->normalSamplerate.maxDownsampler = 65536;
    specification->normalSamplerate.recordLengths = {{UINT_MAX, 1000}, {10240, 1}, {524288, 1}};
    specification->fastrateSamplerate.base = 200e6;
    specification->fastrateSamplerate.max = 250e6;
    specification->fastrateSamplerate.maxDownsampler = 65536;
    specification->fastrateSamplerate.recordLengths = {{UINT_MAX, 1000}, {20480, 1}, {1048576, 1}};
    specification->calibration[0] = {{0x0000, 0xffff, 255}, {0x0000, 0xffff, 255}, {0x0000, 0xffff, 255},
                                     {0x0000, 0xffff, 255}, {0x0000, 0xffff, 255}, {0x0000, 0xffff, 255},
                                     {0x0000, 0xffff, 255}, {0x0000, 0xffff, 255}, {0x0000, 0xffff, 255}};
    specification->calibration[1] = {{0x0000, 0xffff, 255}, {0x0000, 0xffff, 255}, {0x0000, 0xffff, 255},
                                     {0x0000, 0xffff, 255}, {0x0000, 0xffff, 255}, {0x0000, 0xffff, 255},
                                     {0x0000, 0xffff, 255}, {0x0000, 0xffff, 255}, {0x0000, 0xffff, 255}};
    specification->gain = {{0, 0.08}, {2, 0.16}, {3, 0.40},  {0, 0.80}, {2, 1.60},
                           {3, 4.00}, {0, 8.00}, {2, 16.00}, {3, 40.00}};
    specification->sampleSize = 8;
    specification->specialTriggerChannels = {{"EXT", -2}};
}

void ModelDSO2250::applyRequirements(DsoCommandQueue *dsoControl) const {
    dsoControl->addCommand(new BulkForceTrigger(), false);
    dsoControl->addCommand(new BulkCaptureStart(), false);
    dsoControl->addCommand(new BulkTriggerEnabled(), false);
    dsoControl->addCommand(new BulkGetData(), false);
    dsoControl->addCommand(new BulkGetCaptureState(), false);
    dsoControl->addCommand(new BulkSetGain(), false);

    // Instantiate additional commands for the DSO-2250
    dsoControl->addCommand(new BulkSetChannels2250(), false);
    dsoControl->addCommand(new BulkSetTrigger2250(), false);
    dsoControl->addCommand(new BulkSetRecordLength2250(), false);
    dsoControl->addCommand(new BulkSetSamplerate2250(), false);
    dsoControl->addCommand(new BulkSetBuffer2250(), false);
    dsoControl->addCommand(new ControlSetOffset(), false);
    dsoControl->addCommand(new ControlSetRelays(), false);
}
