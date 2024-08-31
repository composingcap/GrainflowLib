#pragma once 
namespace Grainflow {
	class GfBufferInfo {
	public:
		int bufferFrames = 0;
		float oneOverBufferFrames = 0;
		float sampleRateAdjustment = 1;
		int nchannels = 0;
	};
	template<typename T>
	struct GfIBufferReader {
	public:
		bool (*SampleParamBuffer)(T* buffer, GfParam* param, int grainId) = nullptr;
		void (*SampleBuffer)(T* buffer, int channel, double* __restrict samples, double* positions, const int size) = nullptr;
		void (*UpdateBufferInfo)(T* buffer, gfIoConfig ioConfig, GfBufferInfo* bufferInfo) = nullptr;
		void (*SampleEnvelope)(T* buffer, const bool useDefault, const int nEnvelopes, const float env2dPos, double* __restrict samples, double* grainClock, const int size) = nullptr;
	};




}