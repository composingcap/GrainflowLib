#include <benchmark/benchmark.h>
#include <cmath>
#include "gfUtils.h"

using namespace Grainflow;

// ============================================================================
// Utility Function Benchmarks
// ============================================================================

// Benchmark: gf_utils random number generation
static void BM_RandomUniform_Float(benchmark::State& state) {
    for (auto _ : state) {
        float result = gf_utils::random_uniform<float>();
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_RandomUniform_Float);

static void BM_RandomUniform_Int(benchmark::State& state) {
    for (auto _ : state) {
        int result = gf_utils::random_uniform<int>(0, 100);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_RandomUniform_Int);

static void BM_RandomUniform_FloatRange(benchmark::State& state) {
    for (auto _ : state) {
        float result = gf_utils::random_uniform<float>(-1.0f, 1.0f);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_RandomUniform_FloatRange);

// Benchmark: gf_utils math functions
static void BM_PitchToRate(benchmark::State& state) {
    float pitch = 12.0f;
    for (auto _ : state) {
        float result = gf_utils::pitch_to_rate(pitch);
        benchmark::DoNotOptimize(result);
        pitch += 0.1f;
        if (pitch > 24.0f) pitch = 12.0f;
    }
}
BENCHMARK(BM_PitchToRate);

static void BM_Lerp(benchmark::State& state) {
    float t = 0.0f;
    for (auto _ : state) {
        float result = gf_utils::lerp(0.0f, 1.0f, t);
        benchmark::DoNotOptimize(result);
        t += 0.01f;
        if (t > 1.0f) t = 0.0f;
    }
}
BENCHMARK(BM_Lerp);

static void BM_Mod(benchmark::State& state) {
    double val = 0.0;
    for (auto _ : state) {
        double result = gf_utils::mod(val, 1.0);
        benchmark::DoNotOptimize(result);
        val += 0.1;
    }
}
BENCHMARK(BM_Mod);

static void BM_CubicHermite(benchmark::State& state) {
    double a = 0.1, b = 0.5, c = 0.8, d = 0.3;
    float t = 0.0f;

    for (auto _ : state) {
        double result = gf_utils::cubic_hermite(a, b, c, d, t);
        benchmark::DoNotOptimize(result);
        t += 0.01f;
        if (t > 1.0f) t = 0.0f;
    }
}
BENCHMARK(BM_CubicHermite);

static void BM_Deviate(benchmark::State& state) {
    float center = 0.5f;
    float range = 0.1f;

    for (auto _ : state) {
        float result = gf_utils::deviate(center, range);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Deviate);

static void BM_RandomRange(benchmark::State& state) {
    for (auto _ : state) {
        float result = gf_utils::random_range(0.0f, 1.0f);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_RandomRange);

static void BM_Pong(benchmark::State& state) {
    double val = 0.0;
    for (auto _ : state) {
        double result = gf_utils::pong(val, 0.0, 1.0, 1);
        benchmark::DoNotOptimize(result);
        val += 0.1;
        if (val > 2.0) val = 0.0;
    }
}
BENCHMARK(BM_Pong);

BENCHMARK_MAIN();
