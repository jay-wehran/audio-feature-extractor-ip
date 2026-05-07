/*
 * author: Jason Wehran
*/

#include "feature_ip.hpp"
#include <cstdio>

int main() {

    std::vector<int16_t> samples {
        0,1,2,3,4,5,6,7,8,9,
       10,11,12,13,14,15,16,17,18,19,
       20,21,22,23,24,25,26,27,28,29,
       30,31,32,33,34,35,36,37,38,39,
       40,41,42,43,44,45,46,47,48,49,
       50,51,52,53,54,55,56,57,58,59,
       60,61,62,63,64,65,66,67,69,69,
       70,71,72,73,74,75,76,77,78,79,
       80,81,82,83,84,85,86,87,88,89,
       90,91,92,93,94,95,96
    };

    std::vector<FeaturePacket> fp_collection;
    FeaturePacket fp;


    const int frame_length = 32;
    uint64_t accumulated_energy = 0;
    int accumulated_zcr = 0;
    int frame_id = 0;
    int16_t previous_sample = 0;
    bool have_prev = false;

    for (size_t i = 0; i < samples.size(); i++) {
        accumulated_energy += extract_energy(samples[i]);

        if (have_prev) {
            if (detect_zcr(previous_sample, samples[i])) {
                accumulated_zcr++;
            }
        }

        previous_sample = samples[i];
        have_prev = true;

        // check if we are on a 32nd sample (full frame size)
        // if so, pack all the information into a feature packet
        // and reset accumulators + state vars
        if ((i + 1) % frame_length == 0) {
            fp.frame_id = frame_id;
            fp.energy = accumulated_energy;
            fp.zcr = accumulated_zcr;
            fp_collection.push_back(fp);

            accumulated_energy = 0;
            accumulated_zcr = 0;
            have_prev = false;
            frame_id++;
        }
    }

    for (size_t i = 0; i < fp_collection.size(); i++) {
        printf("Feature Packet %ld ==> FRAME ID: %d Energy %lu ZCR: %d\n", 
                i,
                fp_collection[i].frame_id,
                fp_collection[i].energy,
                fp_collection[i].zcr);
    }

    return 0;
}