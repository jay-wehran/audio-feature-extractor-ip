/*
 * author: Jason Wehran
*/
#include "feature_ip.hpp"

/*
 *                AXI4-Stream input  -->  feature_ip(...) internal logic  --> AXI4-Stream output
 *                                                   ^
 *                                                   |
 *                                           AXI4-Lite control
*/

void feature_ip(
    hls::stream<SamplePacket>& s_axis,
    hls::stream<FeaturePacket>& m_axis
) {
    #pragma HLS INTERFACE axis port=s_axis
    #pragma HLS INTERFACE axis port=m_axis
    #pragma HLS INTERFACE ap_ctrl_none port=return

    static uint64_t  accumulated_energy = 0;
    static int       accumulated_zcr    = 0;
    static int       frame_id           = 0;
    static int       sample_counter     = 0;
    static ap_int<16> previous_sample   = 0;
    static bool      have_prev          = false;

    // read one sample from the AXI4-Stream input
    if (s_axis.empty()) return;

    SamplePacket in_pkt = s_axis.read();
    ap_int<16> sample_in = in_pkt.data;

    // --- energy accumulation ---
    accumulated_energy += extract_energy(sample_in);

    // --- ZCR detection ---
    if (have_prev) {
        if (detect_zcr(previous_sample, sample_in)) {
            accumulated_zcr++;
        }
    }

    previous_sample = sample_in;
    have_prev = true;
    sample_counter++;

    // --- frame boundary ---
    if (sample_counter == 32) {
        FeaturePacket out_pkt;
        out_pkt.frame_id = frame_id;
        out_pkt.energy   = accumulated_energy;
        out_pkt.zcr      = accumulated_zcr;
        m_axis.write(out_pkt);

        accumulated_energy = 0;
        accumulated_zcr    = 0;
        have_prev          = false;
        sample_counter     = 0;
        frame_id++;
    }
}

int64_t extract_energy(ap_int<16> sample) {
    int64_t s = sample;
    return s * s;
}

bool detect_zcr(ap_int<16> previous_sample, ap_int<16> current_sample) {
    if ((previous_sample >= 0 && current_sample < 0) ||
        (previous_sample <  0 && current_sample >= 0)) {
        return true;
    }
    return false;
}