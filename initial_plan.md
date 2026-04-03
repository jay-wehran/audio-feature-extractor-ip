# Initial Project Plan: Streaming Audio Feature Extractor Vitis IP

## 1. GitHub Repo
Repository URL: https://github.com/jay-wehran/audio-feature-extractor-ip#

This repository will contain the initial plan, detailed plan, Python golden model, HLS source files, testbenches, and synthesis/simulation results.

## 2. Project Team
- Jason Wehran

## 3. IP Definition
This project designs a custom Vitis IP for streaming audio feature extraction. The IP will receive signed PCM audio samples and compute frame-based features for each block of audio. The initial minimum viable version will compute:
- short-time energy
- zero-crossing count

The intended use case is a lightweight hardware front-end for audio activity detection or embedded audio preprocessing.

### Mathematical operations
For an input frame x[n] of length N:

Energy:
E = sum from n=0 to N-1 of x[n]^2

Zero-crossing count:
Z = number of indices n where the sign of x[n] differs from x[n-1]

These operations are well-suited to hardware because they are repetitive, stream-based, and can be pipelined.

## 4. IP Architecture
The IP will use AXI4-Stream for sample input and AXI-Lite for control/status.

The architecture will contain the following modules:
- AXI4-Stream input interface
- frame buffer / frame counter
- energy computation module
- zero-crossing computation module
- output feature packet formatter
- AXI-Lite control/status registers

### Module interaction
Incoming audio samples arrive through AXI4-Stream. Samples are accumulated into frames. For each frame, the energy and zero-crossing modules compute their outputs. The output packet formatter combines the per-frame results and sends them to the host. AXI-Lite registers provide configuration such as frame length, enable bits, and status.

## 5. Minimum Viable Project Scope
- AXI4-Stream input
- frame buffer
- energy
- ZCR
- AXI-Lite control
- output feature packet
- Python golden model
- HLS simulation + synthesis results