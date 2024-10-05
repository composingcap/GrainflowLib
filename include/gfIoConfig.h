#pragma once
namespace Grainflow
{
	struct gf_io_config
	{
		//Outputs
		double** grain_output = nullptr;
		double** grain_state = nullptr;
		double** grain_progress = nullptr;
		double** grain_playhead = nullptr;
		double** grain_amp = nullptr;
		double** grain_envelope = nullptr;
		double** grain_buffer_channel = nullptr;
		double** grain_stream_channel = nullptr;

		//Inputs
		double** grain_clock = nullptr;
		double** traversal_phasor = nullptr;
		double** fm = nullptr;
		double** am = nullptr;

		int grain_clock_chans;
		int traversal_phasor_chans;
		int fm_chans;
		int am_chans;

		bool livemode = false;
		int block_size = 0;
		int samplerate = 1;
	};
}
