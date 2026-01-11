#include <benchmark/benchmark.h>
#include <memory>
#include <vector>
#include <cmath>
#include "gfGrainCollection.h"
#include "gfGenericBufferReader.h"
#include "gfIoConfig.h"
#include "AudioFile.h"
#include "gfSyn.h"

using namespace Grainflow;

// ============================================================================
// End-to-End Integration Benchmarks with AudioFile
// ============================================================================


// Benchmark: Grain collection with dynamic AM/FM modulation
template<size_t BlockSize, int NumGrains>
static void BM_GrainCollectionWithDynamicModulation(benchmark::State& state) {
    using BufferType = gf_buffer<double>;
    constexpr int SampleRate = 48000;
    constexpr int seconds = 1; 
    constexpr int blocks = seconds*SampleRate/BlockSize;

    // Setup buffer with 1 second of audio
    auto buffer = std::make_unique<BufferType>(SampleRate, 2, SampleRate);

    // Fill buffer with sine wave test signal
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < SampleRate; ++i) {
            buffer->data_->samples[ch][i] = std::sin(2.0 * M_PI * 440.0 * i / SampleRate);
        }
    }

    // Setup buffer reader
    gf_i_buffer_reader<BufferType> reader;
    reader.sample_buffer = gf_buffer_reader<double>::sample_buffer;
    reader.sample_envelope = gf_buffer_reader<double>::sample_envelope;
    reader.update_buffer_info = gf_buffer_reader<double>::update_buffer_info;
    reader.sample_param_buffer = gf_buffer_reader<double>::sample_param_buffer;

    // Create grain collection
    auto grain_collection = std::make_unique<gf_grain_collection<BufferType, BlockSize>>(reader, NumGrains);
    grain_collection->samplerate = SampleRate;
    grain_collection->set_active_grains(NumGrains);

    // Set buffer collection
    std::vector<BufferType*> buffers = {buffer.get()};
    grain_collection->set_buffer_collection(gf_buffers::buffer, buffers);

    // Setup IO config
    gf_io_config<> io_config;
    io_config.block_size = BlockSize;
    io_config.samplerate = SampleRate;

    // Allocate signal arrays
    std::vector<double> grain_clock(BlockSize);
    std::vector<double> traversal(BlockSize);
    std::vector<double> fm(BlockSize);
    std::vector<double> am(BlockSize);
    std::vector<double> output_left(BlockSize, 0.0);
    std::vector<double> output_right(BlockSize, 0.0);

    std::vector<std::vector<double>> grain_output(NumGrains, std::vector<double>(BlockSize));
    std::vector<std::vector<double>> grain_state(NumGrains, std::vector<double>(BlockSize));
    std::vector<std::vector<double>> grain_progress(NumGrains, std::vector<double>(BlockSize));
    std::vector<std::vector<double>> grain_playhead(NumGrains, std::vector<double>(BlockSize));
    std::vector<std::vector<double>> grain_envelope(NumGrains, std::vector<double>(BlockSize));
    std::vector<std::vector<double>> grain_amp(NumGrains, std::vector<double>(BlockSize));
    std::vector<std::vector<double>> grain_stream_channel(NumGrains, std::vector<double>(BlockSize));
    std::vector<std::vector<double>> grain_buffer_channel(NumGrains, std::vector<double>(BlockSize));

    // Create pointer arrays
    std::vector<double*> grain_output_ptrs(NumGrains);
    std::vector<double*> grain_state_ptrs(NumGrains);
    std::vector<double*> grain_progress_ptrs(NumGrains);
    std::vector<double*> grain_playhead_ptrs(NumGrains);
    std::vector<double*> grain_envelope_ptrs(NumGrains);
    std::vector<double*> grain_amp_ptrs(NumGrains);
    std::vector<double*> grain_stream_channel_ptrs(NumGrains);
    std::vector<double*> grain_buffer_channel_ptrs(NumGrains);

    for (int i = 0; i < NumGrains; ++i) {
        grain_output_ptrs[i] = grain_output[i].data();
        grain_state_ptrs[i] = grain_state[i].data();
        grain_progress_ptrs[i] = grain_progress[i].data();
        grain_playhead_ptrs[i] = grain_playhead[i].data();
        grain_envelope_ptrs[i] = grain_envelope[i].data();
        grain_amp_ptrs[i] = grain_amp[i].data();
        grain_stream_channel_ptrs[i] = grain_stream_channel[i].data();
        grain_buffer_channel_ptrs[i] = grain_buffer_channel[i].data();
    }

    double* grain_clock_ptr = grain_clock.data();
    double* traversal_ptr = traversal.data();
    double* fm_ptr = fm.data();
    double* am_ptr = am.data();

    io_config.grain_clock = &grain_clock_ptr;
    io_config.traversal_phasor = &traversal_ptr;
    io_config.fm = &fm_ptr;
    io_config.am = &am_ptr;
    io_config.grain_output = grain_output_ptrs.data();
    io_config.grain_state = grain_state_ptrs.data();
    io_config.grain_progress = grain_progress_ptrs.data();
    io_config.grain_playhead = grain_playhead_ptrs.data();
    io_config.grain_envelope = grain_envelope_ptrs.data();
    io_config.grain_amp = grain_amp_ptrs.data();
    io_config.grain_stream_channel = grain_stream_channel_ptrs.data();
    io_config.grain_buffer_channel = grain_buffer_channel_ptrs.data();

    io_config.grain_clock_chans = 1;
    io_config.traversal_phasor_chans = 1;
    io_config.fm_chans = 1;
    io_config.am_chans = 1;

    auto grain_phasor = std::make_unique<phasor<double, BlockSize>>(16, SampleRate);
    auto traversal_phasor =  std::make_unique<phasor<double, BlockSize>>(0.1, SampleRate);
    auto fm_phasor = std::make_unique<phasor<double, BlockSize>>(3, SampleRate);
    auto am_phasor = std::make_unique<phasor<double, BlockSize>>(5, SampleRate);

    // Benchmark loop
    for (auto _ : state) {
        for (int i = 0; i < blocks; ++i){
        grain_phasor->perform(grain_clock.data());
        traversal_phasor->perform(traversal.data());
        fm_phasor->perform(fm.data());
        am_phasor->perform(am.data());

        grain_collection->process(io_config);
        benchmark::DoNotOptimize(grain_output_ptrs.data());
    }
}

    state.SetItemsProcessed(state.iterations() * BlockSize * NumGrains);
}

BENCHMARK(BM_GrainCollectionWithDynamicModulation<4, 256>);
BENCHMARK(BM_GrainCollectionWithDynamicModulation<16, 256>);
BENCHMARK(BM_GrainCollectionWithDynamicModulation<32, 256>);
BENCHMARK(BM_GrainCollectionWithDynamicModulation<64, 256>);
BENCHMARK(BM_GrainCollectionWithDynamicModulation<128, 256>);


BENCHMARK_MAIN();
