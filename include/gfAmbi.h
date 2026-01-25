#pragma once
#include <vector>
#include <array>
#include <cmath>
#include <mutex>
#include <algorithm>

// Include the main SAF header which handles C/C++ linkage properly
#include "saf.h"

namespace Grainflow
{
	/**
	 * @brief Ambisonics encoder wrapper using SAF (Spatial Audio Framework)
	 *
	 * Encodes mono sources to ambisonics format (ACN/N3D by default).
	 * Supports orders 1-7.
	 *
	 * @tparam InternalBlock DSP block size for optimization
	 * @tparam SigType Signal type (float or double)
	 */
	template <size_t InternalBlock, typename SigType = double>
	class gf_ambi_encode
	{
	public:
		gf_ambi_encode() = default;
		~gf_ambi_encode() = default;

		/**
		 * @brief Set the ambisonics order (1-7)
		 */
		void set_order(int order)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			order_ = std::clamp(order, 1, 7);
			n_channels_ = (order_ + 1) * (order_ + 1);
			sh_weights_.resize(n_channels_);
			current_weights_.resize(n_channels_, 0.0f);
		}

		int get_order() const { return order_; }
		int get_num_channels() const { return n_channels_; }

		/**
		 * @brief Set channel ordering convention
		 */
		void set_channel_order(HOA_CH_ORDER order) { channel_order_ = order; }
		HOA_CH_ORDER get_channel_order() const { return channel_order_; }

		/**
		 * @brief Set normalisation convention
		 */
		void set_normalisation(HOA_NORM norm) { normalisation_ = norm; }
		HOA_NORM get_normalisation() const { return normalisation_; }

		/**
		 * @brief Set source direction for encoding
		 * @param source_index Source index (0-based)
		 * @param azimuth_deg Azimuth in degrees (-180 to 180, 0 = front)
		 * @param elevation_deg Elevation in degrees (-90 to 90)
		 */
		void set_source_direction(int source_index, float azimuth_deg, float elevation_deg)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (source_index < 0) return;

			// Ensure storage for this source
			if (static_cast<int>(source_azimuths_.size()) <= source_index)
			{
				source_azimuths_.resize(source_index + 1, 0.0f);
				source_elevations_.resize(source_index + 1, 0.0f);
			}

			source_azimuths_[source_index] = azimuth_deg;
			source_elevations_[source_index] = elevation_deg;

