# Detailed Project Plan: Streaming Audio Feature Extractor Vitis IP

## 0. Project Summary

This project implements a custom Vitis HLS IP block for streaming audio feature extraction. The IP accepts signed PCM audio samples through a streaming interface and computes frame-based features. The baseline implementation computes:

- short-time energy
- zero-crossing count (ZCR)

The system is intended as a lightweight audio-analysis front end for embedded systems, such as audio activity detection or preprocessing before later DSP stages.

The design will be completed in simulation and synthesis without using a physical FPGA board.

### Baseline configuration
- Input sample type: signed 16-bit integer
- Frame length: 32 samples for initial verification
- Features: energy and ZCR
- Output per frame: frame ID, energy, ZCR

A fixed frame length is used initially to simplify verification and reduce implementation risk.

---

## 1. Module Definitions

The IP is organized into six logical modules. Even if the first HLS implementation combines several of these into one top-level function, the design is planned and verified using this modular structure.

### 1.1 AXI4-Stream Input Module

#### Function
Receives one signed audio sample at a time from an upstream source and forwards accepted samples into the internal datapath.

#### Inputs
- `s_axis_tdata[15:0]` : signed input sample
- `s_axis_tvalid` : input-valid signal
- `ap_clk` : clock
- `ap_rst` : reset

#### Outputs
- `s_axis_tready` : ready-to-accept signal
- `sample_out[15:0]` : accepted sample forwarded internally
- `sample_valid` : indicates an internal sample transfer occurred this cycle

#### Message/data format
- One AXI stream transaction carries one signed 16-bit PCM sample.
- Samples are interpreted as two's-complement signed integers.

#### Control signals
- Standard AXI4-Stream valid/ready handshake
- A sample is accepted when `s_axis_tvalid && s_axis_tready`

#### Timing/sequencing assumptions
- Internal processing occurs only on accepted samples.
- No feature state is updated unless the handshake succeeds.
- One sample is processed per successful stream transfer.

---

### 1.2 Frame Counter / Frame Control Module

#### Function
Tracks the current position within a frame and generates end-of-frame control events.

#### Inputs
- `sample_valid`
- `ap_clk`
- `ap_rst`

#### Outputs
- `sample_index[5:0]` : current sample position within the frame
- `frame_done` : asserted when the final sample of the frame has been processed
- `frame_id[31:0]` : current frame number

#### Data widths
- Frame length in the baseline design is 32, so a 6-bit sample counter is sufficient.
- Frame ID uses 32 bits for simplicity.

#### Control signals
- `frame_done` is a one-cycle event indicating that the current frame is complete.

#### Timing/sequencing assumptions
- The counter increments only when `sample_valid` is asserted.
- The frame begins at sample index 0.
- After the 32nd valid sample is processed, `frame_done` is asserted and the module resets its per-frame count for the next frame.

#### Design note
A full frame memory is not required for the baseline design because both energy and ZCR can be computed as streaming reductions.

---

### 1.3 Energy Computation Module

#### Function
Computes short-time energy for the current frame.

#### Mathematical definition
For frame $x[n]$, $n = 0, \dots, N-1$:

$E = \sum_{n=0}^{N-1} x[n]^2$

#### Inputs
- `sample_in[15:0]`
- `sample_valid`
- `frame_done`
- `ap_clk`
- `ap_rst`

#### Outputs
- `energy_out[63:0]` : completed energy value for the frame
- `energy_valid` : indicates `energy_out` is valid at end-of-frame

#### Data widths
- Input sample: signed 16-bit
- Squared sample: up to 31 bits effectively needed for magnitude
- Accumulator: 64 bits chosen for safety and simplicity

#### Control signals
- `sample_valid` causes square-and-accumulate
- `frame_done` causes the current accumulated energy to be captured for output

#### Timing/sequencing assumptions
- The accumulator resets at the start of a frame or immediately after finalizing the previous frame.
- Each accepted sample contributes exactly once to the accumulator.
- `energy_valid` is asserted when the frame result is ready.

---

### 1.4 Zero-Crossing Count Module

#### Function
Counts sign changes between adjacent samples within a frame.

#### Mathematical definition
$Z = \sum_{n=1}^{N-1} \mathbf{1}\{(x[n-1] \ge 0 \land x[n] < 0)\lor(x[n-1] < 0 \land x[n] \ge 0)\}$

#### Inputs
- `sample_in[15:0]`
- `sample_valid`
- `frame_done`
- `ap_clk`
- `ap_rst`

#### Outputs
- `zcr_out[15:0]` : zero-crossing count for the frame
- `zcr_valid` : indicates `zcr_out` is valid at end-of-frame

