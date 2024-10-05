#pragma once
#include <memory>
#include <random>
#include <algorithm>
#include <numeric>
#include "gfParam.h"
#include "gfUtils.h"
#include "gfIBufferReader.h"
#include "gfIoConfig.h"


/// <summary>
/// Contains entries and functions that modify said entities. This is the
/// fastest way to process data while also having the ability for it to be organized.
/// </summary>
namespace Grainflow
{
	constexpr float hanning_envelope[1024] = {
		0.0000, 0.0000, 0.0000, 0.0001, 0.0002, 0.0002, 0.0003, 0.0005, 0.0006, 0.0008, 0.0009, 0.0011, 0.0014, 0.0016,
		0.0018, 0.0021, 0.0024, 0.0027, 0.0030, 0.0034, 0.0038, 0.0041, 0.0045, 0.0050, 0.0054, 0.0059, 0.0063, 0.0068,
		0.0074, 0.0079, 0.0084, 0.0090, 0.0096, 0.0102, 0.0108, 0.0115, 0.0121, 0.0128, 0.0135, 0.0142, 0.0150, 0.0157,
		0.0165, 0.0173, 0.0181, 0.0189, 0.0198, 0.0206, 0.0215, 0.0224, 0.0233, 0.0243, 0.0252, 0.0262, 0.0272, 0.0282,
		0.0292, 0.0303, 0.0313, 0.0324, 0.0335, 0.0346, 0.0357, 0.0369, 0.0381, 0.0392, 0.0404, 0.0417, 0.0429, 0.0441,
		0.0454, 0.0467, 0.0480, 0.0493, 0.0507, 0.0520, 0.0534, 0.0548, 0.0562, 0.0576, 0.0590, 0.0605, 0.0620, 0.0635,
		0.0650, 0.0665, 0.0680, 0.0696, 0.0711, 0.0727, 0.0743, 0.0759, 0.0776, 0.0792, 0.0809, 0.0826, 0.0843, 0.0860,
		0.0877, 0.0894, 0.0912, 0.0930, 0.0948, 0.0966, 0.0984, 0.1002, 0.1021, 0.1039, 0.1058, 0.1077, 0.1096, 0.1116,
		0.1135, 0.1154, 0.1174, 0.1194, 0.1214, 0.1234, 0.1254, 0.1275, 0.1295, 0.1316, 0.1337, 0.1358, 0.1379, 0.1400,
		0.1421, 0.1443, 0.1464, 0.1486, 0.1508, 0.1530, 0.1552, 0.1575, 0.1597, 0.1620, 0.1642, 0.1665, 0.1688, 0.1711,
		0.1734, 0.1757, 0.1781, 0.1804, 0.1828, 0.1852, 0.1876, 0.1900, 0.1924, 0.1948, 0.1972, 0.1997, 0.2022, 0.2046,
		0.2071, 0.2096, 0.2121, 0.2146, 0.2171, 0.2197, 0.2222, 0.2248, 0.2273, 0.2299, 0.2325, 0.2351, 0.2377, 0.2403,
		0.2429, 0.2456, 0.2482, 0.2509, 0.2536, 0.2562, 0.2589, 0.2616, 0.2643, 0.2670, 0.2697, 0.2725, 0.2752, 0.2779,
		0.2807, 0.2835, 0.2862, 0.2890, 0.2918, 0.2946, 0.2974, 0.3002, 0.3030, 0.3058, 0.3087, 0.3115, 0.3143, 0.3172,
		0.3201, 0.3229, 0.3258, 0.3287, 0.3316, 0.3344, 0.3373, 0.3402, 0.3432, 0.3461, 0.3490, 0.3519, 0.3549, 0.3578,
		0.3607, 0.3637, 0.3666, 0.3696, 0.3726, 0.3755, 0.3785, 0.3815, 0.3845, 0.3875, 0.3904, 0.3934, 0.3964, 0.3994,
		0.4025, 0.4055, 0.4085, 0.4115, 0.4145, 0.4175, 0.4206, 0.4236, 0.4266, 0.4297, 0.4327, 0.4358, 0.4388, 0.4418,
		0.4449, 0.4479, 0.4510, 0.4540, 0.4571, 0.4602, 0.4632, 0.4663, 0.4693, 0.4724, 0.4755, 0.4785, 0.4816, 0.4847,
		0.4877, 0.4908, 0.4939, 0.4969, 0.5000, 0.5031, 0.5061, 0.5092, 0.5123, 0.5153, 0.5184, 0.5215, 0.5245, 0.5276,
		0.5307, 0.5337, 0.5368, 0.5398, 0.5429, 0.5460, 0.5490, 0.5521, 0.5551, 0.5582, 0.5612, 0.5642, 0.5673, 0.5703,
		0.5734, 0.5764, 0.5794, 0.5825, 0.5855, 0.5885, 0.5915, 0.5945, 0.5975, 0.6006, 0.6036, 0.6066, 0.6096, 0.6125,
		0.6155, 0.6185, 0.6215, 0.6245, 0.6274, 0.6304, 0.6334, 0.6363, 0.6393, 0.6422, 0.6451, 0.6481, 0.6510, 0.6539,
		0.6568, 0.6598, 0.6627, 0.6656, 0.6684, 0.6713, 0.6742, 0.6771, 0.6799, 0.6828, 0.6857, 0.6885, 0.6913, 0.6942,
		0.6970, 0.6998, 0.7026, 0.7054, 0.7082, 0.7110, 0.7138, 0.7165, 0.7193, 0.7221, 0.7248, 0.7275, 0.7303, 0.7330,
		0.7357, 0.7384, 0.7411, 0.7438, 0.7464, 0.7491, 0.7518, 0.7544, 0.7571, 0.7597, 0.7623, 0.7649, 0.7675, 0.7701,
		0.7727, 0.7752, 0.7778, 0.7803, 0.7829, 0.7854, 0.7879, 0.7904, 0.7929, 0.7954, 0.7978, 0.8003, 0.8028, 0.8052,
		0.8076, 0.8100, 0.8124, 0.8148, 0.8172, 0.8196, 0.8219, 0.8243, 0.8266, 0.8289, 0.8312, 0.8335, 0.8358, 0.8380,
		0.8403, 0.8425, 0.8448, 0.8470, 0.8492, 0.8514, 0.8536, 0.8557, 0.8579, 0.8600, 0.8621, 0.8642, 0.8663, 0.8684,
		0.8705, 0.8725, 0.8746, 0.8766, 0.8786, 0.8806, 0.8826, 0.8846, 0.8865, 0.8884, 0.8904, 0.8923, 0.8942, 0.8961,
		0.8979, 0.8998, 0.9016, 0.9034, 0.9052, 0.9070, 0.9088, 0.9106, 0.9123, 0.9140, 0.9157, 0.9174, 0.9191, 0.9208,
		0.9224, 0.9241, 0.9257, 0.9273, 0.9289, 0.9304, 0.9320, 0.9335, 0.9350, 0.9365, 0.9380, 0.9395, 0.9410, 0.9424,
		0.9438, 0.9452, 0.9466, 0.9480, 0.9493, 0.9507, 0.9520, 0.9533, 0.9546, 0.9559, 0.9571, 0.9583, 0.9596, 0.9608,
		0.9619, 0.9631, 0.9643, 0.9654, 0.9665, 0.9676, 0.9687, 0.9697, 0.9708, 0.9718, 0.9728, 0.9738, 0.9748, 0.9757,
		0.9767, 0.9776, 0.9785, 0.9794, 0.9802, 0.9811, 0.9819, 0.9827, 0.9835, 0.9843, 0.9850, 0.9858, 0.9865, 0.9872,
		0.9879, 0.9885, 0.9892, 0.9898, 0.9904, 0.9910, 0.9916, 0.9921, 0.9926, 0.9932, 0.9937, 0.9941, 0.9946, 0.9950,
		0.9955, 0.9959, 0.9962, 0.9966, 0.9970, 0.9973, 0.9976, 0.9979, 0.9982, 0.9984, 0.9986, 0.9989, 0.9991, 0.9992,
		0.9994, 0.9995, 0.9997, 0.9998, 0.9998, 0.9999, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 0.9999, 0.9998, 0.9998,
		0.9997, 0.9995, 0.9994, 0.9992, 0.9991, 0.9989, 0.9986, 0.9984, 0.9982, 0.9979, 0.9976, 0.9973, 0.9970, 0.9966,
		0.9962, 0.9959, 0.9955, 0.9950, 0.9946, 0.9941, 0.9937, 0.9932, 0.9926, 0.9921, 0.9916, 0.9910, 0.9904, 0.9898,
		0.9892, 0.9885, 0.9879, 0.9872, 0.9865, 0.9858, 0.9850, 0.9843, 0.9835, 0.9827, 0.9819, 0.9811, 0.9802, 0.9794,
		0.9785, 0.9776, 0.9767, 0.9757, 0.9748, 0.9738, 0.9728, 0.9718, 0.9708, 0.9697, 0.9687, 0.9676, 0.9665, 0.9654,
		0.9643, 0.9631, 0.9619, 0.9608, 0.9596, 0.9583, 0.9571, 0.9559, 0.9546, 0.9533, 0.9520, 0.9507, 0.9493, 0.9480,
		0.9466, 0.9452, 0.9438, 0.9424, 0.9410, 0.9395, 0.9380, 0.9365, 0.9350, 0.9335, 0.9320, 0.9304, 0.9289, 0.9273,
		0.9257, 0.9241, 0.9224, 0.9208, 0.9191, 0.9174, 0.9157, 0.9140, 0.9123, 0.9106, 0.9088, 0.9070, 0.9052, 0.9034,
		0.9016, 0.8998, 0.8979, 0.8961, 0.8942, 0.8923, 0.8904, 0.8884, 0.8865, 0.8846, 0.8826, 0.8806, 0.8786, 0.8766,
		0.8746, 0.8725, 0.8705, 0.8684, 0.8663, 0.8642, 0.8621, 0.8600, 0.8579, 0.8557, 0.8536, 0.8514, 0.8492, 0.8470,
		0.8448, 0.8425, 0.8403, 0.8380, 0.8358, 0.8335, 0.8312, 0.8289, 0.8266, 0.8243, 0.8219, 0.8196, 0.8172, 0.8148,
		0.8124, 0.8100, 0.8076, 0.8052, 0.8028, 0.8003, 0.7978, 0.7954, 0.7929, 0.7904, 0.7879, 0.7854, 0.7829, 0.7803,
		0.7778, 0.7752, 0.7727, 0.7701, 0.7675, 0.7649, 0.7623, 0.7597, 0.7571, 0.7544, 0.7518, 0.7491, 0.7464, 0.7438,
		0.7411, 0.7384, 0.7357, 0.7330, 0.7303, 0.7275, 0.7248, 0.7221, 0.7193, 0.7165, 0.7138, 0.7110, 0.7082, 0.7054,
		0.7026, 0.6998, 0.6970, 0.6942, 0.6913, 0.6885, 0.6857, 0.6828, 0.6799, 0.6771, 0.6742, 0.6713, 0.6684, 0.6656,
		0.6627, 0.6598, 0.6568, 0.6539, 0.6510, 0.6481, 0.6451, 0.6422, 0.6393, 0.6363, 0.6334, 0.6304, 0.6274, 0.6245,
		0.6215, 0.6185, 0.6155, 0.6125, 0.6096, 0.6066, 0.6036, 0.6006, 0.5975, 0.5945, 0.5915, 0.5885, 0.5855, 0.5825,
		0.5794, 0.5764, 0.5734, 0.5703, 0.5673, 0.5642, 0.5612, 0.5582, 0.5551, 0.5521, 0.5490, 0.5460, 0.5429, 0.5398,
		0.5368, 0.5337, 0.5307, 0.5276, 0.5245, 0.5215, 0.5184, 0.5153, 0.5123, 0.5092, 0.5061, 0.5031, 0.5000, 0.4969,
		0.4939, 0.4908, 0.4877, 0.4847, 0.4816, 0.4785, 0.4755, 0.4724, 0.4693, 0.4663, 0.4632, 0.4602, 0.4571, 0.4540,
		0.4510, 0.4479, 0.4449, 0.4418, 0.4388, 0.4358, 0.4327, 0.4297, 0.4266, 0.4236, 0.4206, 0.4175, 0.4145, 0.4115,
		0.4085, 0.4055, 0.4025, 0.3994, 0.3964, 0.3934, 0.3904, 0.3875, 0.3845, 0.3815, 0.3785, 0.3755, 0.3726, 0.3696,
		0.3666, 0.3637, 0.3607, 0.3578, 0.3549, 0.3519, 0.3490, 0.3461, 0.3432, 0.3402, 0.3373, 0.3344, 0.3316, 0.3287,
		0.3258, 0.3229, 0.3201, 0.3172, 0.3143, 0.3115, 0.3087, 0.3058, 0.3030, 0.3002, 0.2974, 0.2946, 0.2918, 0.2890,
		0.2862, 0.2835, 0.2807, 0.2779, 0.2752, 0.2725, 0.2697, 0.2670, 0.2643, 0.2616, 0.2589, 0.2562, 0.2536, 0.2509,
		0.2482, 0.2456, 0.2429, 0.2403, 0.2377, 0.2351, 0.2325, 0.2299, 0.2273, 0.2248, 0.2222, 0.2197, 0.2171, 0.2146,
		0.2121, 0.2096, 0.2071, 0.2046, 0.2022, 0.1997, 0.1972, 0.1948, 0.1924, 0.1900, 0.1876, 0.1852, 0.1828, 0.1804,
		0.1781, 0.1757, 0.1734, 0.1711, 0.1688, 0.1665, 0.1642, 0.1620, 0.1597, 0.1575, 0.1552, 0.1530, 0.1508, 0.1486,
		0.1464, 0.1443, 0.1421, 0.1400, 0.1379, 0.1358, 0.1337, 0.1316, 0.1295, 0.1275, 0.1254, 0.1234, 0.1214, 0.1194,
		0.1174, 0.1154, 0.1135, 0.1116, 0.1096, 0.1077, 0.1058, 0.1039, 0.1021, 0.1002, 0.0984, 0.0966, 0.0948, 0.0930,
		0.0912, 0.0894, 0.0877, 0.0860, 0.0843, 0.0826, 0.0809, 0.0792, 0.0776, 0.0759, 0.0743, 0.0727, 0.0711, 0.0696,
		0.0680, 0.0665, 0.0650, 0.0635, 0.0620, 0.0605, 0.0590, 0.0576, 0.0562, 0.0548, 0.0534, 0.0520, 0.0507, 0.0493,
		0.0480, 0.0467, 0.0454, 0.0441, 0.0429, 0.0417, 0.0404, 0.0392, 0.0381, 0.0369, 0.0357, 0.0346, 0.0335, 0.0324,
		0.0313, 0.0303, 0.0292, 0.0282, 0.0272, 0.0262, 0.0252, 0.0243, 0.0233, 0.0224, 0.0215, 0.0206, 0.0198, 0.0189,
		0.0181, 0.0173, 0.0165, 0.0157, 0.0150, 0.0142, 0.0135, 0.0128, 0.0121, 0.0115, 0.0108, 0.0102, 0.0096, 0.0090,
		0.0084, 0.0079, 0.0074, 0.0068, 0.0063, 0.0059, 0.0054, 0.0050, 0.0045, 0.0041, 0.0038, 0.0034, 0.0030, 0.0027,
		0.0024, 0.0021, 0.0018, 0.0016, 0.0014, 0.0011, 0.0009, 0.0008, 0.0006, 0.0005, 0.0003, 0.0002, 0.0002, 0.0001,
		0.0000, 0.0000
	};

