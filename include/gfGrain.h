#pragma once
#include <memory>
#include <random>
#include <algorithm>
#include <numeric>
#include "gfParam.h"
#include "gfUtils.h"
#include "gfIBufferReader.h"
#include "gfIoConfig.h"
#include "gfSyn.h"


/// <summary>
/// Contains entries and functions that modify said entities. This is the
/// fastest way to process data while also having the ability for it to be organized.
/// </summary>
namespace Grainflow
{
	/// <summary>
	/// An interface that represents a grainflow grain.
	/// To implement a grain, a valid interface needs to implement:
	/// -SampleParamBuffer
	/// -SampleEnvelope
	/// -SampleParamBuffer
	/// </summary>
	template <typename T, size_t Blocksize, typename SigType = double>
	class gf_grain
	{
	private:
		static constexpr SigType Grainclock_Thresh = 1e-7;
		bool reset_ = false;
		SigType last_grain_clock_ = -999;
		float source_position_norm_ = 0;
		bool grain_enabled_ = true;
		bool buffer_defined_ = false;
		gf_value_table value_table_[2];
		SigType sample_id_temp_[Blocksize];
		float density_temp_[Blocksize];
		float amp_temp_[Blocksize];
		SigType temp_sigtype_[Blocksize];
		SigType glisson_temp_[Blocksize];
		std::unique_ptr<Grainflow::phasor<SigType, Blocksize>> vibrato_phasor_;
		bool reset_pending_;
		std::random_device rd_;
		int g_ = 0;
		bool enabled_internal_ = false;
		bool window_changed_;
		


	public:
		int buffer_samplerate = 48000;
		int system_samplerate = 48000;

		bool use_default_envelope = true;
		SigType source_sample = 0;
		size_t stream = 0;
		bool enabled;


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
		gf_param density;
		gf_param vibrato_rate;
		gf_param vibrato_depth;

		/// Links to buffers - this can likely use a template argument and would be better
		T* buffer_ref = nullptr;
		T* envelope_ref = nullptr;
		T* delay_buf_ref = nullptr;
		T* rate_buf_ref = nullptr;
		T* window_buf_ref = nullptr;
		T* glisson_buffer = nullptr;

		gf_i_buffer_reader<T, SigType> buffer_reader;
		gf_buffer_info buffer_info;

		gf_grain(): value_table_{}, sample_id_temp_{}, density_temp_{}, amp_temp_{}, temp_sigtype_{}, glisson_temp_{},
		            reset_pending_(false)
		{
			vibrato_phasor_ = std::make_unique<phasor<SigType, Blocksize>>(0, system_samplerate);

			rate.base = 1;
			amplitude.base = 1;
			direction.base = 1;
			stop_point.base = 1;
			stop_point.value = 1;
			rate_quantize_semi.value = 1;
			n_envelopes.value = 1;
			glisson_rows.value = 1;
			density.base = 1;
			std::fill_n(sample_id_temp_, Blocksize, 0);
			std::fill_n(density_temp_, Blocksize, 0);
			std::fill_n(amp_temp_, Blocksize, 0);
			std::fill_n(temp_sigtype_, Blocksize, 0);
			std::fill_n(glisson_temp_, Blocksize, 0);
		}

		~gf_grain()
		{
			delete buffer_ref;
			delete envelope_ref;
			delete delay_buf_ref;
			delete rate_buf_ref;
			delete window_buf_ref;
			delete glisson_buffer;
		}

