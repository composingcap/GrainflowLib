#pragma once
#include <map>
#include <array>
#include <algorithm>
#include <mutex>
#include <cmath>
#include "gfUtils.h"
#include "gfSafVbap.h"

namespace Grainflow
{
	enum class spat_pan_mode : int
	{
		vbap = 0,
		dbap = 1,
		enum_count,
	};

	template <size_t InternalBlock, typename sigtype = double>

	class gf_spat_pan
	{
	public:
		void set_source_position(int sourceId, std::array<float, 3>& position)
		{
			sourcePositionMap_[sourceId] = position;
			update_source_gains(sourceId, pan_mode);
		}

		void set_speaker_position(int speakerId, std::array<float, 3>& position)
		{
			speakerPositionMap_[speakerId] = position;
			vbap_speakers_dirty_ = true;
		}

		void clear_speaker_position()
		{
			sourceToSpeakerGainMapLast_.clear();
			std::lock_guard<std::mutex> _lock(update_gain_lock_);
			sourceToSpeakerGainMap_.clear();
			dirtyMap_.clear();
			speakerPositionMap_.clear();
			vbap_speakers_dirty_ = true;
		}

		void clear_source_positions()
		{
			sourcePositionMap_.clear();
			std::lock_guard<std::mutex> _lock(update_gain_lock_);
			sourceToSpeakerGainMap_.clear();
			dirtyMap_.clear();
			sourceToSpeakerGainMapLast_.clear();
		}

		void set_pan_mode(spat_pan_mode mode)
		{
			if (mode != pan_mode && (mode == spat_pan_mode::vbap))
			{
				vbap_speakers_dirty_ = true;
			}
			pan_mode = mode;
		}

		void recalculate_all_gains(bool clearHistory = false)
		{
			if (clearHistory)
			{
				std::lock_guard<std::mutex> _lock(update_gain_lock_);
				dirtyMap_.clear();
				sourceToSpeakerGainMapLast_.clear();
			}
			for (auto& source : sourcePositionMap_)
			{
				update_source_gains(source.first, pan_mode);
			}
		}

		void process(sigtype** input, sigtype** output, const int inputChannels, const int outputChannels,
		             const int blockSize)
		{
			perform_pan(input, output, inputChannels, outputChannels, blockSize);
		}

		void get_data_outputs(std::vector<float>& speakers, std::vector<float>& grains,
		                      std::map<int, std::array<float, 3>>& speakerPositions,
		                      std::map<int, std::array<float, 3>>& grainPositions)
		{
			std::lock_guard<std::mutex> _lock(update_gain_lock_);
			speakers.assign(speaker_amps_.begin(), speaker_amps_.begin() + channelCount_);
			grains.assign(grain_amps_.begin(), grain_amps_.begin() + grainCount_);
			grainPositions = sourcePositionMap_;
			speakerPositions = speakerPositionMap_;
		}

		void set_vbap_spread(float spread_deg)
		{
			vbap_spread_ = spread_deg;
			saf_vbap_.set_spread(spread_deg);
		}

		float get_vbap_spread() const { return vbap_spread_; }

	private:
		// Convert Cartesian XYZ to spherical azimuth/elevation (degrees)
		static void cartesian_to_spherical(float x, float y, float z, float& azimuth_deg, float& elevation_deg)
		{
			float r = std::sqrt(x * x + y * y + z * z);
			if (r < 1e-6f)
			{
				azimuth_deg = 0.0f;
				elevation_deg = 0.0f;
				return;
			}
			// Azimuth: angle in XY plane from positive X axis
			// SAF convention: 0 = front, positive = left, negative = right
			azimuth_deg = std::atan2(x, y) * (180.0f / 3.14159265358979f);
			// Elevation: angle from XY plane
			elevation_deg = std::asin(z / r) * (180.0f / 3.14159265358979f);
		}

