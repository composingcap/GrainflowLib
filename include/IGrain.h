#pragma once
#include <memory>
#include <random>
#include <algorithm>
#include <numeric>
#include "gfParam.h"
#include "gfUtils.h"

/// <summary>
/// Contains entries and functions that modify said entities. This is the
/// fastest way to process data while also having the ability for it to be organized.
/// </summary>
namespace Grainflow
{
	/// <summary>
	/// An interface that represents a grainflow grain.
	/// To implement a grain, a valid interface needs to implement:
	/// -SampleParamBuffer
	/// -SampleEnvelope
	/// -SampleParamBuffer
	/// </summary>
	template <typename T1, typename T2, size_t BLOCKSIZE>
	class IGrain
	{
	private:
		bool reset = false;
		double lastGrainClock = -999;
		float sourcePositionNorm = 0;
		bool grainEnabled = true;
		bool bufferDefined = false;
		GfValueTable valueTable[2];
		double sampleIdTemp[BLOCKSIZE];
		float densityTemp[BLOCKSIZE];
		float ampTemp[BLOCKSIZE];
		double tempDouble[BLOCKSIZE];

	protected:
		std::random_device rd;
		int g = 0;
		double sampleRateAdjustment = 1;
		size_t bufferFrames = 441000;
		float oneOverBufferFrames = 1;
		int nchannels = 1;


	public:

		double sourceSample = 0;
		size_t stream = 0;
		float density = 1;

		GfParam delay;
		GfParam window;
		GfParam space;
		GfParam amplitude;
		GfParam rate;
		GfParam glisson;
		GfParam envelope;
		GfParam direction;
		GfParam nEnvelopes;
		GfParam rateQuantizeSemi;
		GfParam loopMode;

		GfParam startPoint;
		GfParam stopPoint;

		GfParam channel;

		/// Links to buffers - this can likely use a template argument and would be better
		T1* bufferRef = nullptr;
		T1* envelopeRef = nullptr;
		T1* delayBufRef = nullptr;
		T1* rateBufRef = nullptr;
		T1* windowBufRef = nullptr;

		IGrain()
		{
			rate.base = 1;
			amplitude.base = 1;
			direction.base = 1;
			stopPoint.base = 1;
			stopPoint.value = 1;
			rateQuantizeSemi.value = 1;
		}
		/// @brief The function implements reading an external buffer for select parameters when the feature is enabled.
		/// @param bufferType
		/// @param paramName
		inline virtual void SampleParamBuffer(GFBuffers bufferType, GfParamName paramName) = 0;

        inline virtual void SampleBuffer(T1* ref, double* __restrict samples, double* positions, const int size) = 0;

        inline virtual void SampleEnvelope(T1* ref, double* __restrict samples, double* grainClock, const int size) = 0;

		inline virtual void UpdateBufferInfo(T1* ref, gfIoConfig ioConfig) = 0;

		inline void Proccess(gfIoConfig ioConfig) {

			if (ioConfig.blockSize < BLOCKSIZE) return;

			UpdateBufferInfo(bufferRef, ioConfig);

			float windowPortion = 1 / std::clamp(1 - space.value, 0.0001f, 1.0f);
			// Check grain clock to make sure it is moving
			if (ioConfig.in[ioConfig.grainClock][0] == ioConfig.in[ioConfig.grainClock][1])
				return;
			float windowVal = window.value;

			for (int i = 0; i < ioConfig.blockSize / BLOCKSIZE; i++)
			{
				int block = i * BLOCKSIZE;
				auto amp = amplitude.value;
				double* grainClock = &ioConfig.in[ioConfig.grainClock][block];
				double* inputAmp = &ioConfig.in[ioConfig.am][block];
				double* fm = &ioConfig.in[ioConfig.fm][block];
				double* traversalPhasor = &ioConfig.in[ioConfig.traversalPhasor][block];

				double* grainProgress = &ioConfig.out[g + ioConfig.grainProgress][block];
				double* grainState = &ioConfig.out[g + ioConfig.grainState][block];
				double* grainPlayhead = &ioConfig.out[g + ioConfig.grainPlayhead][block];
				double* grainAmp = &ioConfig.out[g + ioConfig.grainAmp][block];
				double* grainEnvelope = &ioConfig.out[g + ioConfig.grainEnvelope][block];
				double* grainOutput = &ioConfig.out[g + ioConfig.grainOutput][block];
				double* grainChannels = &ioConfig.out[g + ioConfig.grainBufferChannel][block];
				double* grainStreams = &ioConfig.out[g + ioConfig.grainStreamChannel][block];

				ProccessGrainClock(grainClock, grainProgress, windowVal, windowPortion, BLOCKSIZE);
				auto valueFrames = GrainReset(grainProgress, traversalPhasor, grainState, BLOCKSIZE);
				Increment(fm, grainProgress, sampleIdTemp, tempDouble, BLOCKSIZE);
				SampleEnvelope(envelopeRef, grainEnvelope, grainProgress, BLOCKSIZE);
				SampleBuffer(bufferRef, grainOutput, sampleIdTemp, BLOCKSIZE);
				ExpandValueTable(valueFrames, grainState, ampTemp, densityTemp, BLOCKSIZE);
				OuputBlock(sampleIdTemp, ampTemp, densityTemp, oneOverBufferFrames, stream, inputAmp, grainPlayhead, grainAmp, grainEnvelope, grainOutput, grainChannels, grainStreams, BLOCKSIZE);
			}
		}

