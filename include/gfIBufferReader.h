#pragma once
#include "gfIoConfig.h"

namespace Grainflow
{
	class gf_buffer_info
	{
	public:
		int buffer_frames = 0;
		float one_over_buffer_frames = 0;
		float sample_rate_adjustment = 1;
		int n_channels = 0;
		int samplerate = 48000;
		double one_over_samplerate = 1;
	};

	template <typename T>
	struct gf_i_buffer_reader
	{
	public:
		bool (*sample_param_buffer)(T* buffer, gf_param* param, int grain_id) = nullptr;
		void (*sample_buffer)(T* buffer, int channel, double* __restrict samples, const double* positions,
		                      const int size) = nullptr;
		void (*update_buffer_info)(T* buffer, const gf_io_config& io_config, gf_buffer_info* buffer_info) = nullptr;
		void (*sample_envelope)(T* buffer, const bool use_default, const int n_envelopes, const float env2d_pos,
		                        double* __restrict samples, const double* __restrict grain_clock,
		                        const int size) = nullptr;
		void (*write_buffer)(T* buffer, const int channel, const double* samples, double* __restrict scratch,
			const int start_position, const float overdub, const int size) = nullptr;

	};
}
