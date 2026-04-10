/*
 * author: Jason Wehran
*/

#ifndef FEATURE_IP_HPP
#define FEATURE_IP_HPP

#include <cstdint>
#include <cstdio>
#include <vector>


struct FeaturePacket {
    int frame_id;
    uint64_t energy;
    int zcr;
};

void feature_ip(int16_t sample_in, 
                bool sample_valid, 
                FeaturePacket& packet_out, 
                bool& packet_valid);

int64_t extract_energy(int16_t sample);

bool detect_zcr(int16_t previous_sample, int16_t current_sample);



#endif // FEATURE_IP_HPP