		void SetIndex(int g) { this->g = g; }

		GfParam *ParamGetHandle(GfParamName param)
		{
			switch (param)
			{
			case (GfParamName::delay):
				return &delay;
			case (GfParamName::rate):
				return &rate;
			case (GfParamName::window):
				return &window;
			case (GfParamName::amplitude):
				return &amplitude;
			case (GfParamName::glisson):
				return &glisson;
			case (GfParamName::space):
				return &space;
			case (GfParamName::envelopePosition):
				return &envelope;
			case (GfParamName::nEnvelopes):
				return &nEnvelopes;
			case (GfParamName::direction):
				return &direction;
			case (GfParamName::stopPoint):
				return &stopPoint;
			case (GfParamName::startPoint):
				return &startPoint;
			case (GfParamName::rateQuantizeSemi):
				return &rateQuantizeSemi;
			case (GfParamName::loopMode):
				return &loopMode;
			case (GfParamName::channel):
				return &channel;
			}	

			return nullptr;
		}

		float ParamGet(GfParamName param)
		{
			return ParamGetHandle(param)->value;
		}
		void ParamSet(float value, GfParamName param, GfParamType type)
		{
			GfParam *selectedParam = ParamGetHandle(param);

			switch (type)
			{
			case (GfParamType::base):
				selectedParam->base = value;
				break;
			case (GfParamType::random):
				selectedParam->random = value;
				break;
			case (GfParamType::offset):
				selectedParam->offset = value;
				break;
			case (GfParamType::mode):
				selectedParam->mode = (GfBufferMode)(int)value;
				break;
			case (GfParamType::value):
				selectedParam->value = value;
				break;
			default:
				throw("invalid type");
				return;
			}
		}

		void SampleParam(GfParamName paramName)
		{
			auto param = ParamGetHandle(paramName);
			std::random_device rd;
			param->value = abs((rd() % 10000) * 0.0001f) * (param->random) + param->base + param->offset * g;
		}

		void SampleParam(GfParam* param) {
			std::random_device rd;
			param->value = abs((rd() % 10000) * 0.0001f) * (param->random) + param->base + param->offset * g;
		}

		void SampleNormalized(GfParam* param, float range) {
			std::random_device rd;
			param->value = GfUtils::mod((abs((rd() % 10000) * 0.0001f) * (param->random) + +param->offset) * range + param->base, range);
		}

        inline GfValueTable* GrainReset(double* grainClock, const double* traversal, double* grainState, const int size)
		{
			for (int i = 0; i < 2; i++) {
				valueTable[i].delay = delay.value;
				valueTable[i].rate = rate.value;
				valueTable[i].glisson = glisson.value;
				valueTable[i].window = window.value;
				valueTable[i].amplitude = amplitude.value;
				valueTable[i].space = space.value;
				valueTable[i].envelopePosition = envelope.value;
				valueTable[i].direction = direction.value;
				valueTable[i].density = grainEnabled;
			}
			bool grainReset = lastGrainClock > grainClock[0];
			bool zeroCross = false;
			grainState[0] = !grainReset;
			int resetPosition = 0;
			for (int i = 1; i < size; i++) {
				zeroCross = grainClock[i - 1] > grainClock[i];
				grainState[i] = !zeroCross;
				resetPosition = resetPosition * !(grainReset && zeroCross) + i * (grainReset && zeroCross);
				grainReset = grainReset || zeroCross;
			}
			lastGrainClock = grainClock[size - 1];
			if (!grainReset) return valueTable;

				SampleParamBuffer(GFBuffers::delayBuffer, GfParamName::delay);
				sourceSample = ((traversal[resetPosition]) * bufferFrames - delay.value-1);
				sourceSample = GfUtils::mod(sourceSample, bufferFrames);
				SampleParamBuffer(GFBuffers::rateBuffer, GfParamName::rate);
				rate.value = 1 + GfUtils::round(rate.value - 1, 1-rateQuantizeSemi.value);
				SampleParamBuffer(GFBuffers::windowBuffer, GfParamName::window);
				SampleParam(&space);
				SampleParam(&glisson);
				SampleParam(&envelope);
				SampleParam(&amplitude);
				SampleParam(&startPoint);
				SampleParam(&stopPoint);
				SampleNormalized(&channel, nchannels);
				SampleDensity();
				SampleDirection();


				int i = 1;
				valueTable[i].delay = delay.value;
				valueTable[i].rate = rate.value;
				valueTable[i].glisson = glisson.value;
				valueTable[i].window = window.value;
				valueTable[i].amplitude = amplitude.value;
				valueTable[i].space = space.value;
				valueTable[i].envelopePosition = envelope.value;
				valueTable[i].direction = direction.value;
				valueTable[i].density = grainEnabled;

				return valueTable;
		}

