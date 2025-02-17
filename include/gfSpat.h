#include <map>
#include <array>
#include <algorithm>
#include <mutex>
#include "gfUtils.h"

namespace Grainflow
{
	enum spat_pan_mode
	{
		vbap = 0,
		dbap = 1,
	};

	template <size_t InternalBlock, typename sigtype = double>

	class gf_spat_pan
	{
	public:
		void set_source_position(int sourceId, std::array<float, 3>& position)
		{
			sourcePositionMap_[sourceId] = position;
			update_source_gains(sourceId, spat_pan_mode::dbap);
		}

		void set_speaker_position(int speakerId, std::array<float, 3>& position)
		{
			speakerPositionMap_[speakerId] = position;
		}

		void clear_speaker_position()
		{
			speakerPositionMap_.clear();
			std::lock_guard<std::mutex> _lock(update_gain_lock_);
			sourceToSpeakerGainMap_.clear();
		}

		void clear_source_positions()
		{
			sourcePositionMap_.clear();
			std::lock_guard<std::mutex> _lock(update_gain_lock_);
			sourceToSpeakerGainMap_.clear();
		}

		void recalculate_all_gains()
		{
			for (auto& source : sourcePositionMap_)
			{
				update_source_gains(source.first, spat_pan_mode::dbap);
			}
		}

	private:
		std::map<int, std::array<float, 3>> sourcePositionMap_;
		std::map<int, std::array<float, 3>> speakerPositionMap_;
		std::map<int, std::map<int, float>> sourceToSpeakerGainMap_;
		std::mutex update_gain_lock_;

		float distance_thresh = 2;
		float n_speakers = 3;
		float exponent = 1;


		void update_source_gains(int sourceId, spat_pan_mode mode)
		{
			if (sourcePositionMap_.find(sourceId) == sourcePositionMap_.end()) { return; }
			switch (mode)
			{
			case spat_pan_mode::dbap:
				set_volume_dbap(sourceId, sourcePositionMap_, speakerPositionMap_, sourceToSpeakerGainMap_);
				break;
			case spat_pan_mode::vbap:
				set_volume_vbap(sourceId, sourcePositionMap_, speakerPositionMap_, sourceToSpeakerGainMap_);
				break;
			}
		}


		void set_volume_dbap(const int sourceId,
		                     std::map<int, std::array<float, 3>>& sources,
		                     std::map<int, std::array<float, 3>>& speakers,
		                     std::map<int, std::map<int, float>>& gain_map)
		{
			if (speakers.size() <= 0) return;
			//We need to check if the gain map exists, then create it
			auto source_to_speaker_map = std::map<int, float>{};
			std::map<int, float> distance_map;
			auto source_position = sources[sourceId];
			float totalDistance = 0;
			for (auto& speaker : speakers)
			{
				distance_map[speaker.first] = gf_utils::distance_3d(source_position, speaker.second);
			}
			std::vector<std::pair<int, float>> distance_vec(distance_map.begin(), distance_map.end());

			std::sort(distance_vec.begin(), distance_vec.end(), [](auto& a, auto& b)
			{
				return a.second < b.second;
			});
			for (auto& entry : distance_vec)
			{
				auto& distance = entry.second;
				if (distance_thresh > 0 && distance > distance_thresh) { break; }
				source_to_speaker_map[entry.first] = std::pow(1 - distance / distance_thresh, exponent);
			}
			std::lock_guard<std::mutex> _lock(update_gain_lock_);
			gain_map[sourceId] = source_to_speaker_map;
		}

		void set_volume_vbap(const int sourceId,
		                     std::map<int, std::array<float, 3>>& sources,
		                     std::map<int, std::array<float, 3>>& speakers,
		                     std::map<int, std::map<int, float>>& gain_map
		)
		{
			if (speakers.size() <= 0) return;
			//We need to check if the gain map exists, then create it
			auto source_to_speaker_map = std::map<int, float>{};
			std::map<int, float> distance_map;
			auto source_position = sources[sourceId];
			float totalDistance = 0;

			for (auto& speaker : speakers)
			{
				distance_map[speaker.first] = gf_utils::distance_3d(source_position, speaker.second);
			}
			std::vector<std::pair<int, float>> distance_vec(distance_map.begin(), distance_map.end());

			std::sort(distance_vec.begin(), distance_vec.end(), [](auto& a, auto& b)
			{
				return a.second < b.second;
			});
			int count = 0;
			for (auto& entry : distance_vec)
			{
				++count;
				auto& distance = entry.second;
				//if (distance_thresh > 0 && distance > distance_thresh) { break; }
				totalDistance += distance;
				if (count >= n_speakers) break;
			}


			if (totalDistance <= 0)
			{
				std::lock_guard<std::mutex> _lock(update_gain_lock_);
				gain_map[sourceId] = source_to_speaker_map;
				return;
			}
			count = 0;
			for (auto& entry : distance_vec)
			{
				++count;
				auto& id = entry.first;
				auto& distance = entry.second;
				if (distance_thresh > 0 && distance > distance_thresh) { break; }
				source_to_speaker_map[id] = std::pow(1 - distance / totalDistance, exponent);
				if (count >= n_speakers) break;
			}
			std::lock_guard<std::mutex> _lock(update_gain_lock_);
			gain_map[sourceId] = source_to_speaker_map;
		}

		void perform_pan(sigtype** __restrict input, sigtype** __restrict output, const int inputChannels,
		                 const int outputChannels, const int blockSize)
		{
			if (sourceToSpeakerGainMap_.empty()) { return; }
			std::lock_guard<std::mutex> _lock(update_gain_lock_);
			for (auto& source_pair : sourceToSpeakerGainMap_)
			{
				auto i = source_pair.first;
				if (i < 0 || i >= inputChannels) { continue; }
				auto& source_map = source_pair.second;
				for (auto& gains_pairs : source_map)
				{
					const auto output_index = gains_pairs.first;
					if (output_index >= outputChannels) { continue; }
					const auto gain = gains_pairs.second;
					for (int j = 0; j < blockSize / InternalBlock; ++j)
					{
						const auto subOutBuffer = &output[output_index][j * InternalBlock];
						const auto subInBuffer = &input[i][j * InternalBlock];
						for (int k = 0; k < InternalBlock; ++k)
						{
							subOutBuffer[k] += subInBuffer[k] * gain;
						}
					}
				}
			}
		}

	public:
		void process(sigtype** input, sigtype** output, const int inputChannels, const int outputChannels,
		             const int blockSize)
		{
			perform_pan(input, output, inputChannels, outputChannels, blockSize);
		}
	};
}
