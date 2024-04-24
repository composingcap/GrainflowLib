#pragma once 
#include  <cmath>
#pragma intrinsic(fabs)
#pragma intrinsic(floor)

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

		static inline double pong(double num, double min, double max, int fold) {
			double range = max - min;
			return (range * fold) + (1-fold*2) * std::fabs(mod(num - min, range*(fold+1)) - (range * fold)) + min;
		}


		static inline double round(double val, double step){ 
			step = fabs(step);
			return step * std::floor(val / (step+(step==0)*0.01f)+0.49f) * (step > 0) + val * (step <= 0);

		}

	};
}