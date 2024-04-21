#pragma once 
#include  <cmath>
#pragma intrinsic(fabs)

namespace Grainflow {
	class GfUtils
	{
	public:
		static inline float Deviate(float center, float range)
		{
			std::random_device rd;
			return center + ((rd() % 10000) * 0.0001f - 1) * 2 * range;
		}

		static inline float Lerp(float lower, float upper, float position)
		{
			return lower * (1 - position) + upper * position;
		}

		static inline float PitchToRate(float pitch)
		{
			return pow(2, pitch / 12);
		}

		static inline double mod(double a, double b) { return a - b * floor(a / b); }

		static inline double mod(double num, double min, double max) {return (mod(num - min, max - min) + min);}

	};
}