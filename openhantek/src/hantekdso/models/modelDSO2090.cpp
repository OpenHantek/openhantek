#include "modelDSO2090.h"
#include "hantekprotocol/bulkStructs.h"
#include "hantekprotocol/controlStructs.h"
#include "hantekdsocontrol.h"

using namespace Hantek;

static ModelDSO2090 modelInstance;
static ModelDSO2090A modelInstance2;

void _applyRequirements(HantekDsoControl *dsoControl) {
    dsoControl->addCommand(new BulkForceTrigger(), false);
    dsoControl->addCommand(new BulkCaptureStart(), false);
    dsoControl->addCommand(new BulkTriggerEnabled(), false);
    dsoControl->addCommand(new BulkGetData(), false);
    dsoControl->addCommand(new BulkGetCaptureState(), false);
    dsoControl->addCommand(new BulkSetGain(), false);

    dsoControl->addCommand(new BulkSetTriggerAndSamplerate(), false);
    dsoControl->addCommand(new ControlSetOffset(), false);
    dsoControl->addCommand(new ControlSetRelays(), false);
}

void initSpecifications(Dso::ControlSpecification& specification) {
    specification.cmdSetRecordLength = BulkCode::SETTRIGGERANDSAMPLERATE;
    specification.cmdSetChannels = BulkCode::SETTRIGGERANDSAMPLERATE;
    specification.cmdSetSamplerate = BulkCode::SETTRIGGERANDSAMPLERATE;
    specification.cmdSetTrigger = BulkCode::SETTRIGGERANDSAMPLERATE;
    specification.cmdSetPretrigger = BulkCode::SETTRIGGERANDSAMPLERATE;

    specification.samplerate.single.base = 50e6;
    specification.samplerate.single.max = 50e6;
    specification.samplerate.single.maxDownsampler = 131072;
    specification.samplerate.single.recordLengths = {UINT_MAX, 10240, 32768};
    specification.samplerate.multi.base = 100e6;
    specification.samplerate.multi.max = 100e6;
    specification.samplerate.multi.maxDownsampler = 131072;
    specification.samplerate.multi.recordLengths = {UINT_MAX, 20480, 65536};
    specification.bufferDividers = { 1000 , 1 , 1 };
    specification.voltageLimit[0] = { 255 , 255 , 255 , 255 , 255 , 255 , 255 , 255 , 255 };
    specification.voltageLimit[1] = { 255 , 255 , 255 , 255 , 255 , 255 , 255 , 255 , 255 };
    specification.gain = { {0,0.08} , {1,0.16} , {2,0.40} , {0,0.80} ,
                           {1,1.60} , {2,4.00} , {0,8.00} , {1,16.00} , {2,40.00} };
    specification.sampleSize = 8;
    specification.specialTriggerChannels = {{"EXT", -1}};
}

ModelDSO2090::ModelDSO2090() : DSOModel(ID, 0x04b5, 0x2090, 0x04b4, 0x2090, "dso2090x86", "DSO-2090",
                                        Dso::ControlSpecification(2)) {
    initSpecifications(specification);
}

void ModelDSO2090::applyRequirements(HantekDsoControl *dsoControl)  const {
    _applyRequirements(dsoControl);
}

ModelDSO2090A::ModelDSO2090A() : DSOModel(ID, 0x04b5, 0x2090, 0x04b4, 0x8613, "dso2090x86", "DSO-2090",
                                          Dso::ControlSpecification(2)) {
    initSpecifications(specification);
}

void ModelDSO2090A::applyRequirements(HantekDsoControl *dsoControl)  const {
    _applyRequirements(dsoControl);
}

