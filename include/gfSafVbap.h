#pragma once
#include <vector>
#include <array>
#include <cmath>
#include <mutex>
#include <algorithm>

extern "C" {
#include "../lib/Spatial_Audio_Framework/framework/modules/saf_vbap/saf_vbap.h"
}

namespace Grainflow
{
	/**
	 * @brief VBAP panner wrapper using SAF (Spatial Audio Framework)
	 *
	 * Provides both 2D and 3D Vector Base Amplitude Panning with optional
	 * spreading (MDAP). Uses a precomputed gain table for efficient real-time
	 * lookup.
	 *
	 * @tparam InternalBlock DSP block size for optimization
	 * @tparam SigType Signal type (float or double)
	 */
	template <size_t InternalBlock, typename SigType = double>
	class gf_saf_vbap
	{
	public:
		struct speaker_config
		{
			float azimuth_deg;   // -180 to 180
			float elevation_deg; // -90 to 90
		};

		gf_saf_vbap() = default;

		~gf_saf_vbap()
		{
			free_gain_table();
		}

		// Non-copyable due to raw pointer management
		gf_saf_vbap(const gf_saf_vbap&) = delete;
		gf_saf_vbap& operator=(const gf_saf_vbap&) = delete;

		// Movable
		gf_saf_vbap(gf_saf_vbap&& other) noexcept
		{
			*this = std::move(other);
		}

		gf_saf_vbap& operator=(gf_saf_vbap&& other) noexcept
		{
			if (this != &other)
			{
				free_gain_table();
				gain_table_ = other.gain_table_;
				gain_table_compressed_ = other.gain_table_compressed_;
				gain_table_indices_ = other.gain_table_indices_;
				n_table_entries_ = other.n_table_entries_;
				n_triangles_ = other.n_triangles_;
				n_speakers_ = other.n_speakers_;
				speaker_dirs_ = std::move(other.speaker_dirs_);
				azimuth_resolution_ = other.azimuth_resolution_;
				elevation_resolution_ = other.elevation_resolution_;
				spread_ = other.spread_;
				use_3d_ = other.use_3d_;
				omit_large_triangles_ = other.omit_large_triangles_;
				enable_dummies_ = other.enable_dummies_;
				is_initialized_ = other.is_initialized_;
				current_gains_ = std::move(other.current_gains_);
				target_gains_ = std::move(other.target_gains_);

				other.gain_table_ = nullptr;
				other.gain_table_compressed_ = nullptr;
				other.gain_table_indices_ = nullptr;
				other.is_initialized_ = false;
			}
			return *this;
		}

		/**
		 * @brief Configure the speaker layout for 3D VBAP
		 * @param speakers Vector of speaker configurations (azimuth/elevation in degrees)
		 * @param azimuth_res Azimuth resolution for gain table (degrees, typically 1-5)
		 * @param elevation_res Elevation resolution for gain table (degrees, typically 1-5)
		 */
		void set_speaker_layout_3d(const std::vector<speaker_config>& speakers,
		                           int azimuth_res = 1,
		                           int elevation_res = 1)
		{
			if (speakers.empty()) return;

			std::lock_guard<std::mutex> lock(mutex_);

			n_speakers_ = static_cast<int>(speakers.size());
			azimuth_resolution_ = azimuth_res;
			elevation_resolution_ = elevation_res;
			use_3d_ = true;

			// Convert to flat array format expected by SAF: [az1, el1, az2, el2, ...]
			speaker_dirs_.resize(n_speakers_ * 2);
			for (int i = 0; i < n_speakers_; ++i)
			{
				speaker_dirs_[i * 2] = speakers[i].azimuth_deg;
				speaker_dirs_[i * 2 + 1] = speakers[i].elevation_deg;
			}

			rebuild_gain_table();
		}