#### Data widths
- For a frame length of 32, the maximum ZCR is 31.
- A 16-bit output is more than sufficient and leaves room for later expansion to larger frames.

#### Control/state
- `prev_sample[15:0]` : previous accepted sample
- `have_prev_sample` : indicates whether a previous sample exists within the current frame
- `zcr_count[15:0]` : running crossing counter

#### Zero-crossing rule
A crossing is counted if:
- `prev_sample >= 0` and `sample_in < 0`, or
- `prev_sample < 0` and `sample_in >= 0`

Zero is treated as non-negative. This convention must match the Python golden model exactly.

#### Timing/sequencing assumptions
- The first sample in a frame does not generate a crossing.
- Comparison begins on the second accepted sample of the frame.
- The running count resets at the start of each frame.

---

### 1.5 Output Formatter Module

#### Function
Packages the completed feature values into a single output record per frame.

#### Inputs
- `frame_id[31:0]`
- `energy_out[63:0]`
- `zcr_out[15:0]`
- `frame_done`
- `ap_clk`
- `ap_rst`

#### Outputs
- `result_frame_id[31:0]`
- `result_energy[63:0]`
- `result_zcr[15:0]`
- `result_valid`

#### Message format
One output record per completed frame:

- `frame_id` : 32 bits
- `energy` : 64 bits
- `zcr` : 16 bits

For simulation, this may be represented as an HLS struct. In a more complete system, this could map to an AXI4-Stream output packet.

#### Timing/sequencing assumptions
- `result_valid` is asserted when the final sample of a frame has been processed and both feature values are ready.
- Exactly one output record is produced per frame.

---

### 1.6 AXI-Lite Control/Status Module

#### Function
Provides software-visible configuration and status information for the IP.

#### Inputs
- AXI-Lite write/read transactions
- `ap_clk`
- `ap_rst`

#### Outputs
- configuration register values to the datapath
- status register values readable by software

#### Planned register fields
Baseline planned fields:
- `frame_length`
- `enable_energy`
- `enable_zcr`
- `soft_reset`
- `processed_frame_count`
- `last_energy`
- `last_zcr`
- `result_valid`

#### Data widths
- `frame_length` : 16 bits
- `enable_*` : 1 bit each
- `processed_frame_count` : 32 bits
- `last_energy` : 64 bits
- `last_zcr` : 16 bits

#### Timing/sequencing assumptions
- For the first implementation, some values may be fixed in code rather than fully runtime-configurable.
- AXI-Lite remains part of the architectural plan even if the initial HLS prototype hardcodes frame length.

---

## 2. Top-Level Dataflow and Sequencing

### 2.1 Per-sample behavior
For each accepted input sample:
1. The AXI4-Stream input module accepts the sample.
2. The frame control module increments the sample count.
3. The energy module squares and accumulates the sample.
4. The ZCR module compares the sample against the previous sample and updates the crossing count.
5. The current sample becomes the previous sample for the next cycle.

### 2.2 End-of-frame behavior
When the final sample in the frame is accepted:
1. `frame_done` is asserted.
2. The final energy value is captured.
3. The final ZCR value is captured.
4. The output formatter emits one feature record.
5. Per-frame state resets for the next frame.

### 2.3 Reset behavior
On reset:
- frame counter is cleared
- frame ID is reset or initialized
- energy accumulator is cleared
- ZCR counter is cleared
- previous-sample state is invalidated
- output valid is cleared

---

## 3. Testbench Definition

### 3.1 Verification strategy
The design will be verified using a Python golden model and an HLS C++ testbench.

The Python golden model defines the correct feature behavior for a set of deterministic input frames. The HLS testbench will feed the same frames into the HLS implementation and compare the resulting outputs against the Python-generated expected results.

### 3.2 Golden model
The golden model is implemented in Python.

It performs:
- frame generation
- energy computation
- ZCR computation
- export of expected results to versioned test-vector files

The golden model serves as the source of truth for functional correctness.

### 3.3 Test scenarios
The baseline test scenarios are:

1. **All zeros**
   - verifies zero energy
   - verifies zero ZCR

2. **All positive values**
   - verifies constant-signal energy
   - verifies zero ZCR

3. **Alternating positive/negative**
   - verifies high ZCR behavior
   - verifies energy accumulation under repeated sign flips

4. **Sine wave**
   - verifies realistic oscillatory behavior
   - checks expected ZCR under the selected zero-handling rule

5. **Random noise**
   - verifies general behavior on nontrivial data
   - helps catch edge-case logic bugs

Additional optional scenarios:
- all negative values
- clipped waveform
- frame boundary stress test using back-to-back frames

