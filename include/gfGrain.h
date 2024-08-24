#include "IGrain.h"
namespace Grainflow{
    //Class for grains using an internal grainflow buffer
    template <int blocksize>
    class gfGrain : IGrain<gfBuffer, &gfBuffer, blocksize>{

    };

    //Internal buffer class for grainflow
    class gfBuffer{

    };
};