	/// <summary>
	/// An interface that represents a grainflow grain.
	/// To implement a grain, a valid interface needs to implement:
	/// -SampleParamBuffer
	/// -SampleEnvelope
	/// -SampleParamBuffer
	/// </summary>
	template <typename T, size_t Blocksize>
	class gf_grain
	{
	private:
		bool reset_ = false;
		double last_grain_clock_ = -999;
		float source_position_norm_ = 0;
		bool grain_enabled_ = true;
		bool buffer_defined_ = false;
		gf_value_table value_table_[2];
		double sample_id_temp_[Blocksize];
		float density_temp_[Blocksize];
		float amp_temp_[Blocksize];
		double temp_double_[Blocksize];
		double glisson_temp_[Blocksize];
		bool reset_pending_;
		std::random_device rd_;
		int g_ = 0;

	public:
		int buffer_samplerate = 48000;

		bool use_default_envelope = true;
		double source_sample = 0;
		size_t stream = 0;
		float density = 1;

		gf_param delay;
		gf_param window;
		gf_param space;
		gf_param amplitude;
		gf_param rate;
		gf_param glisson;
		gf_param envelope;
		gf_param direction;
		gf_param n_envelopes;
		gf_param glisson_rows;
		gf_param glisson_position;
		gf_param rate_quantize_semi;
		gf_param loop_mode;
		gf_param start_point;
		gf_param stop_point;
		gf_param channel;

