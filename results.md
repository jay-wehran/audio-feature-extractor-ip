# Verification and Synthesis Results

## C Simulation

The testbench was run via Vitis HLS `csim_design` using the Vitis HLS compiler,
which supports the `hls::stream` and `ap_int` types used in the AXI4-Stream
implementation. All five golden model test cases pass with exact match on
energy and ZCR.

**Vitis HLS CSim output:**

```
==================================================
feature_ip testbench
TEST: all_zeros
Energy   expected=0  got=0  OK
ZCR      expected=0   got=0   OK
packet_valid: OK
RESULT: PASS
TEST: all_positive
Energy   expected=32000000  got=32000000  OK
ZCR      expected=0   got=0   OK
packet_valid: OK
RESULT: PASS
TEST: alternating_pos_neg
Energy   expected=32000000  got=32000000  OK
ZCR      expected=31   got=31   OK
packet_valid: OK
RESULT: PASS
TEST: sine_wave
Energy   expected=16002512  got=16002512  OK
ZCR      expected=3   got=3   OK
packet_valid: OK
RESULT: PASS
TEST: random_noise
Energy   expected=9595071  got=9595071  OK
ZCR      expected=23   got=23   OK
packet_valid: OK
RESULT: PASS
Results: 5 / 5 passed
INFO: [SIM 211-1] CSim done with 0 errors.
```
---

## Synthesis Results

**Tool:** Vitis HLS 2023.2
**Target part:** xc7z020-clg400-1 (Pynq-Z2, Zynq-7000)
**Target clock:** 4.00 ns (250 MHz)

### Timing

| Clock  | Target  | Estimated | Fmax        |
|--------|---------|-----------|-------------|
| ap_clk | 4.00 ns | 2.871 ns  | 348.31 MHz  |

The design meets timing with significant margin — estimated Fmax of 348 MHz
exceeds the 250 MHz target by nearly 40%.

### Latency

| Latency Min (cycles) | Latency Max (cycles) | Latency Min (abs) | Latency Max (abs) | Interval Min | Interval Max |
|----------------------|----------------------|-------------------|-------------------|--------------|--------------|
| 1                    | 8                    | 4.000 ns          | 32.000 ns         | 2            | 9            |

Latency is measured per call to `feature_ip()` (i.e., per sample). The variation
reflects whether the call triggers a frame boundary (more logic) or is a mid-frame
sample (minimal logic). One output record is produced every 32 samples.

### Resource Utilization

| Resource | Used | Available | Utilization |
|----------|------|-----------|-------------|
| BRAM_18K | 0    | 280       | 0%          |
| DSP      | 1    | 220       | ~0%         |
| FF       | 840  | 106,400   | ~0%         |
| LUT      | 423  | 53,200    | ~0%         |
| URAM     | 0    | 0         | 0%          |

The design is extremely lightweight. The single DSP block is used for the
16-bit sample squaring operation (`mul_16s_16s_32_4_1`). The 840 flip-flops
primarily hold the static state registers: 64-bit energy accumulator,
32-bit ZCR counter, 32-bit frame ID, 32-bit sample counter, 16-bit
previous sample, and associated pipeline registers. No block RAM is needed
since both energy and ZCR are computed as streaming reductions requiring
no frame buffering.

---

### Interface

The IP uses proper AXI4-Stream interfaces declared via `#pragma HLS INTERFACE axis`
in `feature_ip.cpp`. Vitis HLS generates the full AXI4-Stream handshake signals
(TDATA, TVALID, TREADY) for both input and output ports:

| RTL Port       | Dir | Bits | Protocol     | Description                         |
|----------------|-----|------|--------------|-------------------------------------|
| ap_clk         | in  | 1    | ap_ctrl_none | Clock                               |
| ap_rst_n       | in  | 1    | ap_ctrl_none | Active-low synchronous reset        |
| s_axis_TDATA   | in  | 32   | axis         | AXI4-Stream input sample data       |
| s_axis_TVALID  | in  | 1    | axis         | AXI4-Stream input valid             |
| s_axis_TREADY  | out | 1    | axis         | AXI4-Stream input ready             |
| m_axis_TDATA   | out | 192  | axis         | AXI4-Stream output feature packet   |
| m_axis_TVALID  | out | 1    | axis         | AXI4-Stream output valid            |
| m_axis_TREADY  | in  | 1    | axis         | AXI4-Stream output ready            |

The full AXI4-Stream handshake is implemented — `s_axis_TREADY` is generated
by the IP to signal readiness to accept samples, and `m_axis_TVALID` is
asserted when a complete feature packet is available. `ap_ctrl_none` removes
the block-level handshake since the function operates continuously
sample-by-sample.

---

## HLS Optimization Pragmas

The following optimization pragmas are applied in `feature_ip.cpp`:

**`#pragma HLS INLINE`** is applied to both `extract_energy()` and `detect_zcr()`
to eliminate function call overhead and enable cross-boundary optimization of
the energy and ZCR datapaths. Inlining allows Vitis HLS to schedule the squaring
operation and sign-comparison logic directly into the parent function's pipeline
stages rather than treating them as separate sub-functions.

**`#pragma HLS INTERFACE axis`** is applied to both stream ports to generate
full AXI4-Stream RTL with TDATA, TVALID, and TREADY signals.

**`#pragma HLS INTERFACE ap_ctrl_none`** removes the block-level handshake
since the function is designed for continuous sample-by-sample invocation.

---

## Pipelining Experiment

`#pragma HLS PIPELINE II=1` was evaluated to determine the throughput
impact of pipelining the top-level function. The pragma successfully
achieved II=1 — meaning the IP could accept one new sample per clock
cycle. However, this introduced a timing tradeoff:

| Configuration | Fmax | II | Timing |
|---------------|------|----|--------|
| No pragma (final) | 348.31 MHz | 2-9 cycles | ✅ Meets target |
| `#pragma HLS PIPELINE II=1` | 146.81 MHz | 1 cycle | ❌ Exceeds 4 ns budget |

With the pipeline pragma, the critical path increased to 6.811 ns,
exceeding the 4 ns clock target. The critical path ran through the
sample counter increment, frame boundary comparison, and static variable
writeback.

For this application, II=1 throughput is unnecessary. At a typical audio
sample rate of 48 kHz, even the unpipelined design at 348 MHz processes
samples orders of magnitude faster than they arrive — approximately 7,250
clock cycles are available per audio sample. The pragma was therefore
permanently removed from the final design, preserving timing margin while
meeting all throughput requirements.

---

## Analysis Against Design Goals

| Goal | Target | Achieved |
|------|--------|----------|
| Functional correctness | Exact match with Python golden model | ✅ 5/5 test cases pass |
| Clock frequency | 250 MHz | ✅ 348 MHz estimated Fmax |
| AXI4-Stream interface | Full TDATA/TVALID/TREADY handshake | ✅ Synthesized via `#pragma HLS INTERFACE axis` |
| No frame buffering | Streaming reduction | ✅ 0 BRAM used |
| Lightweight resource usage | Minimal LUT/FF | ✅ <1% utilization on all resources |
| One output per frame | AXI4-Stream output valid | ✅ Verified in testbench |
| Helper function optimization | Eliminate call overhead | ✅ `#pragma HLS INLINE` on `extract_energy()` and `detect_zcr()` |
| Throughput | Sufficient for audio rate inputs | ✅ ~7,250 cycles available per sample at 48 kHz |