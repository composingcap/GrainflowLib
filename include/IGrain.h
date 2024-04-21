#pragma once
#include <memory>
#include <random>
#include <algorithm>
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
	template <typename T1, typename T2>
	class IGrain
	{
	private:
		bool reset = false;
		double lastGrainClock = -999;
		double sampleRateAdjustment = 1;
		float sourcePositionNorm = 0;
		bool grainEnabled = true;
		bool bufferDefined = false;
		GfValueTable valueTable[2];

	protected:
		std::random_device rd;
		int index = 0;

	public:
		size_t bufferFrames = 441000;
		float oneOverBufferFrames = 1;
		double sourceSample = 0;
		size_t stream = 0;
		size_t bchan = 0;
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

		GfParam startPoint;
		GfParam stopPoint;

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
		}
		/// @brief The function implements reading an external buffer for select parameters when the feature is enabled.
		/// @param bufferType
		/// @param paramName
		inline virtual void SampleParamBuffer(GFBuffers bufferType, GfParamName paramName) = 0;

        inline virtual void SampleBuffer(T2 &sampleLock, double* samples, double* positions, const int size) = 0;

        inline virtual void SampleEnvelope(T2 &sampleLock, double* samples, double* grainClock, const int size) = 0;

		float GetLastClock() { return lastGrainClock; }

		void SetIndex(int index) { this->index = index; }

		void SetBufferFrames(int frames)
		{
			bufferFrames = frames;
			oneOverBufferFrames = 1.0f / bufferFrames;
		}

		void SetSampleRateAdjustment(float ratio)
		{
			sampleRateAdjustment = ratio;
		}

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
			param->value = abs((rd() % 10000) * 0.0001f) * (param->random) + param->base + param->offset * index;
		}

		void SampleParam(GfParam* param) {
			std::random_device rd;
			param->value = abs((rd() % 10000) * 0.0001f) * (param->random) + param->base + param->offset * index;
		}

        inline GfValueTable* GrainReset(double* grainClock, double* traversal, double* grainState, const int size)
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
				valueTable[i].density = density;
			}
			bool grainReset = GetLastClock() > grainClock[0];
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
				SampleParamBuffer(GFBuffers::windowBuffer, GfParamName::window);
				SampleParam(&space);
				SampleParam(&glisson);
				SampleParam(&envelope);
				SampleParam(&amplitude);
				SampleParam(&startPoint);
				SampleParam(&stopPoint);

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
				valueTable[i].density = density;

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

		void SetSampleRateAdjustment(float gloabalSampleRate, float bufferSampleRate)
		{
			sampleRateAdjustment = bufferSampleRate / gloabalSampleRate;
		}

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

		void SampleDensity()
		{
			std::random_device rd;
			grainEnabled = density >= (rd() % 10000) * 0.0001f;
		}

        inline void Increment(double* fm, double* grainClock, double* samplePositions, const int size)
		{
			float start = std::min((double)bufferFrames * startPoint.value, (double)bufferFrames-1);
			float end = std::min((double)bufferFrames * stopPoint.value, (double)bufferFrames - 1);
			for (int i = 0; i < size; i++) {
				sourceSample += fm[i] * sampleRateAdjustment * rate.value * (1 + glisson.value * grainClock[i]) * direction.value;
				sourceSample = GfUtils::mod(sourceSample, start, end);
				samplePositions[i] = sourceSample;
			}
		}


		void StreamSet(int maxGrains, GfStreamSetType mode, int nstreams)
		{
			switch (mode)
			{
			case (GfStreamSetType::automaticStreams):
				stream = index % nstreams;
				break;
			case (GfStreamSetType::perStreams):
				stream = index / nstreams;
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
