# CXX      = g++
# CXXFLAGS = -std=c++14 -Wall -Wextra
# TARGET   = feature_ip_tb
# SRCS     = tb_feature_ip.cpp feature_ip.cpp

# $(TARGET): $(SRCS) feature_ip.hpp
# 	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

# run: $(TARGET)
# 	./$(TARGET)

# clean:
# 	rm -f $(TARGET)


# Makefile for audio feature extractor IP
# Note: this project uses Vitis HLS-specific headers (hls::stream, ap_int)
# and cannot be compiled with plain g++.
# Use the Vitis HLS TCL script for C simulation and synthesis.

sim:
	vitis_hls -f build.tcl

clean:
	rm -rf feature_ip_hls