		/// Links to buffers - this can likely use a template argument and would be better
		T* buffer_ref = nullptr;
		T* envelope_ref = nullptr;
		T* delay_buf_ref = nullptr;
		T* rate_buf_ref = nullptr;
		T* window_buf_ref = nullptr;
		T* glisson_buffer = nullptr;

		gf_i_buffer_reader<T> buffer_reader;
		gf_buffer_info buffer_info;

		gf_grain(): value_table_{}, sample_id_temp_{}, density_temp_{}, amp_temp_{}, temp_double_{}, glisson_temp_{},
		            reset_pending_(false)
		{
			rate.base = 1;
			amplitude.base = 1;
			direction.base = 1;
			stop_point.base = 1;
			stop_point.value = 1;
			rate_quantize_semi.value = 1;
			n_envelopes.value = 1;
			glisson_rows.value = 1;
		}

		inline void process(gf_io_config& io_config)
		{
			if (io_config.block_size < Blocksize) return;
			buffer_reader.update_buffer_info(buffer_ref, io_config, &buffer_info);
			buffer_reader.update_buffer_info(envelope_ref, io_config, nullptr);


			const float window_portion = 1 / std::clamp(1 - space.value, 0.0001f, 1.0f);
			// Check grain clock to make sure it is moving
			if (io_config.grain_clock[0] == io_config.grain_clock[1])
				return;
			const float window_val = window.value;

			for (int i = 0; i < io_config.block_size / Blocksize; i++)
			{
				const int block = i * Blocksize;
				auto amp = amplitude.value;
				const double* grain_clock = &io_config.grain_clock[g_ % io_config.grain_clock_chans][block];
				double* input_amp = &io_config.am[g_ % io_config.am_chans][block];
				double* fm = &io_config.fm[g_ % io_config.fm_chans][block];
				const double* traversal_phasor = &io_config.traversal_phasor[g_ % io_config.traversal_phasor_chans][
					block];

				double* grain_progress = &io_config.grain_progress[g_][block];
				double* grain_state = &io_config.grain_state[g_][block];
				double* grain_playhead = &io_config.grain_playhead[g_][block];
				double* grain_amp = &io_config.grain_amp[g_][block];
				double* grain_envelope = &io_config.grain_envelope[g_][block];
				double* grain_output = &io_config.grain_output[g_][block];
				double* grain_channels = &io_config.grain_buffer_channel[g_][block];
				double* grain_streams = &io_config.grain_stream_channel[g_][block];

				process_grain_clock(grain_clock, grain_progress, window_val, window_portion, Blocksize);
				auto valueFrames = grain_reset(grain_progress, traversal_phasor, grain_state, Blocksize);
				increment(fm, grain_progress, sample_id_temp_, temp_double_, glisson_temp_, Blocksize);
				buffer_reader.sample_envelope(envelope_ref, use_default_envelope, n_envelopes.value, envelope.value,
				                              grain_envelope, grain_progress, Blocksize);
				buffer_reader.sample_buffer(buffer_ref, channel.value, grain_output, sample_id_temp_, Blocksize);
				expand_value_table(valueFrames, grain_state, amp_temp_, density_temp_, Blocksize);
				output_block(sample_id_temp_, amp_temp_, density_temp_, buffer_info.one_over_buffer_frames, stream,
				             input_amp, grain_playhead, grain_amp, grain_envelope, grain_output, grain_channels,
				             grain_streams, Blocksize);
			}
		}

