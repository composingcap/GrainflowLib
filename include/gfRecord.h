#include "gfIBufferReader.h"
#include <atomic>

#include "gfFilters.h"

namespace Grainflow
{
	template <typename T, size_t INTERNALBLOCK, typename SigType = double>
	class gfRecorder
	{
	private:
		struct filter_data
		{
		public:
			biquad<SigType> sample_filter;
			biquad<SigType> od_filter;
			biquad_params<SigType> filter_params;
			float overdub;
		};

		gf_io_config<SigType> config_{};
		gf_buffer_info buffer_info_{};
		size_t write_position_ = 0;
		std::array<std::array<SigType, INTERNALBLOCK>, 5> temp_{0.0};

		gf_i_buffer_reader<T, SigType> buffer_reader_{};

		std::vector<filter_data> filter_data_;

	public:
		std::array<std::atomic<float>, 2> recRange{0.0, 1.0};
		SigType write_position_norm = 0.0;
		SigType write_position_ms = 0.0;
		int write_position_samps = 0;
		bool sync = false;
		bool freeze = false;
		bool state = false;
		float overdub = 0;
		size_t samplerate = 48000;

	private:
		void write_simple(SigType** input, T* buffer, int block, int channels)
		{
			for (int c = 0; c < channels; ++c)
			{
				const auto samps = input[c] + block * INTERNALBLOCK;
				if (overdub <= 0)
				{
					buffer_reader_.write_buffer(buffer, c, samps, write_position_, INTERNALBLOCK);
					return;
				}

				buffer_reader_.read_buffer(buffer, c, temp_[0].data(), write_position_, INTERNALBLOCK);
				const auto old_mix = overdub;
				const auto new_mix = 1 - overdub;
				std::transform(samps, samps + (INTERNALBLOCK - 1), temp_[0].begin(), temp_[0].begin(),
				               [old_mix, new_mix](auto a, auto b)
				               {
					               return a * new_mix + b * old_mix;
				               });
				buffer_reader_.write_buffer(buffer, c, temp_[0].data(), write_position_, INTERNALBLOCK);
			}
		}

		//TODO try not to use 5 temp buffers
		void write_with_filters(SigType** input, T* buffer, int block, int channels)
		{
			for (int c = 0; c < channels; ++c)
			{
				const auto samps = input[c] + block * INTERNALBLOCK;
				std::fill(temp_[2].begin(), temp_[2].end(), 0);
				buffer_reader_.read_buffer(buffer, c, temp_[0].data(), write_position_, INTERNALBLOCK);
				for (auto& filter : filter_data_)
				{
					const auto old_mix = 1 - filter.overdub;
					if (old_mix > 0)
					{
						filter.od_filter.perform(temp_[0].data(), INTERNALBLOCK, filter.filter_params,
						                         temp_[1].data());


						std::transform(temp_[1].begin(), temp_[1].end(), temp_[2].begin(),
						               temp_[2].begin(), [old_mix](auto a, auto b)
						               {
							               return a * old_mix + b;
						               });

						std::transform(temp_[1].begin(), temp_[1].end(), temp_[3].begin(),
						               temp_[3].begin(), [](auto a, auto b)
						               {
							               return a + b;
						               });
					}

					const auto new_mix = filter.overdub;
					if (new_mix > 0)
					{
						filter.sample_filter.
						       perform(samps, INTERNALBLOCK, filter.filter_params, temp_[1].data());
						std::transform(temp_[1].begin(), temp_[1].end(), temp_[2].begin(),
						               temp_[2].begin(), [old_mix](auto a, auto b)
						               {
							               return a * old_mix + b;
						               });
						std::transform(temp_[1].begin(), temp_[1].end(), temp_[4].begin(),
						               temp_[4].begin(), [](auto a, auto b)
						               {
							               return a + b;
						               });
					}
				}
				const auto old_mix = 1 - overdub;
				std::transform(temp_[1].begin(), temp_[1].end(), temp_[3].begin(),
				               temp_[1].begin(), [old_mix](auto a, auto b)
				               {
					               return (a - b) * old_mix;
				               });
				const auto new_mix = overdub;
				std::transform(samps, samps + (INTERNALBLOCK - 1), temp_[4].begin(),
				               temp_[3].begin(), [new_mix](auto a, auto b)
				               {
					               return (a - b) * new_mix;
				               });

				std::transform(temp_[1].begin(), temp_[1].end(), temp_[3].begin(),
				               temp_[3].begin(), [](auto a, auto b) { return a + b; });

				std::transform(temp_[3].begin(), temp_[3].end(), temp_[2].begin(),
				               temp_[2].begin(), [](auto a, auto b) { return a + b; });


				buffer_reader_.write_buffer(buffer, c, temp_[2].data(), write_position_, INTERNALBLOCK);
			}
		}

