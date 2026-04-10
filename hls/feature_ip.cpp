/*
 * author: Jason Wehran
*/

#include "feature_ip.hpp"

/*
 * Process by sample
 * Once we get 32 samples, we package into frame with:
 *  - frame id
 *  - accumulated values 
*/

/*
 *                AXI4-Stream input  -->  feature_ip(...) internal logic  --> AXI4-Stream output
 *                                                   ^
 *                                                   |
 *                                           AXI4-Lite control
*/

// prototype without AXI4 implementation
void feature_ip(int16_t sample_in, bool sample_valid, FeaturePacket& packet_out, bool& packet_valid) {
    
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
