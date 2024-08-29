#include "IGrain.h"
#include "gfParam.h"
namespace Grainflow{
    template<typename bufferRefType, class GrainType>
    class GrainCollection{
        private:
        std::unique_ptr<GrainType[]> grains;
        int _grainCount = 0;
        int _activeGrains = 0;
        int _nstreams = 0;
        bool _autoOverlap = true;

        public:
        int samplerate = 48000;

        GrainCollection(int grainCount = 0){
            if (grainCount > 0){ 
                Resize(grainCount);
            }
        }

        ~GrainCollection(){
            grains.release(); 
        }

        void Resize(int grainCount){
            _grainCount = grainCount;
            grains.reset((new GrainType[grainCount]));
            SetActiveGrains(grainCount);
        }

        int Grains(){
            return _grainCount;
        }

        GrainType* GetGrain(int index){
            if (index >= _grainCount) return nullptr;
            return &grains[index];
        }


        #pragma region DSP
        // Proccesses all grain given an io config with the correct inputs and outputs  
        void Proccess(gfIoConfig &ioConfig){
            for (int g = 0; g < _grainCount; g++){
                grains.get()[g].Proccess(ioConfig); 
            }
        }

        #pragma endregion

        #pragma region Params

        void TransformParams(GfParamName& paramName, GfParamType& paramType, float& value) {
            switch (paramName) {
            case GfParamName::transpose:
                if (paramType == GfParamType::base) value = GfUtils::PitchToRate(value);
                else value = GfUtils::PitchOffsetToRateOffset(value);
                paramName = GfParamName::rate;
                break;
            case GfParamName::glissonSt:
                value = GfUtils::PitchOffsetToRateOffset(value);
                paramName = GfParamName::glisson;
                break;
            case GfParamName::delay:
                value = value * 0.001f * samplerate;
                break;
            case GfParamName::amplitude:
                if (paramType == GfParamType::base) break;
                value = std::max(std::min(-value, 0.0f), -1.0f);
                break;
            default:
                break;
            }
        }

        void ParamSet(int target, GfParamName paramName, GfParamType paramType, float value){
            if (target >= _grainCount) return;
            TransformParams(paramName, paramType, value);
            if (target <= 0){
                for (int g = 0; g < _grainCount; g++){
                    grains.get()[g].ParamSet(value, paramName, paramType);
                    }
                return;
            }
            grains.get()[target-1].ParamSet(value, paramName, paramType);
        }

        GF_RETURN_CODE ParamSet(int target, std::string reflectionString, float value) {
            GfParamName paramName;
            GfParamType paramType;
            auto foundReflection = Grainflow::ParamReflection(reflectionString, paramName, paramType);
            if (!foundReflection) return GF_RETURN_CODE::GF_PARAM_NOT_FOUND;
            ParamSet(target, paramName, paramType, value);
            return GF_RETURN_CODE::GF_SUCCESS;

        }

        void ChannelParamSet(int channel, GfParamName paramName, GfParamType paramType, float value){
            for(int g =0; g < _grainCount; g++){
                if (grains[g].channel.value != channel) continue;
                ParamSet(g+1, paramName, paramType, value);
            }
        }

        GF_RETURN_CODE ChannelParamSet(int channel, std::string reflectionString, float value) {
            GfParamName paramName;
            GfParamType paramType;
            auto foundReflection = Grainflow::ParamReflection(reflectionString, paramName, paramType);
            if (!foundReflection) return GF_RETURN_CODE::GF_PARAM_NOT_FOUND;
            ChannelParamSet(channel, paramName, paramType, value);
            return GF_RETURN_CODE::GF_SUCCESS;
        }

        GF_RETURN_CODE GrainParamFunc(GfParamName paramName, GfParamType paramType, float (*func)(float, float, float), float a, float b) {
            for (int g = 0; g < _grainCount; g++) {
                float value = (*func)(a, b, (float)g / _grainCount);
                ParamSet(g, paramName, paramType, value);
            }
            return GF_RETURN_CODE::GF_SUCCESS;
        }

        GF_RETURN_CODE GrainParamFunc(std::string reflectionString, float (*func)(float, float, float), float a, float b) {
            GfParamName paramName;
            GfParamType paramType;
            auto foundReflection = Grainflow::ParamReflection(reflectionString, paramName, paramType);
            if (!foundReflection) return GF_RETURN_CODE::GF_PARAM_NOT_FOUND;
            return GrainParamFunc(paramName, paramType, func, a, b);
        }