		/**
		 * @brief Configure the speaker layout for 2D VBAP (horizontal plane only)
		 * @param azimuths Vector of speaker azimuths in degrees (-180 to 180)
		 * @param azimuth_res Azimuth resolution for gain table (degrees, typically 1-5)
		 */
		void set_speaker_layout_2d(const std::vector<float>& azimuths,
		                           int azimuth_res = 1)
		{
			if (azimuths.empty()) return;

			std::lock_guard<std::mutex> lock(mutex_);

			n_speakers_ = static_cast<int>(azimuths.size());
			azimuth_resolution_ = azimuth_res;
			use_3d_ = false;

			// For 2D, SAF still expects az/el pairs but elevation should be 0
			speaker_dirs_.resize(n_speakers_ * 2);
			for (int i = 0; i < n_speakers_; ++i)
			{
				speaker_dirs_[i * 2] = azimuths[i];
				speaker_dirs_[i * 2 + 1] = 0.0f;
			}

			rebuild_gain_table();
		}

		/**
		 * @brief Set spreading factor (0 = pure VBAP, >0 = MDAP spreading)
		 * @param spread_deg Spreading in degrees
		 */
		void set_spread(float spread_deg)
		{
			if (spread_deg != spread_)
			{
				std::lock_guard<std::mutex> lock(mutex_);
				spread_ = std::max(0.0f, spread_deg);
				if (is_initialized_)
				{
					rebuild_gain_table();
				}
			}
		}

		/**
		 * @brief Set whether to omit large triangles in triangulation
		 */
		void set_omit_large_triangles(bool omit)
		{
			if (omit != (omit_large_triangles_ != 0))
			{
				std::lock_guard<std::mutex> lock(mutex_);
				omit_large_triangles_ = omit ? 1 : 0;
				if (is_initialized_)
				{
					rebuild_gain_table();
				}
			}
		}

		/**
		 * @brief Enable dummy speakers at poles for incomplete layouts
		 */
		void set_enable_dummies(bool enable)
		{
			if (enable != (enable_dummies_ != 0))
			{
				std::lock_guard<std::mutex> lock(mutex_);
				enable_dummies_ = enable ? 1 : 0;
				if (is_initialized_)
				{
					rebuild_gain_table();
				}
			}
		}

		/**
		 * @brief Get gains for a source at the specified direction
		 * @param azimuth_deg Source azimuth in degrees (-180 to 180)
		 * @param elevation_deg Source elevation in degrees (-90 to 90), ignored for 2D
		 * @param gains Output vector of gains per speaker (will be resized)
		 * @return true if gains were successfully computed
		 */
		bool get_gains(float azimuth_deg, float elevation_deg, std::vector<float>& gains)
		{
			std::lock_guard<std::mutex> lock(mutex_);

			if (!is_initialized_ || gain_table_ == nullptr)
			{
				return false;
			}

			gains.resize(n_speakers_);

			if (use_3d_)
			{
				return get_gains_3d(azimuth_deg, elevation_deg, gains.data());
			}
			else
			{
				return get_gains_2d(azimuth_deg, gains.data());
			}
		}

		/**
		 * @brief Get gains using compressed table (more memory efficient, amplitude normalized)
		 * @param azimuth_deg Source azimuth in degrees
		 * @param elevation_deg Source elevation in degrees (ignored for 2D)
		 * @param gains Output array of 3 gains for active speakers
		 * @param speaker_indices Output array of 3 speaker indices
		 * @return true if successful
		 */
		bool get_gains_compressed(float azimuth_deg, float elevation_deg,
		                          float gains[3], int speaker_indices[3])
		{
			std::lock_guard<std::mutex> lock(mutex_);

			if (!is_initialized_ || gain_table_compressed_ == nullptr)
			{
				return false;
			}

			int idx = compute_table_index(azimuth_deg, elevation_deg);
			if (idx < 0 || idx >= n_table_entries_)
			{
				return false;
			}

			for (int i = 0; i < 3; ++i)
			{
				gains[i] = gain_table_compressed_[idx * 3 + i];
				speaker_indices[i] = gain_table_indices_[idx * 3 + i];
			}
			return true;
		}

