#include "di.hpp"
#include "benchmark/benchmark.h"
#include <iostream>
#include <algorithm>

std::vector<float> calculate_di_SCALAR(const float* contact_matrix, const std::size_t& edge_size, const std::size_t& bin_size) {
    std::size_t range = SIGNIFICANT_BINS / bin_size;
    std::vector<float> di(edge_size, 0);

    for (std::size_t locus_index = 0; locus_index < edge_size; ++locus_index) {
        float A;
        float B;
        
        if (locus_index < range) {
            // edge case
            A = std::accumulate(contact_matrix + locus_index * edge_size, contact_matrix + locus_index * edge_size + locus_index, 0.0f);
            B = std::accumulate(contact_matrix + locus_index * edge_size + locus_index + 1, contact_matrix + locus_index * edge_size + locus_index + range + 1, 0.0f);
        } else if (locus_index >= edge_size - range) {
            // edge case
            A = std::accumulate(contact_matrix + locus_index * edge_size + locus_index - range, contact_matrix + locus_index * edge_size + locus_index, 0.0f);
            B = std::accumulate(contact_matrix + locus_index * edge_size + locus_index + 1, contact_matrix + (locus_index + 1) * edge_size, 0.0f);
        } else {
            // normal case
            A = std::accumulate(contact_matrix + locus_index * edge_size + locus_index - range, contact_matrix + locus_index * edge_size + locus_index, 0.0f);
            B = std::accumulate(contact_matrix + locus_index * edge_size + locus_index + 1, contact_matrix + locus_index * edge_size + locus_index + range + 1, 0.0f);
        }

        float E = (A + B) / 2;

        if (A == 0 && B == 0) {
            di[locus_index] = 0;
        } else {
            try {
                di[locus_index] = ((B - A) / (std::abs(B - A))) * ((((A - E) * (A - E)) / E) + (((B - E) * (B - E)) / E));
            } catch (std::exception& e) {
                di[locus_index] = 0;
            }
        }
    }
    return di;
}

static void BM_calculate_di_SCALAR(benchmark::State& state) {
    std::size_t edge_size = state.range(0);

    std::vector<float> data(edge_size * edge_size, 0);
    // fill random positive floats
    std::generate(data.begin(), data.end(), []() { return static_cast<float>(rand()) / static_cast<float>(RAND_MAX); });

    for (auto _ : state) {
        calculate_di_SCALAR(data.data(), edge_size, 5000);
    }
}

static void BM_calculate_di_AVX2(benchmark::State& state) {
    std::size_t edge_size = state.range(0);

    std::vector<float> data(edge_size * edge_size, 0);
    // fill random positive floats
    std::generate(data.begin(), data.end(), []() { return static_cast<float>(rand()) / static_cast<float>(RAND_MAX); });

    for (auto _ : state) {
        calculate_di_AVX2(data.data(), edge_size, 5000);
    }
}

BENCHMARK(BM_calculate_di_SCALAR)->Arg(33957);
BENCHMARK(BM_calculate_di_AVX2)->Arg(33957);

BENCHMARK_MAIN();