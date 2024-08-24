#pragma once
#include <map>
#include <string>
#include <algorithm>
namespace Grainflow {
	/// <summary>
	/// Available parameters using the GfParam struct
	/// </summary>
	enum class GfParamName
	{
		delay = 0,
		rate,
		glisson,
		window,
		amplitude,
		space,
		envelopePosition,
		nEnvelopes,
		direction,
		startPoint,
		stopPoint,
		rateQuantizeSemi,
		loopMode,
		channel
	};
	/// <summary>
	/// Different parameter types using in the GfParam Struct
	/// </summary>
	enum class GfParamType
	{
		base = 0,
		random,
		offset,
		mode,
		value,
	};

	struct GfValueTable {
		float delay;
		float rate;
		float glisson;
		float window;
		float amplitude;
		float space;
		float envelopePosition;
		float direction;
		int density;
	};

	/// <summary>
/// Parameter entity. When used with GfParamSet() different fields can be set
/// SampleParam() is used to set the value field which is what should be used to read the correct value
/// </summary>


	enum class GfStreamSetType
	{
		automaticStreams = 0,
		perStreams,
		randomStreams,
	};

	enum class GfBufferMode
	{
		normal = 0,
		buffer_sequence = 1,
		buffer_random = 2,
	};


	enum class GFBuffers
	{
		buffer = 0,
		envelope,
		rateBuffer,
		delayBuffer,
		windowBuffer
	};

	struct GfParam
	{
		float base = 0;
		float random = 0;
		float offset = 0;
		float value = 0;
		GfBufferMode mode = GfBufferMode::normal;
	};


	struct gfIoConfig
	{
		//Outputs
		double** grainOutput = nullptr;
		double** grainState = nullptr;
		double** grainProgress = nullptr;
		double** grainPlayhead = nullptr;
		double** grainAmp = nullptr;
		double** grainEnvelope = nullptr;
		double** grainBufferChannel = nullptr;
		double** grainStreamChannel = nullptr;

		//Inputs
		double** grainClock = nullptr;
		double** traversalPhasor = nullptr;
		double** fm = nullptr;
		double** am = nullptr;

		int grainClockChans;
		int traversalPhasorChans;
		int fmChans;
		int amChans;

		bool livemode = 0;
		int blockSize = 0;
		int samplerate = 1;
	};





}