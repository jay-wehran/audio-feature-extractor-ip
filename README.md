# Audio Feature Extractor IP

A streaming fixed-point audio feature extraction IP implemented in Vitis HLS.
The IP accepts signed 16-bit PCM audio samples and computes frame-based features
for use in embedded audio analysis pipelines such as voice activity detection.

## Features

- **Short-Time Energy**: sum of squared samples per frame
- **Zero-Crossing Rate (ZCR)**: count of sign changes between adjacent samples
- **Frame length**: 32 samples
- **Input sample width**: signed 16-bit integer (int16_t)
- **Output per frame**: frame ID, energy (64-bit), ZCR (16-bit)

---

## Repository Structure

```
audio-feature-extractor-ip/
├── hls/
│   ├── feature_ip.hpp        # IP header: FeaturePacket struct, function prototypes
│   ├── feature_ip.cpp        # HLS implementation: feature_ip(), extract_energy(), detect_zcr()
│   ├── tb_feature_ip.cpp     # C++ testbench: 5 test cases with PASS/FAIL output
│   ├── build.tcl             # Vitis HLS TCL script: runs csim + csynth
│   └── Makefile              # Runs Vitis HLS csim and csynth via build.tcl
├── python/
│   ├── golden_model.py       # Python reference model: generates test vectors
│   ├── test_vectors.json     # Generated test vectors (JSON)
│   └── test_vectors.txt      # Generated test vectors (plain text)
├── detailed_plan.md          # Full module-level architecture and interface specification
├── initial_plan.md           # Original project proposal
└── README.md                 # This file
```
---

## IP Interface

The top-level function uses AXI4-Stream ports implemented via `hls::stream`
and `#pragma HLS INTERFACE axis`:

```cpp
void feature_ip(
    hls::stream<SamplePacket>& s_axis,
    hls::stream<FeaturePacket>& m_axis
);
```

`SamplePacket` carries a 16-bit PCM sample and a `last` flag. `FeaturePacket`
carries `frame_id`, `energy` (64-bit), and `zcr`. One `FeaturePacket` is emitted
to `m_axis` after every 32 samples received on `s_axis`. See
[`detailed_plan.md`](detailed_plan.md) for full signal definitions and protocol.

---

## Architecture Overview

The IP is organized into six logical modules implemented as a single HLS function
with static state variables acting as registers:

- **AXI4-Stream Input**: valid/ready handshake, one sample per transfer
- **Frame Counter**: 6-bit sample counter, 32-bit frame ID, generates frame-done event
- **Energy Module**: 64-bit accumulator, square-and-accumulate per sample
- **ZCR Module**: sign comparison against previous sample, zero treated as non-negative
- **Output Formatter**: packages frame ID + energy + ZCR into one result record per frame
- **AXI-Lite Control/Status**: register map planned; baseline uses fixed parameters

For full architecture documentation including pipelining strategy and data widths,
see [`detailed_plan.md`](detailed_plan.md).

---

## Python Golden Model

The golden model generates deterministic test vectors for five scenarios and
exports them to `python/test_vectors.json` and `python/test_vectors.txt`.

```bash
cd python
python3 golden_model.py
```

The golden model defines the reference implementation for both energy and ZCR,
and serves as the source of truth for testbench verification.

---

## Building and Running (Vitis HLS)

This project uses Vitis HLS-specific types (`hls::stream`, `ap_int`) and
requires Vitis HLS for both C simulation and synthesis. Plain g++ compilation
is not supported.

To run C simulation and synthesis:

```bash
cd hls
vitis_hls -f build.tcl
```

Or from within the Vitis HLS interactive prompt:

```tcl
source build.tcl
```

The TCL script automatically:
1. Creates the HLS project
2. Adds source and testbench files
3. Runs C simulation (`csim_design`) — compiles and runs the testbench
4. Runs synthesis (`csynth_design`) — generates RTL and reports

Target part: `xc7z020clg400-1` (Pynq-Z2), Clock: 250 MHz (4 ns period).

## Verification and Results

See [`results.md`](results.md) for:
- C simulation output (5/5 passing)
- Synthesis timing report
- Resource utilization table
- Performance analysis