		/**
		 * @brief Process audio: pan input source to output speakers with interpolation
		 * @param input Input buffer (mono source)
		 * @param output Output buffers (one per speaker)
		 * @param block_size Number of samples to process
		 * @param azimuth_deg Source azimuth
		 * @param elevation_deg Source elevation (ignored for 2D)
		 */
		void process(const SigType* input, SigType** output, int block_size,
		             float azimuth_deg, float elevation_deg = 0.0f)
		{
			std::lock_guard<std::mutex> lock(mutex_);

			if (!is_initialized_ || gain_table_ == nullptr)
			{
				return;
			}

			// Get target gains
			target_gains_.resize(n_speakers_);
			if (use_3d_)
			{
				get_gains_3d(azimuth_deg, elevation_deg, target_gains_.data());
			}
			else
			{
				get_gains_2d(azimuth_deg, target_gains_.data());
			}

			// Initialize current gains if needed
			if (current_gains_.size() != static_cast<size_t>(n_speakers_))
			{
				current_gains_ = target_gains_;
			}

			// Process with linear interpolation
			const float increment = 1.0f / static_cast<float>(block_size);

			for (int speaker = 0; speaker < n_speakers_; ++speaker)
			{
				const float start_gain = current_gains_[speaker];
				const float end_gain = target_gains_[speaker];
				const float gain_delta = (end_gain - start_gain) * increment;

				float gain = start_gain;
				for (int j = 0; j < block_size / static_cast<int>(InternalBlock); ++j)
				{
					SigType* out_ptr = &output[speaker][j * InternalBlock];
					const SigType* in_ptr = &input[j * InternalBlock];

					for (size_t k = 0; k < InternalBlock; ++k)
					{
						out_ptr[k] += in_ptr[k] * static_cast<SigType>(gain);
						gain += gain_delta;
					}
				}
			}

			current_gains_ = target_gains_;
		}

		/**
		 * @brief Process multiple sources to speakers
		 * @param inputs Array of input buffers (one per source)
		 * @param outputs Array of output buffers (one per speaker)
		 * @param n_sources Number of sources
		 * @param block_size Samples per buffer
		 * @param azimuths Array of source azimuths
		 * @param elevations Array of source elevations (can be nullptr for 2D)
		 */
		void process_multi(SigType** inputs, SigType** outputs, int n_sources,
		                   int block_size, const float* azimuths, const float* elevations = nullptr)
		{
			if (!is_initialized_ || gain_table_ == nullptr)
			{
				return;
			}

			// Clear outputs
			for (int s = 0; s < n_speakers_; ++s)
			{
				std::fill(outputs[s], outputs[s] + block_size, static_cast<SigType>(0));
			}

			// Temporary gains buffer
			std::vector<float> gains(n_speakers_);

			for (int src = 0; src < n_sources; ++src)
			{
				float elev = elevations ? elevations[src] : 0.0f;

				{
					std::lock_guard<std::mutex> lock(mutex_);
					if (use_3d_)
					{
						get_gains_3d(azimuths[src], elev, gains.data());
					}
					else
					{
						get_gains_2d(azimuths[src], gains.data());
					}
				}

				// Add to outputs
				for (int speaker = 0; speaker < n_speakers_; ++speaker)
				{
					const float gain = gains[speaker];
					if (gain <= 0.0f) continue;

					for (int j = 0; j < block_size / static_cast<int>(InternalBlock); ++j)
					{
						SigType* out_ptr = &outputs[speaker][j * InternalBlock];
						const SigType* in_ptr = &inputs[src][j * InternalBlock];

						for (size_t k = 0; k < InternalBlock; ++k)
						{
							out_ptr[k] += in_ptr[k] * static_cast<SigType>(gain);
						}
					}
				}
			}
		}

		// Accessors
		int get_num_speakers() const { return n_speakers_; }
		int get_num_triangles() const { return n_triangles_; }
		int get_table_size() const { return n_table_entries_; }
		bool is_initialized() const { return is_initialized_; }
		float get_spread() const { return spread_; }

	private:
		void free_gain_table()
		{
			if (gain_table_)
			{
				free(gain_table_);
				gain_table_ = nullptr;
			}
			if (gain_table_compressed_)
			{
				delete[] gain_table_compressed_;
				gain_table_compressed_ = nullptr;
			}
			if (gain_table_indices_)
			{
				delete[] gain_table_indices_;
				gain_table_indices_ = nullptr;
			}
			is_initialized_ = false;
		}