		void SetBuffer(GFBuffers bufferType, T1 *buffer)
		{
			switch (bufferType)
			{
			case (GFBuffers::buffer):
				bufferRef = buffer;
				break;
			case (GFBuffers::envelope):
				envelopeRef = buffer;
				break;
			case (GFBuffers::rateBuffer):
				rateBufRef = buffer;
				break;
			case (GFBuffers::delayBuffer):
				delayBufRef = buffer;
				break;
			case (GFBuffers::windowBuffer):
				windowBufRef = buffer;
				break;
			};
		};

		T1 *GetBuffer(GFBuffers bufferType)
		{
			switch (bufferType)
			{
			case (GFBuffers::buffer):
				return bufferRef;
			case (GFBuffers::envelope):
				return envelopeRef;
			case (GFBuffers::rateBuffer):
				return rateBufRef;
			case (GFBuffers::delayBuffer):
				return delayBufRef;
			case (GFBuffers::windowBuffer):
				return windowBufRef;
			}
			return nullptr;
		}

		inline void SampleDensity()
		{
			std::random_device rd;
			grainEnabled = density >= (rd() % 10000) * 0.0001f;
		}

		inline void ExpandValueTable(const GfValueTable* valueFrames, const double* grainState, float* __restrict amplitudes, float* __restrict densities, const int size) {
			for (int j = 0; j < size; j++) {
				amplitudes[j] = valueFrames[(int)grainState[j]].amplitude;
				densities[j] = valueFrames[(int)grainState[j]].density;
			}
		}
		inline void ProccessGrainClock(const double* grainClock, double* __restrict grainProgress, const float windowVal, const float windowPortion, const int size) {
			for (int j = 0; j < size; j++) {
				double sample = grainClock[j] + windowVal;
				sample -= floor(sample);
				sample *= windowPortion;
				sample = std::min(sample, 1.0);
				grainProgress[j] = sample;
			}
		}
		inline void OuputBlock(double* __restrict sampleIds, float* __restrict amplitudes, float* __restrict densities, float oneOverBufferFrames, int stream, const double* inputAmp,
			double* __restrict grainPlayhead, double* __restrict grainAmp, double* __restrict grainEnvelope,
			double* __restrict grainOutput, double* __restrict grainStreamChannel, double* __restrict grainBufferChannel, const int size) {
			for (int j = 0; j < size; j++) {
				float density = densities[j];;
				float amplitude = amplitudes[j];
				grainPlayhead[j] = sampleIds[j] * oneOverBufferFrames * density;
				grainAmp[j] = (1 - inputAmp[j]) * amplitude * density;
				grainEnvelope[j] *= density;
				grainOutput[j] *= grainAmp[j] * 0.5 * grainEnvelope[j];
				grainStreamChannel[j] = stream + 1;
				grainBufferChannel[j] = (int)channel.value + 1;
			}
		}

        inline void Increment(const double* fm, const double* grainClock, double* __restrict samplePositions, double* __restrict sampleDeltaTemp, const int size)
		{
			float start = std::min((double)bufferFrames * startPoint.value, (double)bufferFrames-1);
			float end = std::min((double)bufferFrames * stopPoint.value, (double)bufferFrames - 1);
			for (int i = 0; i < size; i++) {
				sampleDeltaTemp[i] = fm[i] * sampleRateAdjustment * rate.value * (1 + glisson.value * grainClock[i]) * direction.value;
			}
			samplePositions[0] = sourceSample;
			double lastPosition = sourceSample;
			for (int i = 1; i < size; i++) {
				samplePositions[i] = lastPosition + sampleDeltaTemp[i-1];
				lastPosition = samplePositions[i];
			}
			int fold = loopMode.base > 1.1f ? 1:0;
			sourceSample = samplePositions[size - 1] + sampleDeltaTemp[size - 1];
			for (int i = 0; i < size; i++) {
				samplePositions[i] = GfUtils::pong(samplePositions[i], start, end, fold);
			}
		}


		void StreamSet(int maxGrains, GfStreamSetType mode, int nstreams)
		{
			switch (mode)
			{
			case (GfStreamSetType::automaticStreams):
				stream = g % nstreams;
				break;
			case (GfStreamSetType::perStreams):
				stream = g / nstreams;
				break;
			case (GfStreamSetType::randomStreams):
				std::random_device rd;
				stream = rd() % nstreams;
				break;
			}
		};
		void SampleDirection()
		{
			if (direction.base >= 1)
				direction.value = 1;
			else if (direction.base <= -1)
				direction.value = -1;
			else
			{
				float randomDirection = (rand() % 1000) * 0.001f;
				if (randomDirection > direction.base)
				{
					direction.value = -1;
				}
				else
				{
					direction.value = 1;
				}
			}
		}
	};



	
}