        float ParamGet(int target, GfParamName paramName){
            if (target >= _grainCount) return;
            if (target <= 1) return grains.get()[0].ParamGet(paramName);
            return grains.get()[target-1].ParamGet(paramName);
        }


        float ParamGet(int target, GfParamName paramName, GfParamType paramType){
            if (target > _grainCount) return 0;
            if (target <= 1) return grains.get()[0].ParamGet(paramName, paramType);
            return grains.get()[target-1].ParamGet(paramName, paramType);
        }

        void SetDensity(int target, float value){
            if (target > 0) {
			grains[target - 1].density = value;
			return;
			}
            for (int g = 0; g < _grainCount; g++)
                {
                    grains[g].density = value;
                }
            }

        void SetActiveGrains(int nGrains){
            if (nGrains <= 0) nGrains = 0;
            else if (nGrains > _grainCount) nGrains = _grainCount;
            _activeGrains = nGrains;
            if (_autoOverlap){
                auto windowOffset = 1.0f / (nGrains > 0 ? nGrains : 1);
                ParamSet(0, GfParamName::window, GfParamType::offset, windowOffset);
            }
        }

        int ActiveGrains(){
            return _activeGrains;
        }

        void SetAutoOverlap(bool autoOverlap){
            _autoOverlap = autoOverlap;
            SetActiveGrains(_activeGrains);
        }
        #pragma endregion

        #pragma region STREAMS

        int Streams(){return _nstreams;}
        GF_RETURN_CODE StreamParamSet(int stream, GfParamName paramName, GfParamType paramType, float value){
            if (stream > _nstreams || stream < 0) return GF_RETURN_CODE::GF_ERR;
            for(int g =0; g < _grainCount; g++){
                if (grains[g].stream != stream && stream != 0) continue;
                ParamSet(g, paramName, paramType, value);
            }
            return GF_RETURN_CODE::GF_SUCCESS;
        }

        GF_RETURN_CODE StreamParamSet(std::string reflectionString, int stream, float value){
            GfParamName paramName;
			GfParamType paramType;
			auto foundReflection = Grainflow::ParamReflection(reflectionString, paramName, paramType);
			if (!foundReflection) return GF_RETURN_CODE::GF_PARAM_NOT_FOUND;
			return StreamParamSet(stream, paramName, paramType, value);
			
        }

        GF_RETURN_CODE StreamParamFunc(GfParamName paramName, GfParamType paramType, float (*func)(float, float, float), float a, float b) {
            for (int s = 0; s < _nstreams; s++)
            {
                float value = func(a, b, (float)s / _nstreams);
                auto returnCode = StreamParamSet(s, paramName, paramType, value);
                if (returnCode != GF_RETURN_CODE::GF_SUCCESS) return returnCode;
            }
            return GF_RETURN_CODE::GF_SUCCESS;
        }
        GF_RETURN_CODE StreamParamFunc(std::string reflectionString, float (*func)(float, float, float), float a, float b) {
            GfParamName paramName;
            GfParamType paramType;
            auto foundReflection = Grainflow::ParamReflection(reflectionString, paramName, paramType);
            if (!foundReflection) return GF_RETURN_CODE::GF_PARAM_NOT_FOUND;
            return StreamParamFunc(paramName, paramType, func, a, b);

        }

        void StreamSet(GfStreamSetType mode, int nstreams){
            _nstreams = nstreams;
            for (int g = 0; g< _grainCount; g++){
				grains[g].StreamSet(_grainCount, mode, nstreams);
			}
        }

        int StreamGet(int grainIndex){
            return (int)grains[grainIndex].stream;
        };

        #pragma endregion

        bufferRefType* GetBuffer(GFBuffers type, int index = 0){return grains[index].GetBuffer(type);}

        int ChanelGet(int index){return grains[index].channel.base;}
        void ChannelsSetInterleaved(int channels){
            for (int g = 0; g < _grainCount; g++){
                grains[g].channel.base = g % channels;
            }
        }
        void ChannelSet(int index, int channel){grains[index].channel.base = channel;}
        void ChannelModeSet(int mode){
             for (int g = 0; g < _grainCount; g++){
                grains[g].channel.random = mode;
            }
        }
        //TODO 
        // - Implement Parameter Name reflections 
        // - Remove any need to access individual grains 
        //  -Streams
        //  -Buffer Channels
        // - Ensure all functionallity is still operational 

    };

}