#include <benchmark/benchmark.h>
#include <vector>
#include <cmath>
#include "gfUtils.h"

using namespace Grainflow;

// ============================================================================
// DSP and Synthesis Benchmarks
// ============================================================================

// Benchmark: Buffer interpolation simulation
template<size_t BlockSize>
static void BM_CubicInterpolation(benchmark::State& state) {
    std::vector<double> buffer(1024);
    std::vector<double> positions(BlockSize);
    std::vector<double> output(BlockSize);

    // Fill buffer with sine wave
    for (size_t i = 0; i < buffer.size(); ++i) {
        buffer[i] = std::sin(2.0 * M_PI * 440.0 * i / 48000.0);
    }

    // Fill positions
    for (size_t i = 0; i < BlockSize; ++i) {
        positions[i] = (i * 1.5) + 100.0;
    }

    for (auto _ : state) {
        for (size_t i = 0; i < BlockSize; ++i) {
            double pos = positions[i];
            int idx = static_cast<int>(pos);
            float frac = pos - idx;

            int idx0 = (idx - 1 + buffer.size()) % buffer.size();
            int idx1 = idx % buffer.size();
            int idx2 = (idx + 1) % buffer.size();
            int idx3 = (idx + 2) % buffer.size();

            output[i] = gf_utils::cubic_hermite(
                buffer[idx0], buffer[idx1], buffer[idx2], buffer[idx3], frac);
        }
        benchmark::DoNotOptimize(output.data());
    }

    state.SetItemsProcessed(state.iterations() * BlockSize);
}

BENCHMARK(BM_CubicInterpolation<16>);
BENCHMARK(BM_CubicInterpolation<32>);
BENCHMARK(BM_CubicInterpolation<64>);
BENCHMARK(BM_CubicInterpolation<128>);

// Benchmark: Envelope generation (triangular)
template<size_t BlockSize>
static void BM_TriangularEnvelope(benchmark::State& state) {
    std::vector<double> progress(BlockSize);
    std::vector<double> envelope(BlockSize);

    for (size_t i = 0; i < BlockSize; ++i) {
        progress[i] = static_cast<double>(i) / BlockSize;
    }

    for (auto _ : state) {
        for (size_t i = 0; i < BlockSize; ++i) {
            const double p = progress[i];
            envelope[i] = (p < 0.5) ? (2.0 * p) : (2.0 - 2.0 * p);
        }
        benchmark::DoNotOptimize(envelope.data());
    }

    state.SetItemsProcessed(state.iterations() * BlockSize);
}

BENCHMARK(BM_TriangularEnvelope<16>);
BENCHMARK(BM_TriangularEnvelope<32>);
BENCHMARK(BM_TriangularEnvelope<64>);
BENCHMARK(BM_TriangularEnvelope<128>);

// Benchmark: Parameter randomization pattern
static void BM_ParamRandomization(benchmark::State& state) {
    float base = 1.0f;
    float offset = 0.5f;
    float random = 0.3f;
    float g = 0.5f;

    for (auto _ : state) {
        const float rng = gf_utils::random_uniform<float>();
        float value = rng * random + base + offset * g;
        benchmark::DoNotOptimize(value);
    }
}
BENCHMARK(BM_ParamRandomization);

// Benchmark: Pitch modulation calculation
static void BM_PitchModulation(benchmark::State& state) {
    float base_pitch = 0.0f;
    float mod_amount = 12.0f;

    for (auto _ : state) {
        float mod = gf_utils::random_uniform<float>(-1.0f, 1.0f);
        float pitch = base_pitch + (mod * mod_amount);
        float rate = gf_utils::pitch_to_rate(pitch);
        benchmark::DoNotOptimize(rate);
    }
}
BENCHMARK(BM_PitchModulation);

// Benchmark: Buffer position wrapping with modulo
template<size_t BlockSize>
static void BM_BufferWrapping(benchmark::State& state) {
    std::vector<double> positions(BlockSize);
    std::vector<double> wrapped(BlockSize);
    double buffer_frames = 48000.0;

    for (size_t i = 0; i < BlockSize; ++i) {
        positions[i] = i * 1000.0 + 50000.0; // Positions beyond buffer
    }

    for (auto _ : state) {
        for (size_t i = 0; i < BlockSize; ++i) {
            wrapped[i] = gf_utils::mod(positions[i], buffer_frames);
        }
        benchmark::DoNotOptimize(wrapped.data());
    }

    state.SetItemsProcessed(state.iterations() * BlockSize);
}

BENCHMARK(BM_BufferWrapping<16>);
BENCHMARK(BM_BufferWrapping<32>);
BENCHMARK(BM_BufferWrapping<64>);
BENCHMARK(BM_BufferWrapping<128>);

BENCHMARK_MAIN();