		void set_index(int g) { this->g_ = g; }

		gf_param* param_get_handle(const gf_param_name param)
		{
			switch (param)
			{
			case (gf_param_name::delay):
				return &delay;
			case (gf_param_name::rate):
				return &rate;
			case (gf_param_name::window):
				return &window;
			case (gf_param_name::amplitude):
				return &amplitude;
			case (gf_param_name::glisson):
				return &glisson;
			case (gf_param_name::space):
				return &space;
			case (gf_param_name::envelope_position):
				return &envelope;
			case (gf_param_name::n_envelopes):
				return &n_envelopes;
			case (gf_param_name::glisson_rows):
				return &glisson_rows;
			case (gf_param_name::direction):
				return &direction;
			case (gf_param_name::stop_point):
				return &stop_point;
			case (gf_param_name::start_point):
				return &start_point;
			case (gf_param_name::rate_quantize_semi):
				return &rate_quantize_semi;
			case (gf_param_name::loop_mode):
				return &loop_mode;
			case (gf_param_name::channel):
				return &channel;
			case (gf_param_name::glisson_position):
				return &glisson_position;
			default:
				return nullptr;
			}
		}

		float param_get(const gf_param_name param)
		{
			return param_get_handle(param)->value;
		}

		void param_set(const float value, const gf_param_name param, const gf_param_type type)
		{
			gf_param* selected_param = param_get_handle(param);

			switch (type)
			{
			case (gf_param_type::base):
				selected_param->base = value;
				break;
			case (gf_param_type::random):
				selected_param->random = value;
				break;
			case (gf_param_type::offset):
				selected_param->offset = value;
				break;
			case (gf_param_type::mode):
				selected_param->mode = static_cast<gf_buffer_mode>(static_cast<int>(value));
				break;
			case (gf_param_type::value):
				selected_param->value = value;
				break;
			default:
				throw("invalid type");
				return;
			}
		}

