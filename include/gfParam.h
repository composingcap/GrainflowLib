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
		ERR = 0,
		delay,
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
		channel,
		//These do not have param scructs
		transpose,
		glissonSt,
		stream,

	};
	/// <summary>
	/// Different parameter types using in the GfParam Struct
	/// </summary>
	enum class GfParamType
	{
		ERR = 0,
		base,
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
		manualStreams,
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

	static bool ParamReflection(std::string param, GfParamName& out_ParamName, GfParamType& out_ParamType){
		//Find and remove param types 
		out_ParamType =  GfParamType::ERR;
		if (auto pos = param.find("Random"); pos != std::string::npos){
			out_ParamType = GfParamType::random;
			param.erase(pos, 6);
		}
		else if (auto pos = param.find("Offset"); pos!= std::string::npos){
			out_ParamType = GfParamType::offset;
			param.erase(pos, 6);
		}
		else out_ParamType = GfParamType::base;
		if(out_ParamType == GfParamType::ERR) return false;

		out_ParamName =  GfParamName::ERR;
		if ( param == "delay"){out_ParamName = GfParamName::delay;}
		else if ( param == "rate"){out_ParamName = GfParamName::rate;}
		else if ( param == "window"){out_ParamName = GfParamName::window;}
		else if ( param == "rate"){out_ParamName = GfParamName::rate;}
		else if ( param == "amp"){out_ParamName = GfParamName::amplitude;}
		else if ( param == "space"){out_ParamName = GfParamName::space;}
		else if ( param == "envelopePosition"){out_ParamName = GfParamName::envelopePosition;}
		else if ( param == "nEnvelopes"){out_ParamName = GfParamName::nEnvelopes;}
		else if ( param == "direction"){out_ParamName = GfParamName::direction;}
		else if ( param == "startPoint"){out_ParamName = GfParamName::startPoint;}
		else if ( param == "stopPoint"){out_ParamName = GfParamName::stopPoint;}
		else if ( param == "rateQuantizeSemi"){out_ParamName = GfParamName::rateQuantizeSemi;}
		else if ( param == "loopMode"){out_ParamName = GfParamName::loopMode;}
		else if ( param == "channel"){out_ParamName = GfParamName::channel;}	
		//These cases are converted internally to other parameters 
		else if (param == "transpose") { out_ParamName = GfParamName::transpose; }
		else if (param == "glissonSt") { out_ParamName = GfParamName::glissonSt; }
		else if (param == "stream") { out_ParamName = GfParamName::stream; }


	
		if(out_ParamName == GfParamName::ERR) return false;

		return true;
	};







}