		void rebuild_vbap_speaker_layout(spat_pan_mode mode)
		{
			if (speakerPositionMap_.empty()) return;

			// Build ordered speaker list and convert to spherical coordinates
			vbap_speaker_order_.clear();
			std::vector<typename gf_saf_vbap<InternalBlock, sigtype>::speaker_config> speakers;

			for (const auto& [id, pos] : speakerPositionMap_)
			{
				vbap_speaker_order_.push_back(id);
				float az, el;
				cartesian_to_spherical(
					pos[0] * dim_mask[0],
					pos[1] * dim_mask[1],
					pos[2] * dim_mask[2],
					az, el
				);
				speakers.push_back({az, el});
			}

			if (mode == spat_pan_mode::vbap)
			{
				float max_el = 0;
				float min_el = 0;
				for (auto& speaker : speakers){
					max_el = std::max(max_el,speaker.elevation_deg);
					min_el = std::min(min_el, speaker.elevation_deg);
				}
				if (max_el - min_el < 1.0f){
					std::vector<float> azimuths;
					for (const auto& s : speakers)
					{
						azimuths.push_back(s.azimuth_deg);
					}
					saf_vbap_.set_speaker_layout_2d(azimuths, vbap_azimuth_resolution_);
				}
				else{
					saf_vbap_.set_speaker_layout_3d(speakers, vbap_azimuth_resolution_, vbap_elevation_resolution_);
				}
			}


			vbap_speakers_dirty_ = false;
		}

