#include "gfUtils.h"
#include <vector>

namespace Grainflow {
	class GfSyn {
	public:
		template<long INTERNALBLOCK>
		static inline bool Phasor(const double* freqs, double** __restrict outputs, const long blockSize, const long channels, const double oneOverSamplerate, double* history) {
			if (blockSize < INTERNALBLOCK) return false;
			long blocks = blockSize / INTERNALBLOCK;
			//This assumes the same frequency over the entire block
			for (long ch = 0; ch < channels; ch++) {
				for (long i = 0; i < blocks; i++) {
					double rate = freqs[ch] * oneOverSamplerate;
					double base = history[ch];
					if (base + rate * (INTERNALBLOCK - 1) < 1.0) { //If the last value will be under 1 do not mod1
						for (long j = 0; j < INTERNALBLOCK; j++) {
							outputs[ch][i * INTERNALBLOCK + j] = base + rate * j;
						}
					}
					else {
						for (int j = 0; j < INTERNALBLOCK; j++) { //mod1 if it will be over 1
							outputs[ch][i * INTERNALBLOCK + j] = GfUtils::mod(base + rate * j);
						}
					}
					history[ch] = outputs[ch][i * INTERNALBLOCK + INTERNALBLOCK - 1];
				}
			}
			return true;
		};
	};
}