#include "gfIBufferReader.h"
#include <atomic>
namespace Grainflow
{
	template <typename T, size_t INTERNALBLOCK, typename SigType = double>
	class gfRecorder
	{
	private:
		gf_io_config<SigType> config_{};
		gf_buffer_info buffer_info_{};
		size_t write_position_ = 0;
		std::array<SigType, INTERNALBLOCK> temp_{0.0};
		gf_i_buffer_reader<T, SigType> buffer_reader_{};

	public:
		std::array<std::atomic<float>,2> recRange {0.0, 1.0};
		SigType write_position_norm = 0.0;
		SigType write_position_ms = 0.0;
		int write_position_samps = 0;
		bool sync = false;
		bool freeze = false;
		bool state = false;
		float overdub = 0;
		size_t samplerate = 48000;

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
				write_position_ = buffer_info_.buffer_frames*(gf_utils::mod(time_override, 1));
			}
			if (buffer_info_.buffer_frames == 0) return;
			int sampleRange = buffer_info_.buffer_frames*recRangeSize;
		    int sampleBase = buffer_info_.buffer_frames * recBase;
			int increment = INTERNALBLOCK*recRangeSign;
			for (int b = 0; b < blocks; ++b)
			{
				for (int c = 0; c < channels; ++c)
				{
					buffer_reader_.write_buffer(buffer, c, &(input[c][b * INTERNALBLOCK]), temp_.data(),
					                            write_position_, overdub, INTERNALBLOCK);
				}

				if (!freeze)
				{
					for (int i = 0; i < INTERNALBLOCK; ++i)
					{
						recorded_head_out[b * INTERNALBLOCK + i] = static_cast<float>((write_position_ + i) %
							buffer_info_.buffer_frames) / buffer_info_.buffer_frames;
					}
					write_position_ = ((write_position_ + increment) + sampleRange) % sampleRange+sampleBase;
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
