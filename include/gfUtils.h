#pragma once
#include  <cmath>
#pragma intrinsic(fabs)
#pragma intrinsic(floor)
#include "gfEnvelopes.h"

namespace Grainflow
{
	class gf_utils
	{
	private:

	public:
		static inline float deviate(const float center, const float range, [[maybe_unused]] float empty = 0)
		{
			std::random_device rd;
			return center + (static_cast<float>(rd() % 10000) * 0.0001f - 0.5f) * 2 * range;
		}

		static inline float random_range(const float bottom, const float top, [[maybe_unused]] float empty = 0)
		{
			std::random_device rd;
			return lerp(bottom, top, rd() % 10000 * 0.0001f);
		}

		static inline float lerp(const float lower, const float upper, const float position)
		{
			return lower * (1 - position) + upper * position;
		}

		static inline float pitch_to_rate(const float pitch)
		{
			return std::exp2f(pitch / 12);
		}

		static inline float rate_to_pitch(const float rate)
		{
			return static_cast<float>(round(12 * log2(rate), 1e-4));
		}

		static inline float rate_offset_to_pitch_offset(const float rate_offset)
		{
			return rate_to_pitch(rate_offset + 1);
		}

		static inline float pitch_offset_to_rate_offset(const float pitch_offset)
		{
			return pitch_to_rate(pitch_offset) - 1;
		}

		static inline double mod(const double a, const double b) { return a - b * floor(a / b); }

		static inline double mod(const double num, const double min, const double max)
		{
			return (mod(num - min, max - min) + min);
		}

		static inline double pong(const double num, const double min, const double max, const int fold)
		{
			const double range = max - min;
			return (range * fold) + (1 - fold * 2) * std::fabs(mod(num - min, range * (fold + 1)) - (range * fold)) +
				min;
		}


		static inline double round(const double val, double step)
		{
			step = fabs(step);
			return step * std::floor(val / (step + static_cast<double>(abs(step) < 0.0001) * 0.01) + 0.49) * (step > 0)
				+ val * (step <= 0);
		}


		static inline double trunc(const double val, double step)
		{
			step = fabs(step);
			return step * std::floor(val / (step + static_cast<double>(abs(step) < 0.0001) * 0.01f)) * (step > 0) + val
				* (step <= 0);
		}

		static inline double sin_lookup(float n){
			return gf_envelopes::quarter_sine_wave[static_cast<
						int>
					((n) * 4095)];
		}
	};

	enum class GF_RETURN_CODE
	{
		GF_SUCCESS = 0,
		GF_ERR = 1,
		GF_PARAM_NOT_FOUND = 2,
	};
}
