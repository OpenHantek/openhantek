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
It also reproduces the GUI's connection probe followed by reconnection and
verifies USB reference ownership, repeated disconnects, unplug errors,
destruction of an open connection, and failed configuration/interface claims.
The original code fails this reference-lifetime regression.
Transient USB errors are also injected into pending configuration, the B2
handshake, and GETDATA. The actual controller timer must recover and publish
another complete record without a fatal communication signal. A real device
disconnection still stops acquisition. The original controller fails the
pending-command timeout regression.
Capture-state tests inject OUT, post-command B2 and IN failures, distinguish
timeouts from device removal, and reject truncated state/trigger headers.
They require an immediate nonfatal status message, preservation of the last
valid trigger pointer, and recovery through the real timer after 100 ms.
Device removal must signal a fatal communication error in the failed poll.
The original implementation fails this regression by swallowing the USB error
as a positive capture-state value. This fix does not identify or repair the
underlying cause of persistent USB timeouts on the physical board.
The post_pipeline test exercises queued Qt events and the actual processing
and graph generators. Hardware-triggered frames produce graph vertices in
normal, Wait/Force and single modes without a software-trigger flag. A constant
1.2 V input in Wait/Force at 1 V/div produces 10,240 finite graph vertices on
a horizontal line, covering a 12 V DC measurement through a x10 probe.
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

The successful 2026-10-14 hardware results are summarized in
dso2250-hardware-validation.json.
The native GUI was also opened, captured three nonempty CH1 graph frames and
closed normally in three consecutive runs (exit 0). Cancelling device selection
twice followed the existing return -1 path (shell exit 255) without crashing.
A fourth capture/close run passed after installing the corrected bundle in
/Applications; its executable matched the workspace bundle.
The raw factory firmware resource remains unchanged. Preparation checks its
full decoded identity before adding the 82-byte clock/speed correction to a
temporary upload file. There are no EEPROM or FPGA flash writes.

On 2026-10-18, the same device no longer delivered capture records with the
corrected production image. Cold initialization, the OEM upload sequence, and
the earlier 100 mV/div settings did not restore acquisition. Restarting the
resident program cleared the command queue, but only four OUT packets were
accepted before the fifth timed out; no state or ADC data was returned.
The cause remains unresolved. The timeout regression passes offline and the
updated GUI remained open for 45 seconds through repeated USB timeouts; these
checks do not establish successful physical acquisition on that date.

On 2026-10-20, short physical captures passed during a retest using the
normal corrected production RAM image and the current controller at a81d646.
After a cold USB power cycle, the production uploader completed normally; the
historical PyUSB capture procedure then received a complete 20,480-byte record
(10,240 samples per channel), measuring CAL at 1 kHz and 2.102 Vpp after the
physical x10 probe correction. No diagnostic firmware was used in this run.
The actual native controller subsequently captured 24 of 24 complete records
at 1 MS/s, each measuring 1 kHz and 2.102-2.133 Vpp after probe correction,
without USB errors. The installed native GUI then captured 1,413 nonempty CH1
graph frames over 45 seconds at 100 mV/div, 1 ms/div and Wait/Force, with
10,240 vertices per frame and no errors during that initial interval. The
same session later reached 3,023 frames, but its final observation at 144 s
had no new frame for 47.9 s and the log contained 81 capture-state timeouts.
The window remained responsive and the user closed it normally; this was
an acquisition freeze rather than an application crash. A subsequent native
cold-start launch uploaded the same production image but again reported
capture-state timeouts. A three-second process stack sample placed the
acquisition worker predominantly in bulkCommand()/bulkTransfer(), before
the capture-state IN read.
A warm reopen with the CAL controls applied still received no CH1 frame in
31 seconds and reported timeouts sending the pending configuration command.
A local Qt helper selected ordinary UI controls and observed processing
signals; the helper is not part of this PR.
These results establish successful short captures, not sustained stability
or the cause of the earlier failure. USB connection, cable,
probe attachment and host state changed during troubleshooting, so the
recovery cannot be attributed to a single port or cable change.

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
