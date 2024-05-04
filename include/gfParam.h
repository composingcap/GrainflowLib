#pragma once
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
		double** in;
		double** out;
		int grainClockCh = 0;
		int traversalPhasorCh;
		int fmCh;
		int amCh;
		int grainOutput = 0;
		int grainState;
		int grainProgress;
		int grainPlayhead;
		int grainAmp;
		int grainEnvelope;
		int grainBufferChannel;
		int grainStreamChannel;
		int grainClock;
		int traversalPhasor;
		int fm;
		int am;
		bool livemode;
		int blockSize;
		int samplerate;
	};


}