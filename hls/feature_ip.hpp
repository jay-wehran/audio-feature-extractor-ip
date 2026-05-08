/*
 * author: Jason Wehran
*/
#ifndef FEATURE_IP_HPP
#define FEATURE_IP_HPP

#include <cstdint>
#include <cstdio>
#include <vector>
#include <ap_int.h>
#include <hls_stream.h>

struct SamplePacket {
    ap_int<16> data;
    bool last;
};

struct FeaturePacket {
    int frame_id;
    uint64_t energy;
    int zcr;
};

void feature_ip(
    hls::stream<SamplePacket>& s_axis,
    hls::stream<FeaturePacket>& m_axis
);

int64_t extract_energy(ap_int<16> sample);
bool detect_zcr(ap_int<16> previous_sample, ap_int<16> current_sample);

#endif // FEATURE_IP_HPP