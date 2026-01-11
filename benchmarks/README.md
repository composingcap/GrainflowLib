# Grainflow Library Benchmarks

This directory contains performance benchmarks for the Grainflow library using Google Benchmark.

## Prerequisites

You need to have Google Benchmark installed on your system:

### macOS (using Homebrew)
```bash
brew install google-benchmark
```

### Linux (Ubuntu/Debian)
```bash
sudo apt-get install libbenchmark-dev
```

### Building from source
```bash
git clone https://github.com/google/benchmark.git
cd benchmark
cmake -E make_directory "build"
cmake -E chdir "build" cmake -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on -DCMAKE_BUILD_TYPE=Release ../
cmake --build "build" --config Release
sudo cmake --build "build" --config Release --target install
```

## Building the Benchmarks

From the grainflow root directory:

```bash
mkdir -p build
cd build
cmake .. -DBUILD_BENCHMARKS=ON
cmake --build .
```

Or using the Ninja generator (faster):

```bash
mkdir -p build
cd build
cmake .. -G Ninja -DBUILD_BENCHMARKS=ON
ninja
```

## Benchmark Organization

The benchmarks are split into three separate executables for better organization and faster targeted testing:

1. **benchmark_utils** - Utility function benchmarks (math, random, interpolation)
2. **benchmark_synthesis** - DSP and synthesis benchmarks (envelopes, modulation, buffer operations)
3. **benchmark_integration** - End-to-end integration tests (complete grain processing with AudioFile)

## Running the Benchmarks

You can run individual benchmark suites:

```bash
# Run only utility benchmarks
./source/GrainflowLib/benchmarks/benchmark_utils

# Run only synthesis benchmarks
./source/GrainflowLib/benchmarks/benchmark_synthesis

# Run only integration benchmarks
./source/GrainflowLib/benchmarks/benchmark_integration
```

Or run all benchmarks at once:

```bash
cd build
ninja run_all_benchmarks
```

### Benchmark Options

Google Benchmark supports various command-line options for any of the benchmark executables:

```bash
# Filter benchmarks by name (run only specific benchmarks)
./benchmark_utils --benchmark_filter=BM_Random

# Run benchmarks for a specific duration
./benchmark_synthesis --benchmark_min_time=2.0

# Output results to JSON
./benchmark_integration --benchmark_format=json --benchmark_out=results.json

# Output results to CSV
./benchmark_utils --benchmark_format=csv --benchmark_out=results.csv

# Show help
./benchmark_utils --help
```

## Benchmark Suite

The benchmark suite includes:

### Utility Functions
- **BM_RandomUniform_Float**: Tests float random number generation
- **BM_RandomUniform_Int**: Tests integer random number generation
- **BM_RandomUniform_FloatRange**: Tests float random number generation with range
- **BM_PitchToRate**: Tests pitch to rate conversion
- **BM_Lerp**: Tests linear interpolation
- **BM_Mod**: Tests modulo operation
- **BM_CubicHermite**: Tests cubic hermite interpolation
- **BM_Deviate**: Tests deviation calculation
- **BM_RandomRange**: Tests random range generation
- **BM_Pong**: Tests ping-pong/fold operation

### Buffer Operations
- **BM_CubicInterpolation**: Tests cubic interpolation on buffers (16, 32, 64, 128 samples)
- **BM_TriangularEnvelope**: Tests envelope generation (16, 32, 64, 128 samples)
- **BM_BufferWrapping**: Tests buffer position wrapping (16, 32, 64, 128 samples)

### DSP Operations
- **BM_ParamRandomization**: Tests parameter sampling with randomization
- **BM_PitchModulation**: Tests pitch modulation calculation

### End-to-End Integration Tests
- **BM_GrainWithAudioFile**: Tests complete single grain processing with AudioFile buffers (16, 32, 64, 128 samples)
- **BM_GrainCollectionWithAudioFile**: Tests complete grain collection processing with AudioFile buffers
  - Various combinations of block sizes (16, 32, 64) and grain counts (2, 4, 8)
  - Measures realistic performance including buffer management, envelope generation, and audio interpolation

## Interpreting Results

Google Benchmark outputs results in the following format:

```
Benchmark                          Time             CPU   Iterations
---------------------------------------------------------------------
BM_RandomUniform_Float          3.45 ns         3.45 ns    202456789
BM_GrainProcess<16>             1234 ns         1234 ns       567890
```

- **Time**: Wall clock time per iteration
- **CPU**: CPU time per iteration
- **Iterations**: Number of times the benchmark was executed

For benchmarks that process multiple items, the output may also include throughput metrics:

```
BM_GrainCollection<16, 8>      items_processed=128/iter
```

## Comparing Performance

You can compare different implementations by running benchmarks before and after changes:

```bash
# Before changes
./grainflow_benchmarks --benchmark_out=before.json --benchmark_format=json

# After changes (rebuild first)
./grainflow_benchmarks --benchmark_out=after.json --benchmark_format=json

# Compare using the compare.py script (from Google Benchmark tools)
python compare.py benchmarks before.json after.json
```

## Adding New Benchmarks

To add a new benchmark:

1. Include necessary headers in `benchmark_main.cpp`
2. Write a benchmark function following this pattern:

```cpp
static void BM_YourBenchmark(benchmark::State& state) {
    // Setup code here (runs once)

    for (auto _ : state) {
        // Code to benchmark (runs many times)
        YourFunctionToTest();
        benchmark::DoNotOptimize(result); // Prevent optimization
    }

    // Optional: report processed items
    state.SetItemsProcessed(state.iterations() * items_per_iteration);
}
BENCHMARK(BM_YourBenchmark);
```

3. Rebuild the benchmarks

## Tips for Accurate Benchmarking

1. **Close other applications** to minimize system noise
2. **Run multiple times** to ensure consistency
3. **Use release builds** (already configured in CMakeLists.txt)
4. **Disable CPU frequency scaling** if possible:
   ```bash
   # macOS
   sudo pmset -a disablesleep 1

   # Linux
   echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
   ```
5. **Use `benchmark::DoNotOptimize()`** to prevent the compiler from optimizing away your code

## References

- [Google Benchmark Documentation](https://github.com/google/benchmark)
- [Google Benchmark User Guide](https://github.com/google/benchmark/blob/main/docs/user_guide.md)