		void update_source_gains(int sourceId, spat_pan_mode mode)
		{
			if (sourcePositionMap_.find(sourceId) == sourcePositionMap_.end()) { return; }
			switch (mode)
			{
			case spat_pan_mode::dbap:
				set_volume_dbap(sourceId, sourcePositionMap_, speakerPositionMap_, sourceToSpeakerGainMap_);
				break;
			case spat_pan_mode::vbap:
				set_volume_vbap(sourceId, sourcePositionMap_, speakerPositionMap_, sourceToSpeakerGainMap_, mode);
				break;
			case spat_pan_mode::enum_count:
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
			std::array<float, 3> source_position;
			std::array<float, 3> speaker_position;
			float totalDistance = 0;

			for (int i = 0; i < source_position.size(); ++i)
			{
				source_position[i] = dim_mask[i] * sources[sourceId][i];
			}

			for (auto& speaker : speakers)
			{
				for (int i = 0; i < speaker_position.size(); ++i)
				{
					speaker_position[i] = dim_mask[i] * speaker.second[i];
				}
				distance_map[speaker.first] = gf_utils::distance_3d(source_position, speaker.second);
			}
			std::vector<std::pair<int, float>> distance_vec(distance_map.begin(), distance_map.end());

			std::sort(distance_vec.begin(), distance_vec.end(), [](auto& a, auto& b)
			{
				return a.second < b.second;
			});
			int counter = 0;
			for (auto& entry : distance_vec)
			{
				if (counter >= n_speakers && n_speakers > 0) { break; }
				auto& distance = entry.second;
				if (distance_thresh > 0 && distance > distance_thresh) { break; }
				source_to_speaker_map[entry.first] = std::pow(1 - distance / distance_thresh, exponent);
				++counter;
			}
			std::lock_guard<std::mutex> _lock(update_gain_lock_);
			gain_map[sourceId] = source_to_speaker_map;
			dirtyMap_[sourceId] = true;
		}

		void set_volume_vbap(const int sourceId,
		                     std::map<int, std::array<float, 3>>& sources,
		                     std::map<int, std::array<float, 3>>& speakers,
		                     std::map<int, std::map<int, float>>& gain_map,
		                     spat_pan_mode mode
		)
		{
			if (speakers.empty()) return;

			// Rebuild SAF VBAP speaker layout if speakers changed
			if (vbap_speakers_dirty_)
			{
				rebuild_vbap_speaker_layout(mode);
			}

			if (!saf_vbap_.is_initialized()) return;

			// Convert source position to spherical coordinates
			const auto& src_pos = sources[sourceId];
			float az, el;
			cartesian_to_spherical(
				src_pos[0] * dim_mask[0],
				src_pos[1] * dim_mask[1],
				src_pos[2] * dim_mask[2],
				az, el
			);

			// Get gains from SAF VBAP
			std::vector<float> gains;
			if (!saf_vbap_.get_gains(az, el, gains))
			{
				return;
			}

			// Map gains back to speaker IDs
			auto source_to_speaker_map = std::map<int, float>{};
			for (size_t i = 0; i < vbap_speaker_order_.size() && i < gains.size(); ++i)
			{
				if (gains[i] > 1e-6f)
				{
					source_to_speaker_map[vbap_speaker_order_[i]] = gains[i];
				}
			}

			std::lock_guard<std::mutex> _lock(update_gain_lock_);
			gain_map[sourceId] = source_to_speaker_map;
			dirtyMap_[sourceId] = true;
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

				if (!dirtyMap_[i])
				{
					// Pan without interpolation 
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
					continue;
				}

				//Pan with interpolation 
				float mix_increment = 1.0f / blockSize;
				if (sourceToSpeakerGainMapLast_.find(source_pair.first) != sourceToSpeakerGainMapLast_.end())
				{
					auto source_map_last = sourceToSpeakerGainMapLast_[source_pair.first];
					for (auto& gains_pairs : source_map_last)
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
								float mix = 1 - mix_increment * (InternalBlock * j + k);
								subOutBuffer[k] += subInBuffer[k] * gain * (mix);
							}
						}
					}
				}

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
							float mix = mix_increment * (InternalBlock * j + k);
							subOutBuffer[k] += subInBuffer[k] * gain * mix;
						}
					}
				}
				sourceToSpeakerGainMapLast_[i] = source_map;
				dirtyMap_[i] = false;
			}
			channelCount_ = outputChannels;
			grainCount_ = inputChannels;
			if (outputChannels > speaker_amps_.size()) { speaker_amps_.resize(outputChannels); };
			//todo optimize 
			for (int i = 0; i < outputChannels; ++i)
			{
				auto maxVal = std::abs(output[i][0]);
				for (int j = 0; j < blockSize / InternalBlock; ++j)
				{
					const auto subOutBuffer = &output[i][j * InternalBlock];
					maxVal = std::max(maxVal, std::abs(subOutBuffer[0]));
					//for (int k = 0; k < InternalBlock; ++k)
					//{
					//	maxVal = std::max(maxVal, std::abs(subOutBuffer[k]));
					//}
				}
				speaker_amps_[i] = maxVal;
			}
			if (inputChannels > grain_amps_.size()) { grain_amps_.resize(inputChannels); };

			for (int i = 0; i < inputChannels; ++i)
			{
				auto maxVal = std::abs(input[i][0]);
				for (int j = 0; j < blockSize / InternalBlock; ++j)
				{
					const auto subOutBuffer = &input[i][j * InternalBlock];
					maxVal = std::max(maxVal, std::abs(subOutBuffer[0]));
					//for (int k = 0; k < InternalBlock; ++k)
					//{
					//	maxVal = std::max(maxVal, std::abs(subOutBuffer[k]));
					//}
				}
				grain_amps_[i] = maxVal;
			}
		}

	private:
		std::map<int, std::array<float, 3>> sourcePositionMap_;
		std::map<int, std::array<float, 3>> speakerPositionMap_;
		std::map<int, std::map<int, float>> sourceToSpeakerGainMap_;
		std::map<int, std::map<int, float>> sourceToSpeakerGainMapLast_;
		std::map<int, bool> dirtyMap_;
		std::vector<float> speaker_amps_;
		std::vector<float> grain_amps_;
		std::mutex update_gain_lock_;
		int channelCount_{0};
		int grainCount_{0};

		// SAF VBAP members
		gf_saf_vbap<InternalBlock, sigtype> saf_vbap_;
		std::vector<int> vbap_speaker_order_;
		bool vbap_speakers_dirty_{true};
		float vbap_spread_{0.0f};
		int vbap_azimuth_resolution_{2};
		int vbap_elevation_resolution_{2};

	public:
		float distance_thresh = 2;
		int n_speakers = 3;
		float exponent = 1;
		spat_pan_mode pan_mode;
		std::array<float, 3> dim_mask{1, 1, 1};
	};
}
