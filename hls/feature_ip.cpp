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

// prototype without AXI4 implementation
void feature_ip(int16_t sample_in, bool sample_valid, FeaturePacket& packet_out, bool& packet_valid) {
    static uint64_t accumulated_energy = 0;
    static int accumulated_zcr = 0;
    static int frame_id = 0;
    static int sample_counter = 0;
    static int16_t previous_sample = 0;
    static bool have_prev = false;

    // increases critical path time
    // see results.md for details
    // #pragma HLS PIPELINE II=1

    packet_valid = false;

    if (!sample_valid) return;

    accumulated_energy += extract_energy(sample_in);

    if (have_prev) {
        if (detect_zcr(previous_sample, sample_in)) {
            accumulated_zcr++;
        }
    }

    previous_sample = sample_in;
    have_prev = true;
    sample_counter++;

    if (sample_counter == 32) {
        packet_out.frame_id = frame_id;
        packet_out.energy = accumulated_energy;
        packet_out.zcr = accumulated_zcr;
        packet_valid = true;

        accumulated_energy = 0;
        accumulated_zcr = 0;
        have_prev = false;
        sample_counter = 0;
        frame_id++;
    }
}

int64_t extract_energy(int16_t sample) {
    int64_t s = sample;
    return s * s;
}

bool detect_zcr(int16_t previous_sample, int16_t current_sample) {
    if ( (previous_sample >= 0 && current_sample < 0) || 
         (previous_sample < 0 && current_sample >= 0) ) {
            return true;
         }
    return false;
}
