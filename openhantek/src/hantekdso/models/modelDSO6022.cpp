#include "modelDSO6022.h"
#include "dsocommandqueue.h"
#include "modelspecification.h"
#include "usb/usbdevice.h"

using namespace Hantek;

static ModelDSO6022BE modelInstance;
static ModelDSO6022BL modelInstance2;

static void initSpecifications(Dso::ModelSpec *specification) {
    // 6022xx do not support any bulk commands
    specification->useControlNoBulk = true;
    specification->isSoftwareTriggerDevice = true;
    specification->isFixedSamplerateDevice = true;
    specification->supportsCaptureState = false;
    specification->supportsOffset = false;
    specification->supportsCouplingRelays = false;
    specification->supportsFastRate = false;

    specification->normalSamplerate.base = 1e6;
    specification->normalSamplerate.max = 48e6;
    specification->normalSamplerate.maxDownsampler = 10;
    specification->normalSamplerate.recordLengths = {{10240, 1}};
    specification->fastrateSamplerate.base = 1e6;
    specification->fastrateSamplerate.max = 48e6;
    specification->fastrateSamplerate.maxDownsampler = 10;
    specification->fastrateSamplerate.recordLengths = {{20480, 1}};
    // This data was based on testing and depends on Divider.
    specification->calibration[0] = {{0x0000, 0xfd, 10},  {0x0000, 0xfd, 20},  {0x0000, 0xfd, 49},
                                     {0x0000, 0xfd, 99},  {0x0000, 0xfd, 198}, {0x0000, 0xfd, 400},
                                     {0x0000, 0xfd, 800}, {0x0000, 0xfd, 1600}};
    specification->calibration[1] = {{0x0000, 0xfd, 10},  {0x0000, 0xfd, 20},  {0x0000, 0xfd, 49},
                                     {0x0000, 0xfd, 99},  {0x0000, 0xfd, 198}, {0x0000, 0xfd, 400},
                                     {0x0000, 0xfd, 800}, {0x0000, 0xfd, 1600}};
    specification->gain = {{10, 0.08}, {10, 0.16}, {10, 0.40}, {10, 0.80},
                           {10, 1.60}, {2, 4.00},  {2, 8.00},  {2, 16.00}};
    specification->fixedSampleRates = {{10, 1e5}, {20, 2e5}, {50, 5e5},  {1, 1e6},   {2, 2e6},
                                       {4, 4e6},  {8, 8e6},  {16, 16e6}, {24, 24e6}, {48, 48e6}};
    specification->sampleSize = 8;

    specification->couplings = {DsoE::Coupling::DC};
    specification->triggerModes = {DsoE::TriggerMode::HARDWARE_SOFTWARE, DsoE::TriggerMode::SINGLE};
    specification->fixedUSBinLength = 16384;
}

void applyRequirements_(DsoCommandQueue *dsoControl) {
    dsoControl->addCommand(new ControlAcquireHardData(), false);
    dsoControl->addCommand(new ControlSetTimeDIV(), false);
    dsoControl->addCommand(new ControlSetVoltDIV_CH2(), false);
    dsoControl->addCommand(new ControlSetVoltDIV_CH1(), false);
}

ModelDSO6022BE::ModelDSO6022BE()
    : DSOModel(ID, 0x04b5, 0x6022, 0x04b4, 0x6022, "dso6022be", "DSO-6022BE", new Dso::ModelSpec(2)) {
    initSpecifications(specification);
}

void ModelDSO6022BE::applyRequirements(DsoCommandQueue *dsoControl) const { applyRequirements_(dsoControl); }

ModelDSO6022BL::ModelDSO6022BL()
    : DSOModel(ID, 0x04b5, 0x602a, 0x04b4, 0x602a, "dso6022bl", "DSO-6022BL", new Dso::ModelSpec(2)) {
    initSpecifications(specification);
}

void ModelDSO6022BL::applyRequirements(DsoCommandQueue *dsoControl) const { applyRequirements_(dsoControl); }
