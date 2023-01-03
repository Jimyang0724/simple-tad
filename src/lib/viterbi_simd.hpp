#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
#include <simdpp/simd.h>

using namespace std;
using namespace simdpp;
#define PI acos(-1)

namespace vectorized {

float emission_func(const float* const& emission, const int& di, const int& state) {
    return emission[state * 3 + di];
}

int* viterbi(const int* const& observations, const std::size_t& num_observation, const float* const& initial, const float* const& transition, const float* const& emission) {
    int* hidden_states = new int[num_observation]();
    float* viterbi = new float[3* num_observation]();

    int* prev_state = new int[3* num_observation]();
    
    float* initial_log1p = new float[4]();
    for(std::size_t i = 0; i < 3; ++i) {
        initial_log1p[i] = std::log1p(initial[i]);
    }
    initial_log1p[3] = -10;

    float* transition_log1p = new float[4*4]();
    for(std::size_t i = 0; i < 4*4; ++i) {
        if ((i + 1) % 4 == 0 || i >= 12)
            transition_log1p[i] = -10;
        else
        {
            int origin_i = i - i / 4;
            origin_i = origin_i%3*3+origin_i/3;
            transition_log1p[i] = log1p(transition[origin_i]);
        }
    }

    float32<4>* simd_transition_log1p = new float32<4>[4]();
    for(std::size_t i = 0; i < 4; ++i) {
        simd_transition_log1p[i] = load(transition_log1p + i * 4);
    }
    
    float* emission_log1p = new float[3*num_observation]();
    for(std::size_t t = 0; t < num_observation; ++t) {
        for(std::size_t i = 0; i < 3; ++i) {
            emission_log1p[t*3 + i] = std::log1p(emission_func(emission, observations[t], i));
        }
    }

    float32<4>* simd_emission_log1p = new float32<4>[num_observation]();
    for(std::size_t t = 0; t < num_observation; ++t) {
        simd_emission_log1p[t] = load(emission_log1p + t*3);
    }

    // initialize viterbi
    float32<4> temp = load(initial_log1p);
    temp = add(temp, simd_emission_log1p[0]);
    store(viterbi + 0, temp);

    int index_ar[4] = { 0, 1, 2, 0 };
    int32<4> simd_index_mask = load(index_ar);

    // run viterbi
    for (std::size_t t = 1; t < num_observation; ++t) {
        for (std::size_t i = 0; i < 3; ++i) {   // current state
            float32<4> temp = load(viterbi + (t-1)*3);
            temp = add(temp, simd_transition_log1p[i]);

            float max = reduce_max(temp);
            viterbi[t * 3 + i] = max + std::log1p(emission_func(emission, observations[t], i));

            int32<4> temp2;
            temp2 = cmp_neq(temp, max);
            temp2 = add(temp2, 1);
            temp2 = mul_lo(temp2, simd_index_mask);
            prev_state[t * 3 + i] = reduce_add(temp2);
        }
    }

    // find the most probable state
    float max = -INFINITY;
    for (std::size_t i = 0; i < 3; ++i) {
        if (viterbi[(num_observation-1) * 3 + i] > max) {
            max = viterbi[(num_observation-1) * 3 + i];
            hidden_states[num_observation - 1] = i;
        }
    }

    // backtrace
    for (int t = num_observation - 2; t >= 0; --t) {
        hidden_states[t] = prev_state[(t + 1) * 3 + hidden_states[t + 1]];
    }

    delete[] viterbi;

    return hidden_states;
}
}