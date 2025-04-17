#include <array>
namespace Grainflow{

    struct biquad_params{
        float a1{0};
        float a2{0};
        float b0{0};
        float b1{0};
        float b2{0}; 
    };


    template<typename SigType = double>
    class biquad{
    private:
    SigType* mem;
    static constexpr int mem_needed = 4 * sizeof(SigType);
    public:
    biquad(){
        mem = reinterpret_cast<SigType*>(malloc(mem_needed));
        clear();
    }
    ~biquad(){
        free(mem);
    }

    void clear(){
        std::fill_n(mem,4,0);
    }

    void perform(SigType* __restrict block, const int blocksize, const biquad_params& params, SigType* __restrict out){
        for (int i = 0; i < blocksize; ++i){
            //This in not optimized-- basic biquad 
            out[i] = block[i] * params.b0 + mem[0] * params.b1 + mem[1] * params.b2 - ( mem[2] * params.a1 + mem[3] * params.a2 );
            mem[1] = mem[0];
            mem[3] = mem[2];
            mem[0] = block[i];
            mem[2] = out[i];
        }
    }
    static void get_needed_mem_size(int& mem_size){
        mem_size = mem_needed;
    }

    static void bandpass_params(biquad_params& params, float center, float bandwidth, int fs){
        float omega = center * M_2_PI/fs;
        float sn = std::sin(omega);
        float cs = std::cos(omega);
        float alpha = sn * 0.5  * (bandwidth/center);
        float a0 = 1.0f/(1+alpha);
        params.b0 = alpha * a0;
        params.b1 = 0;
        params.b2 = -alpha*a0;
        params.a1 = -2.0f * cs * a0;
        params.a2 = (1.0f - alpha) * a0;
    }


    };
}