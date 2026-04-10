/*
 * author: Jason Wehran
*/

#include "feature_ip.hpp"

int main() {

    std::vector<int16_t> samples{0,1,2,3,4,5,6,7,8,9,
                                 10,11,12,13,14,15,16,17,18,19,
                                 20,21,22,23,24,25,26,27,28,29,
                                 30,31,32,33,34,35,36,37,38,39,
                                 40,41,42,43,44,45,46,47,48,49,
                                 50,51,52,53,54,55,56,57,58,59,
                                 60,61,62,63,64,65,66,67,69,69,
                                 70,71,72,73,74,75,76,77,78,79,
                                 80,81,82,83,84,85,86,87,88,89,
                                 90,91,92,93,94,95,96};

    std::vector<FeaturePacket> fp_collection;
    
    FeaturePacket fp;


    const int samples_per_frame = 32;
    int sample_counter = 0;
    uint64_t accumulated_energy = 0;
    int accumulated_zcr = 0;
    int frame_id = 0;
    bool frame_flag = false;
    int16_t previous_sample = 0;

    for (int i = 0; i < samples.size(); i++) {
        // process sample
        if (i != 0) {
            if (i % 31 == 0) {
                frame_flag = true;
            }
            accumulated_energy += extract_energy(samples[i]);
            if (detect_zcr(previous_sample, samples[i])) {
                accumulated_zcr++;
            }
            previous_sample = samples[i];
            
        } else {
            accumulated_energy += extract_energy(samples[i]);
            previous_sample = samples[i];

        }

        // package up data into a frame and reset state
        if (frame_flag) {
            ++frame_id;
            fp.frame_id = frame_id;
            fp.energy = accumulated_energy;
            fp.zcr = accumulated_zcr;

            fp_collection.push_back(fp);

            accumulated_energy = 0;
            accumulated_zcr = 0;
            frame_flag = false;
        }

    }

    for (int i = 0; i < fp_collection.size(); i++) {
        printf("Feature Packet %d ==> Frame ID: %d  Energy: %ld  ZCR: %d\n", i, 
                fp_collection[i].frame_id, fp_collection[i].energy, fp_collection[i].zcr );
    }
    


    return 0;
}