	public:
		gfRecorder(gf_i_buffer_reader<T, SigType> buffer_reader)
		{
			buffer_reader_ = buffer_reader;
			config_.livemode = true;
		}

		~gfRecorder()
		{
		}

		void get_position(SigType& position_samps, SigType& position_norm, SigType& position_ms)
		{
			position_samps = write_position_samps;
			position_norm = write_position_norm;
			position_ms = write_position_ms;
		}

		void process(SigType** __restrict input, const SigType time_override, T* buffer, int frames, int channels,
		             SigType* __restrict recorded_head_out)
		{
			const auto blocks = frames / INTERNALBLOCK;
			float recBase = recRange[0].load();
			float rangeMax = recRange[1].load();
			float recRangeSize = std::abs(rangeMax - recBase);
			float recRangeSign = rangeMax < recBase ? -1 : 1;


			if (!state)
			{
				for (int b = 0; b < blocks; ++b)
				{
					if (buffer_info_.buffer_frames == 0)
					{
						for (int i = 0; i < INTERNALBLOCK; ++i)
						{
							recorded_head_out[b * INTERNALBLOCK + i] = 0;
						}
						return;
					}
					if (!freeze)
					{
						for (int i = 0; i < INTERNALBLOCK; ++i)
						{
							recorded_head_out[b * INTERNALBLOCK + i] = static_cast<float>(write_position_) /
								buffer_info_.
								buffer_frames;
						}
					}
					else
					{
						for (int i = 0; i < INTERNALBLOCK; ++i)
						{
							recorded_head_out[b * INTERNALBLOCK + i] = write_position_norm;
						}
					}
				}
				return;
			}

			auto success = buffer_reader_.update_buffer_info(buffer, config_, &buffer_info_);
			if (!success || buffer_info_.buffer_frames <= 0)
			{
				write_position_ = 0;
				write_position_samps = 0;
				write_position_norm = 0;
				write_position_ms = 0;
				return;
			}

			if (sync)
			{
				write_position_ = buffer_info_.buffer_frames * (gf_utils::mod(time_override, 1));
			}
			if (buffer_info_.buffer_frames == 0) return;
			int sampleRange = buffer_info_.buffer_frames * recRangeSize;
			int sampleBase = buffer_info_.buffer_frames * recBase;
			int increment = INTERNALBLOCK * recRangeSign;
			for (int b = 0; b < blocks; ++b)
			{
				if (filter_data_.empty())
				{
					write_simple(input, buffer, b, channels);
				}
				else
				{
					write_with_filters(input, buffer, b, channels);
				}

				if (!freeze)
				{
					for (int i = 0; i < INTERNALBLOCK; ++i)
					{
						recorded_head_out[b * INTERNALBLOCK + i] = static_cast<float>((write_position_ + i) %
							buffer_info_.buffer_frames) / buffer_info_.buffer_frames;
					}
					write_position_ = ((write_position_ + increment) + sampleRange) % sampleRange + sampleBase;
					write_position_samps = write_position_;
					write_position_norm = static_cast<float>((write_position_samps + INTERNALBLOCK) % buffer_info_.
						buffer_frames) / buffer_info_.buffer_frames;
					write_position_ms = (write_position_samps + INTERNALBLOCK) * 1000 / samplerate;
				}
				else
				{
					for (int i = 0; i < INTERNALBLOCK; ++i)
					{
						recorded_head_out[b * INTERNALBLOCK + i] = write_position_norm;
					}

					write_position_ = ((write_position_ + increment) + sampleRange) % sampleRange + sampleBase;
				}
			}
		}
	};
}