		inline void process(gf_io_config<SigType>& io_config)
		{
			if (!enabled && !enabled_internal_) return;

			if (io_config.block_size < Blocksize) return;
			auto buffer_valid = buffer_reader.update_buffer_info(buffer_ref, io_config, &buffer_info);
			use_default_envelope = !buffer_reader.update_buffer_info(envelope_ref, io_config, nullptr);


			const float window_portion = 1 / std::max(std::min(1.0f - space.value, 0.0001f), 1.0f);
			// Check grain clock to make sure it is moving
			if (io_config.grain_clock[0] == io_config.grain_clock[1])
				return;
			const float window_val = window.value;

			for (int i = 0; i < io_config.block_size / Blocksize; i++)
			{
				const int block = i * Blocksize;
				auto amp = amplitude.value;
				const SigType* grain_clock = &io_config.grain_clock[g_ % io_config.grain_clock_chans][block];
				SigType* input_amp = &io_config.am[g_ % io_config.am_chans][block];
				SigType* fm = &io_config.fm[g_ % io_config.fm_chans][block];
				const SigType* traversal_phasor = &io_config.traversal_phasor[g_ % io_config.traversal_phasor_chans][
					block];

				SigType* grain_progress = &io_config.grain_progress[g_][block];
				SigType* grain_state = &io_config.grain_state[g_][block];
				SigType* grain_playhead = &io_config.grain_playhead[g_][block];
				SigType* grain_amp = &io_config.grain_amp[g_][block];
				SigType* grain_envelope = &io_config.grain_envelope[g_][block];
				SigType* grain_output = &io_config.grain_output[g_][block];
				SigType* grain_channels = &io_config.grain_buffer_channel[g_][block];
				SigType* grain_streams = &io_config.grain_stream_channel[g_][block];

				process_grain_clock(grain_clock, grain_progress, window_val, window_portion, Blocksize);
				auto valueFrames = grain_reset(grain_progress, traversal_phasor, grain_state, Blocksize);
				if (!enabled_internal_)
				{
					std::fill_n(grain_state, Blocksize, 0.0);
					std::fill_n(grain_progress, Blocksize, 0.0);
					continue;
				}
				if (window_changed_)
				{
					// todo: Stopping grain state will break the panner (fix this)
					std::fill_n(grain_progress, Blocksize, 0.0);
					continue;
				}
				increment(fm, grain_progress, sample_id_temp_, temp_sigtype_, glisson_temp_, system_samplerate,
				          Blocksize);
				buffer_reader.sample_envelope(envelope_ref, use_default_envelope, n_envelopes.value, envelope.value,
				                              grain_envelope, grain_progress, Blocksize);
				if (sample_id_temp_[0] != sample_id_temp_[0]) continue; //Nan check
				if (buffer_valid)
				{
					buffer_reader.sample_buffer(buffer_ref, channel.value, grain_output, sample_id_temp_,
					                            Blocksize, start_point.value, stop_point.value);
				}
				expand_value_table(valueFrames, grain_state, amp_temp_, density_temp_, Blocksize);
				output_block(sample_id_temp_, amp_temp_, density_temp_, buffer_info.one_over_buffer_frames, stream,
				             input_amp, grain_playhead, grain_amp, grain_envelope, grain_output, grain_streams,
				             grain_channels
				             , Blocksize);
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
			case (gf_param_name::density):
				return &density;
			case (gf_param_name::vibrato_rate):
				return &vibrato_rate;
			case (gf_param_name::vibrato_depth):
				return &vibrato_depth;

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

		inline gf_value_table* grain_reset(const SigType* __restrict grain_clock, const SigType* traversal,
		                                   SigType* __restrict grain_state, const int size)
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
				value_table_[i].density = grain_enabled_ && !window_changed_;
			}

			//TODO the performance of this statement can be improved 
			bool grain_reset = (last_grain_clock_ > grain_clock[0] && grain_clock[0] >= Grainclock_Thresh) || (
				last_grain_clock_ < Grainclock_Thresh && grain_clock[0] > Grainclock_Thresh);
			grain_state[0] = !grain_reset && grain_clock[0] >= Grainclock_Thresh;
			int reset_position = 0;
			for (int i = 1; i < size; i++)
			{
				const bool zero_cross = (grain_clock[i - 1] > grain_clock[i] && grain_clock[i] >= Grainclock_Thresh) ||
				(
					grain_clock[i - 1] <=
					Grainclock_Thresh && grain_clock[i] > Grainclock_Thresh);
				grain_state[i] = !zero_cross && grain_clock[i] >= Grainclock_Thresh;
				reset_position = reset_position * !(grain_reset && zero_cross) + i * (grain_reset && zero_cross);
				grain_reset = grain_reset || zero_cross;
			}
			const int enabled_mask = enabled_internal_ ? 1 : 0;

			last_grain_clock_ = grain_clock[size - 1] * enabled_mask + (1 - enabled_mask) * 0.001;
			if (!grain_reset) return value_table_;


			if (!buffer_reader.sample_param_buffer(get_buffer(gf_buffers::delay_buffer),
			                                       param_get_handle(gf_param_name::delay), g_))
				sample_param(
					gf_param_name::delay);
			source_sample = ((traversal[reset_position]) * buffer_info.buffer_frames - (delay.value * 0.001f *
				buffer_samplerate) - 1);
			source_sample = gf_utils::mod<SigType>(source_sample, buffer_info.buffer_frames);
			if (!buffer_reader.sample_param_buffer(get_buffer(gf_buffers::rate_buffer),
			                                       param_get_handle(gf_param_name::rate),
			                                       g_))
				sample_param(gf_param_name::rate);

			const auto last_window = window.value;
			rate.value = 1 + gf_utils::round(rate.value - 1, 1 - rate_quantize_semi.value);
			if (!window_changed_)
			{
				if (!buffer_reader.sample_param_buffer(get_buffer(gf_buffers::window_buffer),
				                                       param_get_handle(gf_param_name::window), g_))
					sample_param(
						gf_param_name::window);
			}
			window_changed_ = std::abs(window.value - last_window) > 0.00000001;
			sample_param(&space);
			sample_param(&glisson);
			sample_param(&envelope);
			sample_param(&amplitude);
			sample_param(&start_point);
			sample_param(&stop_point);
			sample_param(&glisson_position);
			sample_param(&vibrato_rate);
			sample_param(&vibrato_depth);
			sample_normalized(&channel, buffer_info.n_channels);
			sample_density();
			sample_direction();


			const int i = 1;
			value_table_[i].delay = delay.value * 0.001 * buffer_samplerate;
			value_table_[i].rate = rate.value;
			value_table_[i].glisson = glisson.value;
			value_table_[i].window = window.value;
			value_table_[i].amplitude = amplitude.value;
			value_table_[i].space = space.value;
			value_table_[i].envelopePosition = envelope.value;
			value_table_[i].direction = direction.value;
			value_table_[i].density = !window_changed_ && grain_enabled_;

			enabled_internal_ = enabled;

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
			grain_enabled_ = density.base > (rd() % 10000) * 0.0001f;
		}

