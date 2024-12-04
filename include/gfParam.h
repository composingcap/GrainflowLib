#pragma once
#include <map>
#include <string>
#include <algorithm>

namespace Grainflow
{
	/// <summary>
	/// Available parameters using the GfParam struct
	/// </summary>
	enum class gf_param_name : std::uint8_t
	{
		ERR = 0,
		delay,
		rate,
		glisson,
		glisson_rows,
		glisson_position,
		window,
		amplitude,
		space,
		envelope_position,
		n_envelopes,
		direction,
		start_point,
		stop_point,
		rate_quantize_semi,
		loop_mode,
		channel,
		density,

		//These do not have param structs
		transpose,
		glisson_st,
		stream,
	};

	/// <summary>
	/// Different parameter types using in the GfParam Struct
	/// </summary>
	enum class gf_param_type : std::uint8_t
	{
		ERR = 0,
		base,
		random,
		offset,
		mode,
		value,
	};

	struct gf_value_table
	{
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


	enum class gf_stream_set_type : std::uint8_t
	{
		automatic_streams = 0,
		per_streams,
		random_streams,
		manual_streams,
	};

	enum class gf_buffer_mode : std::uint8_t
	{
		normal = 0,
		buffer_sequence = 1,
		buffer_random = 2,
	};


	enum class gf_buffers : std::uint8_t
	{
		buffer = 0,
		envelope,
		rate_buffer,
		delay_buffer,
		window_buffer,
		glisson_buffer,
	};

	struct gf_param
	{
		float base = 0;
		float random = 0;
		float offset = 0;
		float value = 0;
		gf_buffer_mode mode = gf_buffer_mode::normal;
	};


	static bool param_reflection(std::string param, gf_param_name& out_param_name, gf_param_type& out_param_type)
	{
		//Find and remove param types 
		out_param_type = gf_param_type::ERR;
		if (const auto pos = param.find("Random"); pos != std::string::npos)
		{
			out_param_type = gf_param_type::random;
			param.erase(pos, 6);
		}
		else if (const auto pos = param.find("Offset"); pos != std::string::npos)
		{
			out_param_type = gf_param_type::offset;
			param.erase(pos, 6);
		}
		else out_param_type = gf_param_type::base;
		if (out_param_type == gf_param_type::ERR) return false;

		out_param_name = gf_param_name::ERR;
		if (param == "delay") { out_param_name = gf_param_name::delay; }
		else if (param == "rate") { out_param_name = gf_param_name::rate; }
		else if (param == "window") { out_param_name = gf_param_name::window; }
		else if (param == "rate") { out_param_name = gf_param_name::rate; }
		else if (param == "amp") { out_param_name = gf_param_name::amplitude; }
		else if (param == "space") { out_param_name = gf_param_name::space; }
		else if (param == "envelopePosition") { out_param_name = gf_param_name::envelope_position; }
		else if (param == "nEnvelopes") { out_param_name = gf_param_name::n_envelopes; }
		else if (param == "direction") { out_param_name = gf_param_name::direction; }
		else if (param == "startPoint") { out_param_name = gf_param_name::start_point; }
		else if (param == "stopPoint") { out_param_name = gf_param_name::stop_point; }
		else if (param == "rateQuantizeSemi") { out_param_name = gf_param_name::rate_quantize_semi; }
		else if (param == "loopMode") { out_param_name = gf_param_name::loop_mode; }
		else if (param == "channel") { out_param_name = gf_param_name::channel; }
		else if (param == "density") { out_param_name = gf_param_name::density; }
		//These cases are converted internally to other parameters 
		else if (param == "transpose") { out_param_name = gf_param_name::transpose; }
		else if (param == "glissonSt") { out_param_name = gf_param_name::glisson_st; }
		else if (param == "stream") { out_param_name = gf_param_name::stream; }


		if (out_param_name == gf_param_name::ERR) return false;

		return true;
	};
}
