#pragma once 
#include  <cmath>
namespace Grainflow {
	class GfUtils
	{
	public:
		static float Deviate(float center, float range)
		{
			std::random_device rd;
			return center + ((rd() % 10000) * 0.0001f - 1) * 2 * range;
		}

		static float Lerp(float lower, float upper, float position)
		{
			return lower * (1 - position) + upper * position;
		}

		static float PitchToRate(float pitch)
		{
			return pow(2, pitch / 12);
		}

		static double mod(double a, double b) { return a - b * floor(a / b); }



	};
}