		static inline void expand_value_table(const gf_value_table* __restrict value_frames,
		                                      const SigType* __restrict grain_state, float* __restrict amplitudes,
		                                      float* __restrict densities, const int size)
		{
			for (int j = 0; j < size; j++)
			{
				amplitudes[j] = value_frames[static_cast<int>(grain_state[j])].amplitude;
				densities[j] = value_frames[static_cast<int>(grain_state[j])].density * grain_state[j];
			}
		}

		static inline void process_grain_clock(const SigType* __restrict grain_clock,
		                                       SigType* __restrict grain_progress,
		                                       const float window_val, const float window_portion, const int size)
		{
			for (int j = 0; j < size; j++)
			{
				auto sample = grain_clock[j] + window_val;
				sample -= floor(sample);
				sample *= window_portion;
				grain_progress[j] = sample;
			}
			for (int j = 0; j < size; j++)
			{
				grain_progress[j] = std::min<SigType>(grain_progress[j], 1.0);
			}
		}

		inline void output_block(const SigType* __restrict sample_ids, const float* __restrict amplitudes,
		                         const float* __restrict densities, const float one_over_buffer_frames,
		                         const int stream, const SigType* input_amp,
		                         SigType* __restrict grain_playhead, SigType* __restrict grain_amp,
		                         SigType* __restrict grain_envelope,
		                         SigType* __restrict grain_output, SigType* __restrict grain_stream_channel,
		                         SigType* __restrict grain_buffer_channel, const int size) const
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

		inline void increment(const SigType* __restrict fm, const SigType* __restrict grain_clock,
		                      SigType* __restrict sample_positions, SigType* __restrict sample_delta_temp,
		                      SigType* __restrict glisson_temp, const int samplerate, const int size)
		{
			const int fold = loop_mode.base > 1.1f ? 1 : 0;
			const double start_tmp = std::min(static_cast<double>(buffer_info.buffer_frames) * start_point.value,
			                                  static_cast<double>(buffer_info.buffer_frames));
			const double end_tmp = std::min(static_cast<double>(buffer_info.buffer_frames) * stop_point.value,
			                                static_cast<double>(buffer_info.buffer_frames));
			if (start_tmp == end_tmp) return;
			const double start = std::min(start_tmp, end_tmp);
			//Need to check the order in case a user feeds us these out of order
			const double end = std::max(start_tmp, end_tmp);

			if (vibrato_rate.value > 0.0f && vibrato_depth.value > 0.0f)
			{
				vibrato_phasor_->set_rate(vibrato_rate.value, samplerate);
				vibrato_phasor_->perform(glisson_temp);
				GfSyn::ChevyshevSin<SigType, Blocksize>(sample_delta_temp, glisson_temp);
				auto depth = vibrato_depth.value;
				std::transform(sample_delta_temp, sample_delta_temp + Blocksize, fm, sample_delta_temp,
				               [depth](auto a, auto fm) { return gf_utils::pitch_to_rate(fm + a * depth * 0.5f); });
			}
			else
			{
				for (int i = 0; i < size; i++)
				{
					sample_delta_temp[i] = gf_utils::pitch_to_rate(fm[i]);
				}
			}

			if (glisson.mode == gf_buffer_mode::normal && glisson_rows.value >= 1)
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
			SigType last_position = source_sample;
			for (int i = 1; i < size; i++)
			{
				sample_positions[i] = last_position + sample_delta_temp[i - 1];
				last_position = sample_positions[i];
			}
			source_sample = sample_positions[size - 1] + sample_delta_temp[size - 1];
			source_sample = gf_utils::pong(source_sample, start, end, fold);

			for (int i = 0; i < size; i++)
			{
				sample_positions[i] = gf_utils::pong(sample_positions[i], start, end, fold);
			}
		}


		void stream_set(const int max_grains, const gf_stream_set_type mode, const int nstreams)
		{
			switch (mode)
			{
			case gf_stream_set_type::automatic_streams:
				{
					stream = (g_) % (nstreams);
					break;
				}
			case gf_stream_set_type::per_streams:
				{
					stream = (g_) / (nstreams);
					break;
				}
			case gf_stream_set_type::random_streams:
				{
					std::random_device rd;
					stream = rd() % (nstreams);
					break;
				}
			case gf_stream_set_type::manual_streams:
				{
					stream = (max_grains - 1 + nstreams) % (nstreams - 1);
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
