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
        void ParamSet(int target, GfParamName paramName, GfParamType paramType, float value){
            if (target >= _grainCount) return;
            if (target <= 0){
                for (int g = 0; g < _grainCount; g++){
                    grains.get()[g].ParamSet(value, paramName, paramType);
                    }
                return;
            }
            grains.get()[target-1].ParamSet(value, paramName, paramType);
        }

        void ChannelParamSet(int channel, GfParamName paramName, GfParamType paramType, float value){
            for(int g =0; g < _grainCount; g++){
                if (grains[g].channel.value != channel) continue;
                ParamSet(g+1, paramName, paramType, value);
            }
        }

        void ParamDeviate(GfParamName paramName, GfParamType paramType, float deviation, float center){
                for (int g = 0; g < _grainCount; g++){
                    auto value = GfUtils::Deviate(center, deviation);
                    grains.get()[g].ParamSet(value, paramName, paramType);
                    }
        }

        void ParamRandomRange(GfParamName paramName, GfParamType paramType, float low, float high){
                for (int g = 0; g < _grainCount; g++){
                    auto value = GfUtils::RandomRange(low, high);
                    grains.get()[g].ParamSet(value, paramName, paramType);
                    }
        }

        void ParamSpread(GfParamName paramName, GfParamType paramType, float low, float high){
                for (int g = 0; g < _grainCount; g++){
                    auto value = GfUtils::Lerp(low, high, g/_grainCount);
                    grains.get()[g].ParamSet(value, paramName, paramType);
                    }
        }

        float ParamGet(int target, GfParamName paramName){
            if (target >= _grainCount) return;
            if (target <= 1) return grains.get()[0].ParamGet(paramName);
            return grains.get()[target-1].ParamGet(paramName);
        }


        float ParamGet(int target, GfParamName paramName, GfParamType paramType){
            if (target >= _grainCount) return;
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

        GF_RETURN_CODE StreamParamDeviate(std::string reflectionString, float deviation, float center){
            GfParamName paramName;
			GfParamType paramType;
			auto foundReflection = Grainflow::ParamReflection(reflectionString, paramName, paramType);
			if (!foundReflection) return GF_RETURN_CODE::GF_PARAM_NOT_FOUND;
            for (int s = 0; s < _nstreams; s++)
			{
				auto value = GfUtils::Deviate(deviation, center);
				auto returnCode = StreamParamSet(s, paramName, paramType, value);
                if (returnCode!=GF_RETURN_CODE::GF_SUCCESS) return returnCode;
			}
            return GF_RETURN_CODE::GF_SUCCESS;
        }

         GF_RETURN_CODE StreamParamRandomRange(std::string reflectionString, float low, float high){
            GfParamName paramName;
			GfParamType paramType;
			auto foundReflection = Grainflow::ParamReflection(reflectionString, paramName, paramType);
			if (!foundReflection) return GF_RETURN_CODE::GF_PARAM_NOT_FOUND;
            for (int s = 0; s < _nstreams; s++)
			{
				auto value = GfUtils::RandomRange(low, high);
				auto returnCode = StreamParamSet(s, paramName, paramType, value);
                if (returnCode != GF_RETURN_CODE::GF_SUCCESS) return returnCode;
			}
            return GF_RETURN_CODE::GF_SUCCESS;
        }

         GF_RETURN_CODE StreamParamSpread(std::string reflectionString, float low, float high){
            GfParamName paramName;
			GfParamType paramType;
			auto foundReflection = Grainflow::ParamReflection(reflectionString, paramName, paramType);
			if (!foundReflection) return GF_RETURN_CODE::GF_PARAM_NOT_FOUND;
            for (int s = 0; s < _nstreams; s++)
			{
				auto value = GfUtils::Lerp(low, high, _nstreams > 0 ? s/_nstreams : 1);
				auto returnCode = StreamParamSet(s, paramName, paramType, value);
                if (returnCode != GF_RETURN_CODE::GF_SUCCESS) return returnCode;
			}
            return GF_RETURN_CODE::GF_SUCCESS;
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