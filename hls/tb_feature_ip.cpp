/*
 * author: Jason Wehran
*/

#include "feature_ip.hpp"
#include <cstdio>
#include <cmath>
#include <vector>
#include <string>

static const int FRAME_LENGTH = 32;
static const int RNG_SEED = 12345;

// --- test matrices ---

// random noise samples taken from tes_vectors.json
// no way to easily reporoduce the Mersenne Twister RNG algo
// in C++
static const std::vector<int16_t> RANDOM_FRAME = {
      -147, 500, -980, 679, 690, 644, -389, 751,
      -246, 908, -604, -447, 159, -107, -669, -236,
      -746, 790, -114, -466, 151, 284, -643, 252,
      132,-618, -274, 511, 962, 500, -814, 87
};

std::vector<int16_t> gen_all_zeros() {
    return std::vector<int16_t>(FRAME_LENGTH, 0);
}

std::vector<int16_t> gen_all_positive() {
    return std::vector<int16_t>(FRAME_LENGTH, 1000);
}

std::vector<int16_t> gen_alternating() {
    std::vector<int16_t> frame(FRAME_LENGTH);
    for (int i = 0; i < FRAME_LENGTH; i++) {
        frame[i] = (i % 2 == 0) ? 1000 : -1000;
    }
    return frame;
}

std::vector<int16_t> gen_sine_wave() {
    std::vector<int16_t> frame(FRAME_LENGTH);
    for (int i = 0; i < FRAME_LENGTH; i++) {
        double value = 1000.0 * sin(2.0 * M_PI * 2.0 * i / FRAME_LENGTH);
        frame[i] = (int16_t)round(value);
    }
    return frame;
}

std::vector<int16_t> gen_random_noise() {
    return std::vector<int16_t>(RANDOM_FRAME.begin(), RANDOM_FRAME.end());
}

// --- test bench ---

struct Expected{
    uint64_t energy;
    int zcr;
};

Expected compute_expected(const std::vector<int16_t>& frame) {
    uint64_t energy = 0;

    for (int i = 0; i < FRAME_LENGTH; i++) {
        energy += (int64_t)frame[i] * frame[i];
    }

    int zcr = 0;
    for (int i = 1; i < FRAME_LENGTH; i++) {
        if ((frame[i-1] >= 0 && frame[i] < 0) ||
            (frame[i-1] < 0 && frame[i] >= 0)) {
                zcr++;
            }
        }
    return { energy, zcr }; 
}

void run_test(const std::string& name,
              const std::vector<int16_t>& frame,
              const Expected& expected,
              int& pass_count,
              int& fail_count) 
    {
        FeaturePacket packet_out;
        bool packet_valid = false;

        for (int i = 0; i < FRAME_LENGTH; i++) {
            feature_ip(frame[i], true, packet_out, packet_valid);
        }

        bool energy_ok = (packet_out.energy == expected.energy);
        bool zcr_ok = (packet_out.zcr == expected.zcr);
        bool valid_ok = packet_valid;
        bool pass = energy_ok && zcr_ok && valid_ok;

        printf("--------------------------------------------------\n");
        printf("TEST: %s\n", name.c_str());
        printf("  Energy   expected=%lu  got=%lu  %s\n",
               expected.energy, packet_out.energy, energy_ok ? "OK" : "FAIL");
        printf("  ZCR      expected=%d   got=%d   %s\n",
               expected.zcr, packet_out.zcr, zcr_ok ? "OK" : "FAIL");
        printf("  packet_valid: %s\n", valid_ok ? "OK" : "FAIL");
        printf("  RESULT: %s\n", pass ? "PASS" : "FAIL");
        
        pass ? pass_count++ : fail_count++;
    }

int main() {
    int pass_count = 0;
    int fail_count = 0;

    struct TestCase {
        std::string          name;
        std::vector<int16_t> frame;
    };

    std::vector<TestCase> tests = {
        { "all_zeros",           gen_all_zeros()    },
        { "all_positive",        gen_all_positive() },
        { "alternating_pos_neg", gen_alternating()  },
        { "sine_wave",           gen_sine_wave()    },
        { "random_noise",        gen_random_noise() },
    };

    printf("==================================================\n");
    printf("  feature_ip testbench\n");
    printf("==================================================\n");

    for (auto& tc : tests) {
        Expected expected = compute_expected(tc.frame);
        run_test(tc.name, tc.frame, expected, pass_count, fail_count);
    }

    printf("==================================================\n");
    printf("  Results: %d / %d passed\n", pass_count, pass_count + fail_count);
    printf("==================================================\n");

    return (fail_count == 0) ? 0 : 1;
}