		float param_get(const gf_param_name param_name, const gf_param_type param_type)
		{
			const auto param = param_get_handle(param_name);
			switch (param_type)
			{
			case(gf_param_type::base): return param->base;
			case(gf_param_type::random): return param->random;
			case(gf_param_type::offset): return param->offset;
			case(gf_param_type::value): return param->value;
			default: return 0;
			}
		}

		void sample_param(const gf_param_name param_name)
		{
			const auto param = param_get_handle(param_name);
			std::random_device rd;
			param->value = abs((rd() % 10000) * 0.0001f) * (param->random) + param->base + param->offset * g_;
		}

		void sample_param(gf_param* param) const
		{
			std::random_device rd;
			param->value = abs((rd() % 10000) * 0.0001f) * (param->random) + param->base + param->offset * g_;
		}

		static void sample_normalized(gf_param* param, const float range)
		{
			std::random_device rd;
			param->value = gf_utils::mod(
				(abs((rd() % 10000) * 0.0001f) * (param->random) + param->offset) * range + param->base, range);
		}

		inline gf_value_table* grain_reset(const double* __restrict grain_clock, const double* traversal,
		                                   double* __restrict grain_state, const int size)
		{
			for (int i = 0; i < 2; i++)
			{
				value_table_[i].delay = delay.value * 0.001 * buffer_info.samplerate;
				value_table_[i].rate = rate.value;
				value_table_[i].glisson = glisson.value;
				value_table_[i].window = window.value;
				value_table_[i].amplitude = amplitude.value;
				value_table_[i].space = space.value;
				value_table_[i].envelopePosition = envelope.value;
				value_table_[i].direction = direction.value;
				value_table_[i].density = grain_enabled_;
			}
			//TODO the performance of this statement can be improved 
			bool grain_reset = (last_grain_clock_ > grain_clock[0] && grain_clock[0] >= 0.00000001) || (
				last_grain_clock_ <= 0.00000001 && grain_clock[0] > 0.00000001);
			grain_state[0] = !grain_reset && grain_clock[0] >= 0.00000001;
			int reset_position = 0;
			for (int i = 1; i < size; i++)
			{
				const bool zero_cross = (grain_clock[i - 1] > grain_clock[i] && grain_clock[i] >= 0.00000001) || (
					grain_clock[i - 1] <=
					0.00000001 && grain_clock[i] > 0.00000001);
				grain_state[i] = !zero_cross && grain_clock[i] >= 0.00000001;
				reset_position = reset_position * !(grain_reset && zero_cross) + i * (grain_reset && zero_cross);
				grain_reset = grain_reset || zero_cross;
			}

			last_grain_clock_ = grain_clock[size - 1];
			if (!grain_reset) return value_table_;

			if (!buffer_reader.sample_param_buffer(get_buffer(gf_buffers::delay_buffer),
			                                       param_get_handle(gf_param_name::delay), g_))
				sample_param(
					gf_param_name::delay);
			source_sample = ((traversal[reset_position]) * buffer_info.buffer_frames - (delay.value * 0.001f *
				buffer_samplerate) - 1);
			source_sample = gf_utils::mod(source_sample, buffer_info.buffer_frames);
			if (!buffer_reader.sample_param_buffer(get_buffer(gf_buffers::rate_buffer),
			                                       param_get_handle(gf_param_name::rate),
			                                       g_))
				sample_param(gf_param_name::rate);
			rate.value = 1 + gf_utils::round(rate.value - 1, 1 - rate_quantize_semi.value);
			if (!buffer_reader.sample_param_buffer(get_buffer(gf_buffers::window_buffer),
			                                       param_get_handle(gf_param_name::window), g_))
				sample_param(
					gf_param_name::window);
			sample_param(&space);
			sample_param(&glisson);
			sample_param(&envelope);
			sample_param(&amplitude);
			sample_param(&start_point);
			sample_param(&stop_point);
			sample_param(&glisson_position);
			sample_normalized(&channel, buffer_info.n_channels);
			sample_density();
			sample_direction();


			int i = 1;
			value_table_[i].delay = delay.value * 0.001 * buffer_samplerate;
			value_table_[i].rate = rate.value;
			value_table_[i].glisson = glisson.value;
			value_table_[i].window = window.value;
			value_table_[i].amplitude = amplitude.value;
			value_table_[i].space = space.value;
			value_table_[i].envelopePosition = envelope.value;
			value_table_[i].direction = direction.value;
			value_table_[i].density = grain_enabled_;

			return value_table_;
		}