		void rebuild_gain_table()
		{
			free_gain_table();

			if (speaker_dirs_.empty() || n_speakers_ <= 0)
			{
				return;
			}
			if (use_3d_)
			{
				//To do fix edge case where 2D speaker setup causes vbap 3d to crash. Options are to auto sense or or to handle the issue
				generateVBAPgainTable3D(
					speaker_dirs_.data(),
					n_speakers_,
					azimuth_resolution_,
					elevation_resolution_,
					omit_large_triangles_,
					enable_dummies_,
					spread_,
					&gain_table_,
					&n_table_entries_,
					&n_triangles_
				);
			}
			else
			{
				int n_pairs = 0;
				generateVBAPgainTable2D(
					speaker_dirs_.data(),
					n_speakers_,
					azimuth_resolution_,
					&gain_table_,
					&n_table_entries_,
					&n_pairs
				);
				n_triangles_ = n_pairs; // For 2D, triangles = pairs
			}

			if (gain_table_ == nullptr)
			{
				// Triangulation failed
				is_initialized_ = false;
				return;
			}

			// Create compressed table for efficient lookup
			if (use_3d_ && n_table_entries_ > 0 && n_speakers_ > 0)
			{
				gain_table_compressed_ = new float[n_table_entries_ * 3];
				gain_table_indices_ = new int[n_table_entries_ * 3];
				compressVBAPgainTable3D(
					gain_table_,
					n_table_entries_,
					n_speakers_,
					gain_table_compressed_,
					gain_table_indices_
				);
			}

			is_initialized_ = true;
			current_gains_.clear(); // Force re-initialization on next process
		}

		int compute_table_index(float azimuth_deg, float elevation_deg) const
		{
			if (use_3d_)
			{
				int n_azi = static_cast<int>(360.0f / azimuth_resolution_ + 0.5f) + 1;
				float azi_wrapped = std::fmod(azimuth_deg + 180.0f, 360.0f);
				if (azi_wrapped < 0) azi_wrapped += 360.0f;
				int azi_idx = static_cast<int>(azi_wrapped / azimuth_resolution_ + 0.5f);
				int elev_idx = static_cast<int>((elevation_deg + 90.0f) / elevation_resolution_ + 0.5f);
				return elev_idx * n_azi + azi_idx;
			}
			else
			{
				float azi_wrapped = std::fmod(azimuth_deg + 180.0f, 360.0f);
				if (azi_wrapped < 0) azi_wrapped += 360.0f;
				return static_cast<int>(azi_wrapped / azimuth_resolution_ + 0.5f);
			}
		}

		bool get_gains_3d(float azimuth_deg, float elevation_deg, float* gains) const
		{
			int idx = compute_table_index(azimuth_deg, elevation_deg);
			if (idx < 0 || idx >= n_table_entries_)
			{
				std::fill(gains, gains + n_speakers_, 0.0f);
				return false;
			}

			for (int s = 0; s < n_speakers_; ++s)
			{
				gains[s] = gain_table_[idx * n_speakers_ + s];
			}
			return true;
		}

		bool get_gains_2d(float azimuth_deg, float* gains) const
		{
			int idx = compute_table_index(azimuth_deg, 0.0f);
			if (idx < 0 || idx >= n_table_entries_)
			{
				std::fill(gains, gains + n_speakers_, 0.0f);
				return false;
			}

			for (int s = 0; s < n_speakers_; ++s)
			{
				gains[s] = gain_table_[idx * n_speakers_ + s];
			}
			return true;
		}

		// SAF gain table data
		float* gain_table_ = nullptr;
		float* gain_table_compressed_ = nullptr;
		int* gain_table_indices_ = nullptr;
		int n_table_entries_ = 0;
		int n_triangles_ = 0;
		int n_speakers_ = 0;

		// Speaker configuration
		std::vector<float> speaker_dirs_; // Flat array: [az1, el1, az2, el2, ...]
		int azimuth_resolution_ = 1;
		int elevation_resolution_ = 1;
		float spread_ = 0.0f;
		bool use_3d_ = true;
		int omit_large_triangles_ = 0;
		int enable_dummies_ = 0;

		// State
		bool is_initialized_ = false;
		std::vector<float> current_gains_;
		std::vector<float> target_gains_;
		std::mutex mutex_;
	};
}
