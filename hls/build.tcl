# build.tcl
# Vitis HLS build script for audio feature extractor IP
# Runs C simulation and synthesis for the feature_ip top-level function
# Target part: Pynq-Z2 (xc7z020clg400-1)
# Clock: 250 MHz (4ns period)

open_project feature_ip_hls -reset
set_top feature_ip

add_files -cflags "-I." feature_ip.cpp
add_files -tb tb_feature_ip.cpp

open_solution "solution1" -reset
set_part {xc7z020clg400-1}
create_clock -period 4 -name default

csim_design
csynth_design