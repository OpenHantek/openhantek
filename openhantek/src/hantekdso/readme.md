# Content
This directory contains the heart of OpenHantek, the `DSOControl` class and all model definitions.

## DsoControl and DsoLoop

This is the core logic class that ties together the USBDevice object, the protocol parts, the sample-fetch
loop (DsoLoop) and command queue (DsoCommandQueue). Device settings can only be done through this class,
but are stored separately in the DeviceSettings object.

DsoLoop is responsible for fetching new samples from the Dso at the right time, converting it to a normalized
range [-1,1] of values and provide the result via `getLastSamples()`. Observers are notified of a new set of
available samples via the signal `samplesAvailable()`. The DsoSamples structure that is used for result retrieving
is allocated only once and will be overwritten in each loop step, it is therefore necessary to copy the result.
The structure provides a mutex to synchronize concurrent threads (usually the post processing thread and the
DsoControl thread).

## DsoCommandQueue

DsoControl inherits from DsoCommandQueue and uses the provided two command queues for usb-control and usb-bulk
commands. Commands like setGain() are not performed directly but in a batched way.
The first step of each fetch-sample loop is to send all pending/queued commands via `sendPendingCommands()`.

DsoCommandQueue requires a model to "register" all necessary commands beforehand via `addCommand(cmd)`. The command
can be altered later via `modifyCommand(id)`. Because commands are not applied directly, only the last modification
before a new loop cycle is going to begin, will actually be send to the device.

## DSO Model

A model needs a `Dso::ModelSpec` (files: modelspecification.h/cpp), which
describes what specific Hantek protocol commands are to be used and what capabilities are supported. All known
models are specified in the subdirectory `models`. A new model inherits from `DSOModel` and implements
`applyRequirements(DsoCommandQueue)`. Within this method all necessary commands are registered via
DsoCommandQueue::addCommand. In the constructor of the model, the field `specification` is used to describe the models
capabilites.

# Namespace
Relevant classes in here are in the `DSO` namespace.

# Dependency
* Files in this directory depend on structs in the `usb` folder.
* Files in this directory depend on structs in the `hantekprotocol` folder.
