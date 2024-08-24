#include "IGrain.h"
#include "gfParam.h"
namespace Grainflow{
    template<class GrainType>
    class GrainCollection{
        private:
        std::unique_ptr<GrainType[]> grains;
        int _grainCount = 0;

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

        #pragma region SET_GET
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

        void StreamParamSet(int stream, GfParamName paramName, GfParamType paramType, float value){
            for(int g =0; g < _grainCount; g++){
                if (grains[g].stream != stream) continue;
                ParamSet(g+1, paramName, paramType, value);
            }
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
        #pragma endregion

    };

}