			// Also update legacy single-source values for source 0
			if (source_index == 0)
			{
				azimuth_ = azimuth_deg;
				elevation_ = elevation_deg;
				update_sh_weights();
			}
		}

		/**
		 * @brief Get source direction
		 * @param source_index Source index (0-based)
		 */
		void get_source_direction(int source_index, float& azimuth_deg, float& elevation_deg) const
		{
			if (source_index >= 0 && source_index < static_cast<int>(source_azimuths_.size()))
			{
				azimuth_deg = source_azimuths_[source_index];
				elevation_deg = source_elevations_[source_index];
			}
			else
			{
				azimuth_deg = azimuth_;
				elevation_deg = elevation_;
			}
		}

		/**
		 * @brief Get spherical harmonic weights for current direction
		 * @param weights Output array (must have n_channels_ elements)
		 */
		void get_sh_weights(float* weights) const
		{
			std::lock_guard<std::mutex> lock(mutex_);
			std::copy(sh_weights_.begin(), sh_weights_.end(), weights);
		}

		/**
		 * @brief Encode a mono source to ambisonics
		 * @param input Input buffer (mono)
		 * @param output Output buffers (one per ambisonics channel)
		 * @param block_size Number of samples
		 * @param azimuth_deg Source azimuth (optional, uses stored value if not provided)
		 * @param elevation_deg Source elevation (optional)
		 */
		void encode(const SigType* input, SigType** output, int block_size,
		            float azimuth_deg, float elevation_deg)
		{
			std::lock_guard<std::mutex> lock(mutex_);

			// Update direction if changed
			if (azimuth_deg != azimuth_ || elevation_deg != elevation_)
			{
				azimuth_ = azimuth_deg;
				elevation_ = elevation_deg;
				update_sh_weights();
			}

			// Interpolation increment
			const float increment = 1.0f / static_cast<float>(block_size);

			// Encode with gain interpolation
			for (int ch = 0; ch < n_channels_; ++ch)
			{
				const float start_weight = current_weights_[ch];
				const float end_weight = sh_weights_[ch];
				const float weight_delta = (end_weight - start_weight) * increment;

				float weight = start_weight;
				for (int j = 0; j < block_size / static_cast<int>(InternalBlock); ++j)
				{
					SigType* out_ptr = &output[ch][j * InternalBlock];
					const SigType* in_ptr = &input[j * InternalBlock];

					for (size_t k = 0; k < InternalBlock; ++k)
					{
						out_ptr[k] = in_ptr[k] * static_cast<SigType>(weight);
						weight += weight_delta;
					}
				}
				current_weights_[ch] = end_weight;
			}
		}

		/**
		 * @brief Encode multiple sources to ambisonics (accumulative)
		 * @param inputs Input buffers (one per source)
		 * @param outputs Output buffers (one per ambisonics channel) - cleared first
		 * @param n_sources Number of sources
		 * @param block_size Samples per buffer
		 * @param azimuths Array of source azimuths
		 * @param elevations Array of source elevations
		 */
		void encode_multi(SigType** inputs, SigType** outputs, int n_sources,
		                  int block_size, const float* azimuths, const float* elevations)
		{
			// Clear outputs first
			for (int ch = 0; ch < n_channels_; ++ch)
			{
				std::fill(outputs[ch], outputs[ch] + block_size, static_cast<SigType>(0));
			}

			// Ensure we have weight storage for all sources
			if (static_cast<int>(multi_source_weights_.size()) < n_sources)
			{
				multi_source_weights_.resize(n_sources);
				for (int src = 0; src < n_sources; ++src)
				{
					if (static_cast<int>(multi_source_weights_[src].size()) != n_channels_)
					{
						multi_source_weights_[src].resize(n_channels_, 0.0f);
					}
				}
			}

			std::vector<float> target_weights(n_channels_);
			float dir[2];
			const float increment = 1.0f / static_cast<float>(block_size);

			for (int src = 0; src < n_sources; ++src)
			{
				// Ensure this source has weight storage
				if (static_cast<int>(multi_source_weights_[src].size()) != n_channels_)
				{
					multi_source_weights_[src].resize(n_channels_, 0.0f);
				}

				// Compute target SH weights for this source
				dir[0] = azimuths[src];
				dir[1] = elevations[src];
				getRSH_recur(order_, dir, 1, target_weights.data());

				// Apply normalisation conversion if needed
				if (normalisation_ != HOA_NORM_N3D)
				{
					convertHOANormConvention(target_weights.data(), order_, 1,
					                         HOA_NORM_N3D, normalisation_);
				}

				// Apply channel order conversion if needed
				if (channel_order_ != HOA_CH_ORDER_ACN)
				{
					convertHOAChannelConvention(target_weights.data(), order_, 1,
					                            HOA_CH_ORDER_ACN, channel_order_);
				}

				// Encode with interpolation
				for (int ch = 0; ch < n_channels_; ++ch)
				{
					const float start_weight = multi_source_weights_[src][ch];
					const float end_weight = target_weights[ch];
					const float weight_delta = (end_weight - start_weight) * increment;

					float weight = start_weight;
					for (int j = 0; j < block_size / static_cast<int>(InternalBlock); ++j)
					{
						SigType* out_ptr = &outputs[ch][j * InternalBlock];
						const SigType* in_ptr = &inputs[src][j * InternalBlock];

						for (size_t k = 0; k < InternalBlock; ++k)
						{
							out_ptr[k] += in_ptr[k] * static_cast<SigType>(weight);
							weight += weight_delta;
						}
					}
					multi_source_weights_[src][ch] = end_weight;
				}
			}
		}

	private:
		void update_sh_weights()
		{
			if (sh_weights_.size() != static_cast<size_t>(n_channels_))
			{
				sh_weights_.resize(n_channels_);
				current_weights_.resize(n_channels_, 0.0f);
			}

			float dir[2] = {azimuth_, elevation_};
			getRSH_recur(order_, dir, 1, sh_weights_.data());

			// Apply normalisation conversion if needed
			if (normalisation_ != HOA_NORM_N3D)
			{
				convertHOANormConvention(sh_weights_.data(), order_, 1,
				                         HOA_NORM_N3D, normalisation_);
			}

			// Apply channel order conversion if needed
			if (channel_order_ != HOA_CH_ORDER_ACN)
			{
				convertHOAChannelConvention(sh_weights_.data(), order_, 1,
				                            HOA_CH_ORDER_ACN, channel_order_);
			}
		}

		int order_ = 1;
		int n_channels_ = 4; // (order+1)^2 for order 1
		float azimuth_ = 0.0f;
		float elevation_ = 0.0f;
		HOA_CH_ORDER channel_order_ = HOA_CH_ORDER_ACN;
		HOA_NORM normalisation_ = HOA_NORM_SN3D; // AmbiX default

		std::vector<float> sh_weights_;
		std::vector<float> current_weights_; // For interpolation (single source)
		std::vector<std::vector<float>> multi_source_weights_; // For interpolation (multi source)
		std::vector<float> source_azimuths_; // Per-source azimuths
		std::vector<float> source_elevations_; // Per-source elevations
		mutable std::mutex mutex_;
	};

	/**
	 * @brief Ambisonics decoder wrapper using SAF (Spatial Audio Framework)
	 *
	 * Decodes ambisonics to loudspeaker feeds.
	 * Supports various decoder methods (SAD, MMD, EPAD, AllRAD).
	 *
	 * @tparam InternalBlock DSP block size for optimization
	 * @tparam SigType Signal type (float or double)
	 */
	template <size_t InternalBlock, typename SigType = double>
	class gf_ambi_decode
	{
	public:
		gf_ambi_decode() = default;

		~gf_ambi_decode()
		{
			free_decoder_matrix();
			free_binaural_decoder();
		}

		/**
		 * @brief Set the ambisonics order (1-7)
		 */
		void set_order(int order)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			int new_order = std::clamp(order, 1, 7);
			if (new_order != order_)
			{
				order_ = new_order;
				n_ambi_channels_ = (order_ + 1) * (order_ + 1);
				decoder_dirty_ = true;
			}
		}

		int get_order() const { return order_; }
		int get_num_ambi_channels() const { return n_ambi_channels_; }
		int get_num_speakers() const { return binaural_mode_ ? 2 : n_speakers_; }

		/**
		 * @brief Enable/disable binaural decoding mode
		 * When enabled, outputs 2 channels (binaural) instead of speaker feeds
		 */
		void set_binaural(bool enable)
		{
			if (enable != binaural_mode_)
			{
				binaural_mode_ = enable;
				decoder_dirty_ = true;
			}
		}
		bool get_binaural() const { return binaural_mode_; }

		/**
		 * @brief Set channel ordering convention of input ambisonics
		 */
		void set_channel_order(HOA_CH_ORDER order)
		{
			if (order != channel_order_)
			{
				channel_order_ = order;
				decoder_dirty_ = true;
			}
		}
		HOA_CH_ORDER get_channel_order() const { return channel_order_; }

		/**
		 * @brief Set normalisation convention of input ambisonics
		 */
		void set_normalisation(HOA_NORM norm)
		{
			if (norm != normalisation_)
			{
				normalisation_ = norm;
				decoder_dirty_ = true;
			}
		}
		HOA_NORM get_normalisation() const { return normalisation_; }

		/**
		 * @brief Set decoder method
		 */
		void set_decoder_method(LOUDSPEAKER_AMBI_DECODER_METHODS method)
		{
			if (method != decoder_method_)
			{
				decoder_method_ = method;
				decoder_dirty_ = true;
			}
		}
		LOUDSPEAKER_AMBI_DECODER_METHODS get_decoder_method() const { return decoder_method_; }

		/**
		 * @brief Enable/disable max rE weighting
		 */
		void set_max_re(bool enable)
		{
			if (enable != enable_max_re_)
			{
				enable_max_re_ = enable;
				decoder_dirty_ = true;
			}
		}
		bool get_max_re() const { return enable_max_re_; }

		/**
		 * @brief Set speaker layout
		 * @param dirs_deg Speaker directions [azi1, elev1, azi2, elev2, ...] in degrees
		 * @param n_speakers Number of speakers
		 */
		void set_speaker_layout(const float* dirs_deg, int n_speakers)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			n_speakers_ = n_speakers;
			speaker_dirs_.assign(dirs_deg, dirs_deg + n_speakers * 2);
			decoder_dirty_ = true;
		}

		/**
		 * @brief Set speaker layout from azimuth/elevation vectors
		 */
		void set_speaker_layout(const std::vector<float>& azimuths,
		                        const std::vector<float>& elevations)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			n_speakers_ = static_cast<int>(std::min(azimuths.size(), elevations.size()));
			speaker_dirs_.resize(n_speakers_ * 2);
			for (int i = 0; i < n_speakers_; ++i)
			{
				speaker_dirs_[i * 2] = azimuths[i];
				speaker_dirs_[i * 2 + 1] = elevations[i];
			}
			decoder_dirty_ = true;
		}

		/**
		 * @brief Decode ambisonics to speaker feeds or binaural
		 * @param inputs Input buffers (ambisonics channels)
		 * @param outputs Output buffers (speaker feeds or 2-ch binaural) - cleared first
		 * @param block_size Number of samples
		 */
		void decode(SigType** inputs, SigType** outputs, int block_size)
		{
			std::lock_guard<std::mutex> lock(mutex_);

			if (decoder_dirty_)
			{
				rebuild_decoder();
			}

			if (binaural_mode_)
			{
				decode_binaural(inputs, outputs, block_size);
				return;
			}

			if (decoder_matrix_.empty() || n_speakers_ == 0)
			{
				return;
			}

			// Clear outputs
			for (int s = 0; s < n_speakers_; ++s)
			{
				std::fill(outputs[s], outputs[s] + block_size, static_cast<SigType>(0));
			}

			const int num_blocks = block_size / static_cast<int>(InternalBlock);
			const int remainder = block_size % static_cast<int>(InternalBlock);

			// Apply decoder matrix: output[speaker] = sum(decoder[speaker][ch] * input[ch])
			for (int s = 0; s < n_speakers_; ++s)
			{
				for (int ch = 0; ch < n_ambi_channels_; ++ch)
				{
					const float gain = decoder_matrix_[s * n_ambi_channels_ + ch];
					if (std::abs(gain) < 1e-6f) continue;

					const SigType typed_gain = static_cast<SigType>(gain);

					// Process full blocks
					for (int j = 0; j < num_blocks; ++j)
					{
						SigType* out_ptr = &outputs[s][j * InternalBlock];
						const SigType* in_ptr = &inputs[ch][j * InternalBlock];

						for (size_t k = 0; k < InternalBlock; ++k)
						{
							out_ptr[k] += in_ptr[k] * typed_gain;
						}
					}

					// Process remaining samples
					if (remainder > 0)
					{
						SigType* out_ptr = &outputs[s][num_blocks * InternalBlock];
						const SigType* in_ptr = &inputs[ch][num_blocks * InternalBlock];

						for (int k = 0; k < remainder; ++k)
						{
							out_ptr[k] += in_ptr[k] * typed_gain;
						}
					}
				}
			}
		}

		/**
		 * @brief Check if decoder needs rebuilding
		 */
		bool is_dirty() const { return decoder_dirty_; }

		/**
		 * @brief Force decoder rebuild
		 */
		void rebuild()
		{
			std::lock_guard<std::mutex> lock(mutex_);
			rebuild_decoder();
		}

	private:
		void free_decoder_matrix()
		{
			decoder_matrix_.clear();
		}

		void rebuild_decoder()
		{
			free_decoder_matrix();

			if (binaural_mode_)
			{
				rebuild_binaural_decoder();
				decoder_dirty_ = false;
				return;
			}

			if (n_speakers_ == 0 || speaker_dirs_.empty())
			{
				decoder_dirty_ = false;
				return;
			}

			// Allocate decoder matrix: n_speakers x n_ambi_channels
			decoder_matrix_.resize(n_speakers_ * n_ambi_channels_);

			// Get decoder matrix from SAF (expects N3D/ACN input)
			getLoudspeakerDecoderMtx(
				speaker_dirs_.data(),
				n_speakers_,
				decoder_method_,
				order_,
				enable_max_re_ ? 1 : 0,
				decoder_matrix_.data()
			);

			// Check if SAF returned a valid matrix (not all zeros)
			bool matrix_valid = false;
			for (size_t i = 0; i < decoder_matrix_.size() && !matrix_valid; ++i)
			{
				if (std::abs(decoder_matrix_[i]) > 1e-10f)
				{
					matrix_valid = true;
				}
			}

			// If SAF failed, create a simple projection decoder as fallback
			if (!matrix_valid)
			{
				// Simple SAD (Sampling Ambisonic Decoder) - just sample SH at speaker directions
				for (int s = 0; s < n_speakers_; ++s)
				{
					float azi = speaker_dirs_[s * 2];
					float elev = speaker_dirs_[s * 2 + 1];
					float dir[2] = {azi, elev};

					// Get SH weights for this speaker direction
					std::vector<float> sh_weights(n_ambi_channels_);
					getRSH_recur(order_, dir, 1, sh_weights.data());

					// Copy to decoder matrix row
					for (int ch = 0; ch < n_ambi_channels_; ++ch)
					{
						decoder_matrix_[s * n_ambi_channels_ + ch] = sh_weights[ch];
					}
				}
			}

			// SAF decoder matrix expects N3D/ACN input, so we need to pre-bake
			// the conversion from the user's selected format into the matrix.
			// This is done by scaling each column (ambi channel) of the matrix.

			// Apply normalization conversion to matrix columns
			if (normalisation_ != HOA_NORM_N3D)
			{
				// Get conversion factors by converting a unit signal
				std::vector<float> norm_factors(n_ambi_channels_, 1.0f);
				convertHOANormConvention(norm_factors.data(), order_, 1,
				                         normalisation_, HOA_NORM_N3D);

				// Scale each column of the decoder matrix
				for (int s = 0; s < n_speakers_; ++s)
				{
					for (int ch = 0; ch < n_ambi_channels_; ++ch)
					{
						decoder_matrix_[s * n_ambi_channels_ + ch] *= norm_factors[ch];
					}
				}
			}

			// Apply channel order conversion if needed
			if (channel_order_ != HOA_CH_ORDER_ACN)
			{
				// Reorder columns of the decoder matrix
				std::vector<float> reordered_row(n_ambi_channels_);
				for (int s = 0; s < n_speakers_; ++s)
				{
					// Copy row to temp buffer
					for (int ch = 0; ch < n_ambi_channels_; ++ch)
					{
						reordered_row[ch] = decoder_matrix_[s * n_ambi_channels_ + ch];
					}
					// Apply channel order conversion
					convertHOAChannelConvention(reordered_row.data(), order_, 1,
					                            HOA_CH_ORDER_ACN, channel_order_);
					// Copy back
					for (int ch = 0; ch < n_ambi_channels_; ++ch)
					{
						decoder_matrix_[s * n_ambi_channels_ + ch] = reordered_row[ch];
					}
				}
			}

			decoder_dirty_ = false;
		}

		void decode_binaural(SigType** inputs, SigType** outputs, int block_size)
		{
			if (binaural_matrix_.empty())
			{
				return;
			}

			// Clear outputs (2 channels: left and right)
			std::fill(outputs[0], outputs[0] + block_size, static_cast<SigType>(0));
			std::fill(outputs[1], outputs[1] + block_size, static_cast<SigType>(0));

			const int num_blocks = block_size / static_cast<int>(InternalBlock);
			const int remainder = block_size % static_cast<int>(InternalBlock);

			// Apply binaural decoder matrix: output[ear] = sum(binaural_matrix[ear][ch] * input[ch])
			for (int ear = 0; ear < 2; ++ear)
			{
				for (int ch = 0; ch < n_ambi_channels_; ++ch)
				{
					const float gain = binaural_matrix_[ear * n_ambi_channels_ + ch];
					if (std::abs(gain) < 1e-6f) continue;

					const SigType typed_gain = static_cast<SigType>(gain);

					for (int j = 0; j < num_blocks; ++j)
					{
						SigType* out_ptr = &outputs[ear][j * InternalBlock];
						const SigType* in_ptr = &inputs[ch][j * InternalBlock];

						for (size_t k = 0; k < InternalBlock; ++k)
						{
							out_ptr[k] += in_ptr[k] * typed_gain;
						}
					}

					if (remainder > 0)
					{
						SigType* out_ptr = &outputs[ear][num_blocks * InternalBlock];
						const SigType* in_ptr = &inputs[ch][num_blocks * InternalBlock];

						for (int k = 0; k < remainder; ++k)
						{
							out_ptr[k] += in_ptr[k] * typed_gain;
						}
					}
				}
			}
		}

		void free_binaural_decoder()
		{
			binaural_matrix_.clear();
		}

		void rebuild_binaural_decoder()
		{
			free_binaural_decoder();

			// Create virtual speaker layout for binaural decoding
			// Using a simple arrangement: speakers at ear positions plus virtual sources
			// This creates a basic amplitude-panned binaural output

			// Binaural matrix: 2 (ears) x n_ambi_channels
			binaural_matrix_.resize(2 * n_ambi_channels_);

			// Simple first-order binaural decode using virtual cardioid patterns
			// Left ear: W + 0.5*Y (cardioid pointing left)
			// Right ear: W - 0.5*Y (cardioid pointing right)
			// For higher orders, we add more directional components

			std::fill(binaural_matrix_.begin(), binaural_matrix_.end(), 0.0f);

			// W channel (omnidirectional) goes to both ears equally
			binaural_matrix_[0] = 0.7071f;  // Left ear, W
			binaural_matrix_[n_ambi_channels_] = 0.7071f;  // Right ear, W

			if (n_ambi_channels_ >= 4)
			{
				// Y channel (left-right) - ACN order: W=0, Y=1, Z=2, X=3
				// Positive Y is left in ACN convention
				binaural_matrix_[1] = 0.5f;   // Left ear, Y (left side)
				binaural_matrix_[n_ambi_channels_ + 1] = -0.5f;  // Right ear, Y (right side)

				// X channel (front-back) - slight emphasis for front
				binaural_matrix_[3] = 0.25f;  // Left ear, X
				binaural_matrix_[n_ambi_channels_ + 3] = 0.25f;  // Right ear, X
			}

			// Apply normalization conversion if needed
			if (normalisation_ != HOA_NORM_N3D)
			{
				std::vector<float> norm_factors(n_ambi_channels_, 1.0f);
				convertHOANormConvention(norm_factors.data(), order_, 1,
				                         normalisation_, HOA_NORM_N3D);
				for (int ear = 0; ear < 2; ++ear)
				{
					for (int ch = 0; ch < n_ambi_channels_; ++ch)
					{
						binaural_matrix_[ear * n_ambi_channels_ + ch] *= norm_factors[ch];
					}
				}
			}

			// Apply channel order conversion if needed
			if (channel_order_ != HOA_CH_ORDER_ACN)
			{
				for (int ear = 0; ear < 2; ++ear)
				{
					std::vector<float> reordered(n_ambi_channels_);
					for (int ch = 0; ch < n_ambi_channels_; ++ch)
					{
						reordered[ch] = binaural_matrix_[ear * n_ambi_channels_ + ch];
					}
					convertHOAChannelConvention(reordered.data(), order_, 1,
					                            HOA_CH_ORDER_ACN, channel_order_);
					for (int ch = 0; ch < n_ambi_channels_; ++ch)
					{
						binaural_matrix_[ear * n_ambi_channels_ + ch] = reordered[ch];
					}
				}
			}
		}

		int order_ = 1;
		int n_ambi_channels_ = 4;
		int n_speakers_ = 0;

		HOA_CH_ORDER channel_order_ = HOA_CH_ORDER_ACN;
		HOA_NORM normalisation_ = HOA_NORM_SN3D;
		LOUDSPEAKER_AMBI_DECODER_METHODS decoder_method_ = LOUDSPEAKER_DECODER_ALLRAD;
		bool enable_max_re_ = true;
		bool binaural_mode_ = false;

		std::vector<float> speaker_dirs_; // [azi1, el1, azi2, el2, ...]
		std::vector<float> decoder_matrix_; // n_speakers x n_ambi_channels
		std::vector<float> binaural_matrix_; // 2 x n_ambi_channels

		bool decoder_dirty_ = true;
		mutable std::mutex mutex_;
	};
}