		void set_buffer(const gf_buffers buffer_type, T* buffer)
		{
			switch (buffer_type)
			{
			case (gf_buffers::buffer):
				buffer_ref = buffer;
				break;
			case (gf_buffers::envelope):
				envelope_ref = buffer;
				break;
			case (gf_buffers::rate_buffer):
				rate_buf_ref = buffer;
				break;
			case (gf_buffers::delay_buffer):
				delay_buf_ref = buffer;
				break;
			case (gf_buffers::window_buffer):
				window_buf_ref = buffer;
				break;
			case (gf_buffers::glisson_buffer):
				glisson_buffer = buffer;
				break;
			};
		};

		T* get_buffer(const gf_buffers buffer_type)
		{
			switch (buffer_type)
			{
			case (gf_buffers::buffer):
				return buffer_ref;
			case (gf_buffers::envelope):
				return envelope_ref;
			case (gf_buffers::rate_buffer):
				return rate_buf_ref;
			case (gf_buffers::delay_buffer):
				return delay_buf_ref;
			case (gf_buffers::window_buffer):
				return window_buf_ref;
			case (gf_buffers::glisson_buffer):
				return glisson_buffer;
			}
			return nullptr;
		}

		inline void sample_density()
		{
			std::random_device rd;
			grain_enabled_ = density > (rd() % 10000) * 0.0001f;
		}

