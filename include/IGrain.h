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
constexpr float HanningEnvelope[1024] = {
                0.0000 , 0.0000 , 0.0000 , 0.0001 , 0.0002 , 0.0002 , 0.0003 , 0.0005 , 0.0006 , 0.0008 , 0.0009 , 0.0011 , 0.0014 , 0.0016 , 0.0018 , 0.0021 , 0.0024 , 0.0027 , 0.0030 , 0.0034 , 0.0038 , 0.0041 , 0.0045 , 0.0050 , 0.0054 , 0.0059 , 0.0063 , 0.0068 , 0.0074 , 0.0079 , 0.0084 , 0.0090 , 0.0096 , 0.0102 , 0.0108 , 0.0115 , 0.0121 , 0.0128 , 0.0135 , 0.0142 , 0.0150 , 0.0157 , 0.0165 , 0.0173 , 0.0181 , 0.0189 , 0.0198 , 0.0206 , 0.0215 , 0.0224 , 0.0233 , 0.0243 , 0.0252 , 0.0262 , 0.0272 , 0.0282 , 0.0292 , 0.0303 , 0.0313 , 0.0324 , 0.0335 , 0.0346 , 0.0357 , 0.0369 , 0.0381 , 0.0392 , 0.0404 , 0.0417 , 0.0429 , 0.0441 , 0.0454 , 0.0467 , 0.0480 , 0.0493 , 0.0507 , 0.0520 , 0.0534 , 0.0548 , 0.0562 , 0.0576 , 0.0590 , 0.0605 , 0.0620 , 0.0635 , 0.0650 , 0.0665 , 0.0680 , 0.0696 , 0.0711 , 0.0727 , 0.0743 , 0.0759 , 0.0776 , 0.0792 , 0.0809 , 0.0826 , 0.0843 , 0.0860 , 0.0877 , 0.0894 , 0.0912 , 0.0930 , 0.0948 , 0.0966 , 0.0984 , 0.1002 , 0.1021 , 0.1039 , 0.1058 , 0.1077 , 0.1096 , 0.1116 , 0.1135 , 0.1154 , 0.1174 , 0.1194 , 0.1214 , 0.1234 , 0.1254 , 0.1275 , 0.1295 , 0.1316 , 0.1337 , 0.1358 , 0.1379 , 0.1400 , 0.1421 , 0.1443 , 0.1464 , 0.1486 , 0.1508 , 0.1530 , 0.1552 , 0.1575 , 0.1597 , 0.1620 , 0.1642 , 0.1665 , 0.1688 , 0.1711 , 0.1734 , 0.1757 , 0.1781 , 0.1804 , 0.1828 , 0.1852 , 0.1876 , 0.1900 , 0.1924 , 0.1948 , 0.1972 , 0.1997 , 0.2022 , 0.2046 , 0.2071 , 0.2096 , 0.2121 , 0.2146 , 0.2171 , 0.2197 , 0.2222 , 0.2248 , 0.2273 , 0.2299 , 0.2325 , 0.2351 , 0.2377 , 0.2403 , 0.2429 , 0.2456 , 0.2482 , 0.2509 , 0.2536 , 0.2562 , 0.2589 , 0.2616 , 0.2643 , 0.2670 , 0.2697 , 0.2725 , 0.2752 , 0.2779 , 0.2807 , 0.2835 , 0.2862 , 0.2890 , 0.2918 , 0.2946 , 0.2974 , 0.3002 , 0.3030 , 0.3058 , 0.3087 , 0.3115 , 0.3143 , 0.3172 , 0.3201 , 0.3229 , 0.3258 , 0.3287 , 0.3316 , 0.3344 , 0.3373 , 0.3402 , 0.3432 , 0.3461 , 0.3490 , 0.3519 , 0.3549 , 0.3578 , 0.3607 , 0.3637 , 0.3666 , 0.3696 , 0.3726 , 0.3755 , 0.3785 , 0.3815 , 0.3845 , 0.3875 , 0.3904 , 0.3934 , 0.3964 , 0.3994 , 0.4025 , 0.4055 , 0.4085 , 0.4115 , 0.4145 , 0.4175 , 0.4206 , 0.4236 , 0.4266 , 0.4297 , 0.4327 , 0.4358 , 0.4388 , 0.4418 , 0.4449 , 0.4479 , 0.4510 , 0.4540 , 0.4571 , 0.4602 , 0.4632 , 0.4663 , 0.4693 , 0.4724 , 0.4755 , 0.4785 , 0.4816 , 0.4847 , 0.4877 , 0.4908 , 0.4939 , 0.4969 , 0.5000 , 0.5031 , 0.5061 , 0.5092 , 0.5123 , 0.5153 , 0.5184 , 0.5215 , 0.5245 , 0.5276 , 0.5307 , 0.5337 , 0.5368 , 0.5398 , 0.5429 , 0.5460 , 0.5490 , 0.5521 , 0.5551 , 0.5582 , 0.5612 , 0.5642 , 0.5673 , 0.5703 , 0.5734 , 0.5764 , 0.5794 , 0.5825 , 0.5855 , 0.5885 , 0.5915 , 0.5945 , 0.5975 , 0.6006 , 0.6036 , 0.6066 , 0.6096 , 0.6125 , 0.6155 , 0.6185 , 0.6215 , 0.6245 , 0.6274 , 0.6304 , 0.6334 , 0.6363 , 0.6393 , 0.6422 , 0.6451 , 0.6481 , 0.6510 , 0.6539 , 0.6568 , 0.6598 , 0.6627 , 0.6656 , 0.6684 , 0.6713 , 0.6742 , 0.6771 , 0.6799 , 0.6828 , 0.6857 , 0.6885 , 0.6913 , 0.6942 , 0.6970 , 0.6998 , 0.7026 , 0.7054 , 0.7082 , 0.7110 , 0.7138 , 0.7165 , 0.7193 , 0.7221 , 0.7248 , 0.7275 , 0.7303 , 0.7330 , 0.7357 , 0.7384 , 0.7411 , 0.7438 , 0.7464 , 0.7491 , 0.7518 , 0.7544 , 0.7571 , 0.7597 , 0.7623 , 0.7649 , 0.7675 , 0.7701 , 0.7727 , 0.7752 , 0.7778 , 0.7803 , 0.7829 , 0.7854 , 0.7879 , 0.7904 , 0.7929 , 0.7954 , 0.7978 , 0.8003 , 0.8028 , 0.8052 , 0.8076 , 0.8100 , 0.8124 , 0.8148 , 0.8172 , 0.8196 , 0.8219 , 0.8243 , 0.8266 , 0.8289 , 0.8312 , 0.8335 , 0.8358 , 0.8380 , 0.8403 , 0.8425 , 0.8448 , 0.8470 , 0.8492 , 0.8514 , 0.8536 , 0.8557 , 0.8579 , 0.8600 , 0.8621 , 0.8642 , 0.8663 , 0.8684 , 0.8705 , 0.8725 , 0.8746 , 0.8766 , 0.8786 , 0.8806 , 0.8826 , 0.8846 , 0.8865 , 0.8884 , 0.8904 , 0.8923 , 0.8942 , 0.8961 , 0.8979 , 0.8998 , 0.9016 , 0.9034 , 0.9052 , 0.9070 , 0.9088 , 0.9106 , 0.9123 , 0.9140 , 0.9157 , 0.9174 , 0.9191 , 0.9208 , 0.9224 , 0.9241 , 0.9257 , 0.9273 , 0.9289 , 0.9304 , 0.9320 , 0.9335 , 0.9350 , 0.9365 , 0.9380 , 0.9395 , 0.9410 , 0.9424 , 0.9438 , 0.9452 , 0.9466 , 0.9480 , 0.9493 , 0.9507 , 0.9520 , 0.9533 , 0.9546 , 0.9559 , 0.9571 , 0.9583 , 0.9596 , 0.9608 , 0.9619 , 0.9631 , 0.9643 , 0.9654 , 0.9665 , 0.9676 , 0.9687 , 0.9697 , 0.9708 , 0.9718 , 0.9728 , 0.9738 , 0.9748 , 0.9757 , 0.9767 , 0.9776 , 0.9785 , 0.9794 , 0.9802 , 0.9811 , 0.9819 , 0.9827 , 0.9835 , 0.9843 , 0.9850 , 0.9858 , 0.9865 , 0.9872 , 0.9879 , 0.9885 , 0.9892 , 0.9898 , 0.9904 , 0.9910 , 0.9916 , 0.9921 , 0.9926 , 0.9932 , 0.9937 , 0.9941 , 0.9946 , 0.9950 , 0.9955 , 0.9959 , 0.9962 , 0.9966 , 0.9970 , 0.9973 , 0.9976 , 0.9979 , 0.9982 , 0.9984 , 0.9986 , 0.9989 , 0.9991 , 0.9992 , 0.9994 , 0.9995 , 0.9997 , 0.9998 , 0.9998 , 0.9999 , 1.0000 , 1.0000 , 1.0000 , 1.0000 , 1.0000 , 0.9999 , 0.9998 , 0.9998 , 0.9997 , 0.9995 , 0.9994 , 0.9992 , 0.9991 , 0.9989 , 0.9986 , 0.9984 , 0.9982 , 0.9979 , 0.9976 , 0.9973 , 0.9970 , 0.9966 , 0.9962 , 0.9959 , 0.9955 , 0.9950 , 0.9946 , 0.9941 , 0.9937 , 0.9932 , 0.9926 , 0.9921 , 0.9916 , 0.9910 , 0.9904 , 0.9898 , 0.9892 , 0.9885 , 0.9879 , 0.9872 , 0.9865 , 0.9858 , 0.9850 , 0.9843 , 0.9835 , 0.9827 , 0.9819 , 0.9811 , 0.9802 , 0.9794 , 0.9785 , 0.9776 , 0.9767 , 0.9757 , 0.9748 , 0.9738 , 0.9728 , 0.9718 , 0.9708 , 0.9697 , 0.9687 , 0.9676 , 0.9665 , 0.9654 , 0.9643 , 0.9631 , 0.9619 , 0.9608 , 0.9596 , 0.9583 , 0.9571 , 0.9559 , 0.9546 , 0.9533 , 0.9520 , 0.9507 , 0.9493 , 0.9480 , 0.9466 , 0.9452 , 0.9438 , 0.9424 , 0.9410 , 0.9395 , 0.9380 , 0.9365 , 0.9350 , 0.9335 , 0.9320 , 0.9304 , 0.9289 , 0.9273 , 0.9257 , 0.9241 , 0.9224 , 0.9208 , 0.9191 , 0.9174 , 0.9157 , 0.9140 , 0.9123 , 0.9106 , 0.9088 , 0.9070 , 0.9052 , 0.9034 , 0.9016 , 0.8998 , 0.8979 , 0.8961 , 0.8942 , 0.8923 , 0.8904 , 0.8884 , 0.8865 , 0.8846 , 0.8826 , 0.8806 , 0.8786 , 0.8766 , 0.8746 , 0.8725 , 0.8705 , 0.8684 , 0.8663 , 0.8642 , 0.8621 , 0.8600 , 0.8579 , 0.8557 , 0.8536 , 0.8514 , 0.8492 , 0.8470 , 0.8448 , 0.8425 , 0.8403 , 0.8380 , 0.8358 , 0.8335 , 0.8312 , 0.8289 , 0.8266 , 0.8243 , 0.8219 , 0.8196 , 0.8172 , 0.8148 , 0.8124 , 0.8100 , 0.8076 , 0.8052 , 0.8028 , 0.8003 , 0.7978 , 0.7954 , 0.7929 , 0.7904 , 0.7879 , 0.7854 , 0.7829 , 0.7803 , 0.7778 , 0.7752 , 0.7727 , 0.7701 , 0.7675 , 0.7649 , 0.7623 , 0.7597 , 0.7571 , 0.7544 , 0.7518 , 0.7491 , 0.7464 , 0.7438 , 0.7411 , 0.7384 , 0.7357 , 0.7330 , 0.7303 , 0.7275 , 0.7248 , 0.7221 , 0.7193 , 0.7165 , 0.7138 , 0.7110 , 0.7082 , 0.7054 , 0.7026 , 0.6998 , 0.6970 , 0.6942 , 0.6913 , 0.6885 , 0.6857 , 0.6828 , 0.6799 , 0.6771 , 0.6742 , 0.6713 , 0.6684 , 0.6656 , 0.6627 , 0.6598 , 0.6568 , 0.6539 , 0.6510 , 0.6481 , 0.6451 , 0.6422 , 0.6393 , 0.6363 , 0.6334 , 0.6304 , 0.6274 , 0.6245 , 0.6215 , 0.6185 , 0.6155 , 0.6125 , 0.6096 , 0.6066 , 0.6036 , 0.6006 , 0.5975 , 0.5945 , 0.5915 , 0.5885 , 0.5855 , 0.5825 , 0.5794 , 0.5764 , 0.5734 , 0.5703 , 0.5673 , 0.5642 , 0.5612 , 0.5582 , 0.5551 , 0.5521 , 0.5490 , 0.5460 , 0.5429 , 0.5398 , 0.5368 , 0.5337 , 0.5307 , 0.5276 , 0.5245 , 0.5215 , 0.5184 , 0.5153 , 0.5123 , 0.5092 , 0.5061 , 0.5031 , 0.5000 , 0.4969 , 0.4939 , 0.4908 , 0.4877 , 0.4847 , 0.4816 , 0.4785 , 0.4755 , 0.4724 , 0.4693 , 0.4663 , 0.4632 , 0.4602 , 0.4571 , 0.4540 , 0.4510 , 0.4479 , 0.4449 , 0.4418 , 0.4388 , 0.4358 , 0.4327 , 0.4297 , 0.4266 , 0.4236 , 0.4206 , 0.4175 , 0.4145 , 0.4115 , 0.4085 , 0.4055 , 0.4025 , 0.3994 , 0.3964 , 0.3934 , 0.3904 , 0.3875 , 0.3845 , 0.3815 , 0.3785 , 0.3755 , 0.3726 , 0.3696 , 0.3666 , 0.3637 , 0.3607 , 0.3578 , 0.3549 , 0.3519 , 0.3490 , 0.3461 , 0.3432 , 0.3402 , 0.3373 , 0.3344 , 0.3316 , 0.3287 , 0.3258 , 0.3229 , 0.3201 , 0.3172 , 0.3143 , 0.3115 , 0.3087 , 0.3058 , 0.3030 , 0.3002 , 0.2974 , 0.2946 , 0.2918 , 0.2890 , 0.2862 , 0.2835 , 0.2807 , 0.2779 , 0.2752 , 0.2725 , 0.2697 , 0.2670 , 0.2643 , 0.2616 , 0.2589 , 0.2562 , 0.2536 , 0.2509 , 0.2482 , 0.2456 , 0.2429 , 0.2403 , 0.2377 , 0.2351 , 0.2325 , 0.2299 , 0.2273 , 0.2248 , 0.2222 , 0.2197 , 0.2171 , 0.2146 , 0.2121 , 0.2096 , 0.2071 , 0.2046 , 0.2022 , 0.1997 , 0.1972 , 0.1948 , 0.1924 , 0.1900 , 0.1876 , 0.1852 , 0.1828 , 0.1804 , 0.1781 , 0.1757 , 0.1734 , 0.1711 , 0.1688 , 0.1665 , 0.1642 , 0.1620 , 0.1597 , 0.1575 , 0.1552 , 0.1530 , 0.1508 , 0.1486 , 0.1464 , 0.1443 , 0.1421 , 0.1400 , 0.1379 , 0.1358 , 0.1337 , 0.1316 , 0.1295 , 0.1275 , 0.1254 , 0.1234 , 0.1214 , 0.1194 , 0.1174 , 0.1154 , 0.1135 , 0.1116 , 0.1096 , 0.1077 , 0.1058 , 0.1039 , 0.1021 , 0.1002 , 0.0984 , 0.0966 , 0.0948 , 0.0930 , 0.0912 , 0.0894 , 0.0877 , 0.0860 , 0.0843 , 0.0826 , 0.0809 , 0.0792 , 0.0776 , 0.0759 , 0.0743 , 0.0727 , 0.0711 , 0.0696 , 0.0680 , 0.0665 , 0.0650 , 0.0635 , 0.0620 , 0.0605 , 0.0590 , 0.0576 , 0.0562 , 0.0548 , 0.0534 , 0.0520 , 0.0507 , 0.0493 , 0.0480 , 0.0467 , 0.0454 , 0.0441 , 0.0429 , 0.0417 , 0.0404 , 0.0392 , 0.0381 , 0.0369 , 0.0357 , 0.0346 , 0.0335 , 0.0324 , 0.0313 , 0.0303 , 0.0292 , 0.0282 , 0.0272 , 0.0262 , 0.0252 , 0.0243 , 0.0233 , 0.0224 , 0.0215 , 0.0206 , 0.0198 , 0.0189 , 0.0181 , 0.0173 , 0.0165 , 0.0157 , 0.0150 , 0.0142 , 0.0135 , 0.0128 , 0.0121 , 0.0115 , 0.0108 , 0.0102 , 0.0096 , 0.0090 , 0.0084 , 0.0079 , 0.0074 , 0.0068 , 0.0063 , 0.0059 , 0.0054 , 0.0050 , 0.0045 , 0.0041 , 0.0038 , 0.0034 , 0.0030 , 0.0027 , 0.0024 , 0.0021 , 0.0018 , 0.0016 , 0.0014 , 0.0011 , 0.0009 , 0.0008 , 0.0006 , 0.0005 , 0.0003 , 0.0002 , 0.0002 , 0.0001 , 0.0000 , 0.0000
        }; 

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

		bool useDefaultEnvelope = true;
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

        inline GfValueTable* GrainReset(double* __restrict grainClock, const double* traversal, double* __restrict grainState, const int size)
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
			grainEnabled = density > (rd() % 10000) * 0.0001f;
		}

		inline void ExpandValueTable(const  GfValueTable* __restrict valueFrames, const double* __restrict grainState, float* __restrict amplitudes, float* __restrict densities, const int size) {
			for (int j = 0; j < size; j++) {
				amplitudes[j] = valueFrames[(int)grainState[j]].amplitude;
				densities[j] = valueFrames[(int)grainState[j]].density;
			}
		}
		inline void ProccessGrainClock(const double* __restrict grainClock, double* __restrict grainProgress, const float windowVal, const float windowPortion, const int size) {
			for (int j = 0; j < size; j++) {
				double sample = grainClock[j] + windowVal;
				sample -= floor(sample);
				sample *= windowPortion;
				grainProgress[j] = sample;
			}
			for (int j = 0; j < size; j++) {
				grainProgress[j] = std::min(grainProgress[j], 1.0);

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

        inline void Increment(const double* __restrict fm, const double* __restrict grainClock, double* __restrict samplePositions, double* __restrict sampleDeltaTemp, const int size)
		{
			int fold = loopMode.base > 1.1f ? 1 : 0;
			double start = std::min((double)bufferFrames * startPoint.value, (double)bufferFrames-1);
			double end = std::min((double)bufferFrames * stopPoint.value, (double)bufferFrames - 1);

			for (int i = 0; i < size; i++) {
				sampleDeltaTemp[i] = GfUtils::PitchToRate(fm[i]);
			}
			for (int i = 0; i < size; i++) {
				sampleDeltaTemp[i] *= sampleRateAdjustment * rate.value * (1 + glisson.value * grainClock[i]) * direction.value;
			}
			samplePositions[0] = sourceSample;
			double lastPosition = sourceSample;
			for (int i = 1; i < size; i++) {
				samplePositions[i] = lastPosition + sampleDeltaTemp[i-1];
				lastPosition = samplePositions[i];
			}
			sourceSample = samplePositions[size - 1] + sampleDeltaTemp[size - 1];
			sourceSample = GfUtils::pong(sourceSample, start, end, fold);
			sourceSample = std::clamp(sourceSample, start, end);

			for (int i = 0; i < size; i++) {
				samplePositions[i] = GfUtils::pong(samplePositions[i], start, end, fold);
			}
			for (int i = 0; i < size; i++) {
				samplePositions[i] = std::clamp(samplePositions[i], start, end);
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
