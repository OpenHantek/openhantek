DSO-2250 regression and manual hardware tests

Build the ordinary application and offline tests:
  cmake -S . -B build -DBUILD_TESTING=ON
  cmake --build build
  ctest --test-dir build --output-on-failure
Use the usual Qt5/libusb/FFTW dependency options from docs/build.md.

The dso2250_protocol executable links production controller/USB code against
fake libusb C functions, and does not link a physical USB implementation.
It verifies legacy and revision-1 selection, SDK packet bytes, rates, channels,
relay/DAC banks, capture duration, Gray ordering, rolling ordering, default
record restoration, a DSO-2090 regression, and guarded firmware preparation.
The post_pipeline test exercises queued Qt events and the actual processing
and graph generators. Hardware-triggered frames produce graph vertices in
normal, auto and single modes without a software-trigger flag.
These offline targets currently build on non-Windows platforms.

The optional physical probe is never registered with CTest:
  cmake -S . -B build -DBUILD_TESTING=ON -DBUILD_HARDWARE_TESTS=ON
  cmake --build build --target dso2250_hardware
  ./build/tests/dso2250_hardware report.json
Connect exactly one DSO-2250. Connect CH1 to its CAL output (2 Vpp/1 kHz),
ground to CAL GND, and set the probe to x10. Close other scope applications.
The probe automatically loads the corrected RAM firmware on a cold device,
then acquires 24 real short frames through the production controller and
its queued event path. It exits unsuccessfully on incomplete records or
CAL frequency/amplitude outside the stated tolerances.

Observed hardware results are summarized in dso2250-hardware-validation.json.
The raw factory firmware resource remains unchanged. Preparation checks its
full decoded identity before adding the 82-byte clock/speed correction to a
temporary upload file. There are no EEPROM or FPGA flash writes.

Manual GUI check:
Open the generated OpenHantek.app and choose DSO-2250. The record selector
must contain 10.24 kS on first startup. For CAL with a x10 probe, choose CH1
100 mV/div, 1 ms/div and Wait/Force triggering. To synchronize the CAL waveform,
move the red trigger-level arrow on the right edge to about 100 mV, then choose
Hard-/Software, source CH1 and rising slope. A 0 V threshold is below the CAL
low plateau and does not trigger reliably. GUI capture and this stable trigger
setting were confirmed manually on the physical device.
This version has no probe multiplier;
about 0.21 Vpp at the USB scope input corresponds to about 2.1 Vpp at the probe.
The program supports both legacy opcode01 and the revision-1 split protocol.