		static inline void expand_value_table(const gf_value_table* __restrict value_frames,
		                                      const double* __restrict grain_state, float* __restrict amplitudes,
		                                      float* __restrict densities, const int size)
		{
			for (int j = 0; j < size; j++)
			{
				amplitudes[j] = value_frames[static_cast<int>(grain_state[j])].amplitude;
				densities[j] = value_frames[static_cast<int>(grain_state[j])].density * grain_state[j];
			}
		}

		static inline void process_grain_clock(const double* __restrict grain_clock, double* __restrict grain_progress,
		                                       const float window_val, const float window_portion, const int size)
		{
			for (int j = 0; j < size; j++)
			{
				double sample = grain_clock[j] + window_val;
				sample -= floor(sample);
				sample *= window_portion;
				grain_progress[j] = sample;
			}
			for (int j = 0; j < size; j++)
			{
				grain_progress[j] = std::min(grain_progress[j], 1.0);
			}
		}

		inline void output_block(const double* __restrict sample_ids, const float* __restrict amplitudes,
		                         const float* __restrict densities, const float one_over_buffer_frames,
		                         const int stream, const double* input_amp,
		                         double* __restrict grain_playhead, double* __restrict grain_amp,
		                         double* __restrict grain_envelope,
		                         double* __restrict grain_output, double* __restrict grain_stream_channel,
		                         double* __restrict grain_buffer_channel, const int size) const
		{
			for (int j = 0; j < size; j++)
			{
				const float density = densities[j];;
				const float amplitude = amplitudes[j];
				grain_playhead[j] = sample_ids[j] * one_over_buffer_frames * density;
				grain_amp[j] = (1 - input_amp[j]) * amplitude * density;
				grain_envelope[j] *= density;
				grain_output[j] *= grain_amp[j] * 0.5 * grain_envelope[j];
				grain_stream_channel[j] = stream + 1;
				grain_buffer_channel[j] = static_cast<int>(channel.value) + 1;
			}
		}

