# Verification and Synthesis Results

## C Simulation

The testbench was run via both `g++` (local) and Vitis HLS `csim_design`.
All five golden model test cases pass with exact match on energy and ZCR.

**Vitis HLS CSim output:**

```
==================================================
  feature_ip testbench
==================================================
--------------------------------------------------
TEST: all_zeros
  Energy   expected=0  got=0  OK
  ZCR      expected=0   got=0   OK
  packet_valid: OK
  RESULT: PASS
--------------------------------------------------
TEST: all_positive
  Energy   expected=32000000  got=32000000  OK
  ZCR      expected=0   got=0   OK
  packet_valid: OK
  RESULT: PASS
--------------------------------------------------
TEST: alternating_pos_neg
  Energy   expected=32000000  got=32000000  OK
  ZCR      expected=31   got=31   OK
  packet_valid: OK
  RESULT: PASS
--------------------------------------------------
TEST: sine_wave
  Energy   expected=16002512  got=16002512  OK
  ZCR      expected=3   got=3   OK
  packet_valid: OK
  RESULT: PASS
--------------------------------------------------
TEST: random_noise
  Energy   expected=9861132  got=9861132  OK
  ZCR      expected=19   got=19   OK
  packet_valid: OK
  RESULT: PASS
==================================================
  Results: 5 / 5 passed
==================================================
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
| ap_clk | 4.00 ns | 2.638 ns  | 379.08 MHz  |

The design meets timing with significant margin — estimated Fmax of 379 MHz
exceeds the 250 MHz target by over 50%.

### Latency

| Latency Min (cycles) | Latency Max (cycles) | Latency Min (abs) | Latency Max (abs) | Interval Min | Interval Max |
|----------------------|----------------------|-------------------|-------------------|--------------|--------------|
| 1                    | 6                    | 4.000 ns          | 24.000 ns         | 2            | 7            |

Latency is measured per call to `feature_ip()` (i.e., per sample). The variation
reflects whether the call triggers a frame boundary (more logic) or is a mid-frame
sample (minimal logic). One output record is produced every 32 samples.

### Resource Utilization

| Resource | Used | Available | Utilization |
|----------|------|-----------|-------------|
| BRAM_18K | 0    | 280       | 0%          |
| DSP      | 1    | 220       | ~0%         |
| FF       | 839  | 106,400   | ~0%         |
| LUT      | 437  | 53,200    | ~0%         |
| URAM     | 0    | 0         | 0%          |

The design is extremely lightweight. The single DSP block is used for the
16-bit sample squaring operation (`mul_16s_16s_32_4_1`). The 839 flip-flops
primarily hold the static state registers: 64-bit energy accumulator,
32-bit ZCR counter, 32-bit frame ID, 32-bit sample counter, 16-bit
previous sample, and associated pipeline registers. No block RAM is needed
since both energy and ZCR are computed as streaming reductions requiring
no frame buffering.

---

## Analysis Against Design Goals

| Goal | Target | Achieved |
|------|--------|----------|
| Functional correctness | Exact match with Python golden model | ✅ 5/5 test cases pass |
| Clock frequency | 250 MHz | ✅ 379 MHz estimated Fmax |
| No frame buffering | Streaming reduction | ✅ 0 BRAM used |
| Lightweight resource usage | Minimal LUT/FF | ✅ <1% utilization on all resources |
| One output per frame | packet_valid handshake | ✅ Verified in testbench |