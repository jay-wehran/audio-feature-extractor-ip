/*
 * author: Jason Wehran
 * Testbench for feature_ip — AXI4-Stream interface
*/

#include "feature_ip.hpp"
#include <cstdio>
#include <cmath>
#include <vector>
#include <string>

static const int FRAME_LENGTH = 32;

static const std::vector<int16_t> RANDOM_FRAME = {
    -186, 319, 431, -700, 585, 709, -124, 951,
    -758, 314, 604, 147, -595, 840, -569, 481,
    -595, 63, 522, -30, -543, 906, 430, -417,
     380, -746, 840, 68, -220, 780, -235, 339
};

std::vector<int16_t> gen_all_zeros()    { return std::vector<int16_t>(FRAME_LENGTH, 0); }
std::vector<int16_t> gen_all_positive() { return std::vector<int16_t>(FRAME_LENGTH, 1000); }

std::vector<int16_t> gen_alternating() {
    std::vector<int16_t> frame(FRAME_LENGTH);
    for (int i = 0; i < FRAME_LENGTH; i++)
        frame[i] = (i % 2 == 0) ? 1000 : -1000;
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

struct Expected {
    uint64_t energy;
    int      zcr;
};

Expected compute_expected(const std::vector<int16_t>& frame) {
    uint64_t energy = 0;
    for (int i = 0; i < FRAME_LENGTH; i++)
        energy += (int64_t)frame[i] * frame[i];

    int zcr = 0;
    for (int i = 1; i < FRAME_LENGTH; i++) {
        if ((frame[i-1] >= 0 && frame[i] < 0) ||
            (frame[i-1] <  0 && frame[i] >= 0))
            zcr++;
    }
    return { energy, zcr };
}

void run_test(const std::string& name,
              const std::vector<int16_t>& frame,
              const Expected& expected,
              int& pass_count,
              int& fail_count) {

    hls::stream<SamplePacket>  s_axis;
    hls::stream<FeaturePacket> m_axis;

    // push all samples into the input stream
    for (int i = 0; i < FRAME_LENGTH; i++) {
        SamplePacket pkt;
        pkt.data = frame[i];
        pkt.last = (i == FRAME_LENGTH - 1);
        s_axis.write(pkt);
    }

    // call feature_ip once per sample
    for (int i = 0; i < FRAME_LENGTH; i++) {
        feature_ip(s_axis, m_axis);
    }

    // read output
    bool output_valid = !m_axis.empty();
    FeaturePacket result;
    if (output_valid) result = m_axis.read();

    bool energy_ok = output_valid && (result.energy == expected.energy);
    bool zcr_ok    = output_valid && (result.zcr    == expected.zcr);
    bool pass      = energy_ok && zcr_ok && output_valid;

    printf("--------------------------------------------------\n");
    printf("TEST: %s\n", name.c_str());
    printf("  Energy   expected=%lu  got=%lu  %s\n",
           expected.energy, output_valid ? result.energy : 0UL,
           energy_ok ? "OK" : "FAIL");
    printf("  ZCR      expected=%d   got=%d   %s\n",
           expected.zcr, output_valid ? result.zcr : -1,
           zcr_ok ? "OK" : "FAIL");
    printf("  packet_valid: %s\n", output_valid ? "OK" : "FAIL");
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