		inline void increment(const double* __restrict fm, const double* __restrict grain_clock,
		                      double* __restrict sample_positions, double* __restrict sample_delta_temp,
		                      double* __restrict glisson_temp, const int size)
		{
			const int fold = loop_mode.base > 1.1f ? 1 : 0;
			const double start = std::min(static_cast<double>(buffer_info.buffer_frames) * start_point.value,
			                              static_cast<double>(buffer_info.buffer_frames) - 1);
			const double end = std::min(static_cast<double>(buffer_info.buffer_frames) * stop_point.value,
			                            static_cast<double>(buffer_info.buffer_frames) - 1);

			for (int i = 0; i < size; i++)
			{
				sample_delta_temp[i] = gf_utils::pitch_to_rate(fm[i]);
			}
			if (glisson.mode == gf_buffer_mode::normal)
			{
				for (int i = 0; i < size; i++)
				{
					sample_delta_temp[i] *= buffer_info.sample_rate_adjustment * rate.value * (1 + glisson.value *
						grain_clock[i]) * direction.value;
				}
			}
			else
			{
				buffer_reader.sample_envelope(glisson_buffer, false, glisson_rows.value, glisson_position.value,
				                              glisson_temp, grain_clock, size);
				for (int i = 0; i < size; i++)
				{
					sample_delta_temp[i] *= buffer_info.sample_rate_adjustment * rate.value * (1 + glisson_temp[i] *
						glisson.value * grain_clock[i]) * direction.value;
				}
			}
			sample_positions[0] = source_sample;
			double last_position = source_sample;
			for (int i = 1; i < size; i++)
			{
				sample_positions[i] = last_position + sample_delta_temp[i - 1];
				last_position = sample_positions[i];
			}
			source_sample = sample_positions[size - 1] + sample_delta_temp[size - 1];
			source_sample = gf_utils::pong(source_sample, start, end, fold);
			source_sample = std::clamp(source_sample, start, end);

			for (int i = 0; i < size; i++)
			{
				sample_positions[i] = gf_utils::pong(sample_positions[i], start, end, fold);
			}
			for (int i = 0; i < size; i++)
			{
				sample_positions[i] = std::clamp(sample_positions[i], start, end);
			}
		}


		void stream_set(const int max_grains, const gf_stream_set_type mode, const int nstreams)
		{
			switch (mode)
			{
			case gf_stream_set_type::automatic_streams:
				{
					stream = g_ % nstreams;
					break;
				}
			case gf_stream_set_type::per_streams:
				{
					stream = g_ / nstreams;
					break;
				}
			case gf_stream_set_type::random_streams:
				{
					std::random_device rd;
					stream = rd() % nstreams;
					break;
				}
			case gf_stream_set_type::manual_streams:
				{
					stream = (max_grains - 1 + nstreams) % nstreams;
					break;
				}
			default:
				{
					break;
				}
			}
		};

		void sample_direction()
		{
			if (direction.base >= 1)
				direction.value = 1;
			else if (direction.base <= -1)
				direction.value = -1;
			else
			{
				if (const float random_direction = (rand() % 1000) * 0.001f; random_direction > direction.base)
				{
					direction.value = -1;
				}
				else
				{
					direction.value = 1;
				}
			}
		}
	};
}