### 3.4 Functional correctness checks
For each test frame, the testbench will verify:
- exact match of computed energy
- exact match of computed ZCR
- exactly one output record per frame
- correct frame ID sequencing
- correct reset of accumulators and counters between frames

### 3.5 Module-level unit tests
Planned unit-style checks include:

#### AXI4-Stream input module
- confirm that samples are processed only when valid/ready handshake succeeds
- confirm no internal update occurs without accepted input

#### Frame control module
- confirm frame_done asserts after exactly 32 accepted samples
- confirm counter resets correctly for next frame

#### Energy module
- confirm sum of squares matches expected values on simple frames
- confirm accumulator resets between frames

#### ZCR module
- confirm crossing rule matches Python on hand-constructed cases
- confirm first sample does not generate a crossing
- confirm zero-handling convention is preserved

#### Output formatter
- confirm exactly one output record is emitted per completed frame
- confirm output fields contain the correct final results

### 3.6 Integration tests
After unit-level behavior is validated, full integration tests will:
- stream complete frames through the full top-level function
- verify output packets against Python reference data
- verify behavior for multiple frames in sequence

### 3.7 Performance and latency evaluation
The project will evaluate:
- functional latency from frame completion to output-valid assertion
- throughput in terms of one accepted sample per cycle where possible
- synthesis-reported resource usage:
  - LUTs
  - flip-flops
  - DSPs
  - BRAMs

Since the baseline design is frame-based, the main latency metric is:
- how soon after the last sample of a frame the result becomes available

The performance analysis will be based on Vitis HLS simulation/synthesis reports rather than physical board measurements.

---

## 4. Development Steps

The project will be implemented incrementally in small, testable milestones.

### Milestone 1: Finalize specification
Tasks:
- confirm fixed sample width
- confirm fixed frame length
- confirm output record structure
- confirm zero-crossing convention

Deliverable:
- updated planning documents and stable golden-model assumptions

### Milestone 2: Complete Python golden model
Tasks:
- generate deterministic test vectors
- compute expected energy and ZCR
- save test vectors in versioned files

Unit checks:
- manually inspect simple cases
- verify outputs for all-zero, all-positive, and alternating-sign frames

Deliverable:
- `python/golden_model.py`
- committed test-vector files

### Milestone 3: Implement energy path
Tasks:
- implement sample squaring
- implement running accumulator
- handle reset and frame-end capture

Unit checks:
- verify exact energy on hand-built frames
- verify reset behavior between frames

Deliverable:
- working energy-only HLS logic

### Milestone 4: Implement ZCR path
Tasks:
- implement previous-sample storage
- implement sign-comparison logic
- implement crossing counter and reset logic

Unit checks:
- verify crossing behavior on hand-built test frames
- verify exact agreement with Python sign convention

Deliverable:
- working ZCR-only HLS logic

### Milestone 5: Implement frame control
Tasks:
- implement sample counter
- generate frame_done event
- maintain frame ID

Unit checks:
- verify frame boundaries after exactly 32 accepted samples
- verify frame ID increments correctly

Deliverable:
- working frame controller

### Milestone 6: Integrate feature datapath
Tasks:
- combine energy, ZCR, and frame control
- ensure all blocks update only on accepted samples
- verify correct sequencing across back-to-back frames

Integration checks:
- compare top-level outputs to Python expected values for all test vectors

Deliverable:
- integrated top-level HLS feature extractor

### Milestone 7: Implement output formatting
Tasks:
- package frame ID, energy, and ZCR into a result record
- generate result-valid behavior

Unit/integration checks:
- verify one output per completed frame
- verify field correctness

Deliverable:
- complete functional HLS output path

### Milestone 8: Add control/status interface
Tasks:
- define planned AXI-Lite register map
- implement fixed or partially configurable control path as practical

Checks:
- verify configuration values are visible to the datapath
- verify status outputs are updated correctly

Deliverable:
- control/status interface or documented fixed-parameter equivalent

### Milestone 9: Run full HLS verification
Tasks:
- run C simulation
- resolve all mismatches against golden model
- run additional multi-frame tests

Deliverable:
- passing functional testbench

### Milestone 10: Run synthesis and evaluate results
Tasks:
- run Vitis HLS synthesis
- collect latency and resource reports
- document throughput and area results

Deliverable:
- synthesis summary and final analysis

---

## 5. Integration Plan

Integration will proceed in the following order:

1. golden model only (done)
2. energy module only
3. ZCR module only
4. frame control only
5. combine energy + ZCR + frame control
6. add output formatter
7. add control/status interface
8. run full-system verification
9. run synthesis and report results