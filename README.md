GrainflowLib is a lightweight header only library for granulation.

To use the library you first need to include `./include` in your project and write an interface that extends IGrain for your application.

## Setting up grainflow for your project

An example of how to implement grainflowLib as a Max external can be found in the examples folder. 

It should be fairly easy to adapt grainflowlib to any audio library that uses a buffer structure. You need to:
1. Write an interface for IGrain for you platform
2. Write some glue code 
3. Write code to set grainflow parameters 

### Creating your IGrain interface

An interface for IGrain requires two template arguments.
```
T1 : A data structure that can be used to refrence a buffer 

T2 : A data structure that can read from that buffer
```

You will need to implement these virtual functions in your interface that will store and read values from those buffers.
```
virtual void SampleParamBuffer(GFBuffers bufferType, GfParamName paramName) = 0;

virtual float SampleBuffer(T2 &sampleLock) = 0;

virtual float SampleEnvelope(T2 &sampleLock, float grainClock) = 0;
```

Also note you will need to use the `void SetBuffer(GFBuffers bufferType, T1 *buffer` function to maintain a per-grain refrence to your buffers. This is vital for the `SampleParamBuffer()` function which reads values from an external buffer to determine parameter values.

### Proccessing Code
Overall the structure of the code with be something like: 
```
ProccessSample(i, grainClock, samplePosition, sampleSource, externalPlaySpeed, envelopeSource){
    Grainflow::GrainReset(grainClock, samplePosition);
    sample = Grainflow::SampleBuffer(sampleSource);
    envelope = Grainflow::SampleEnvelop(envelopeSource, grainClock);
    output[i] = sample*envelope;
    Grainflow::Increment(externalPlaySpeed, grainClock)
}
```

### Parameters 
`gfParam.h`  contains a data structure to help set various grainflow parameters. `IGrain` uses the `GfParam` class to set an manage various parameters. 

#### Setting Parameters

The `ParamSet()` function should be used to set a grainflow parameter for example:
```
//Set the base value of delay in ms 
myGrain.ParamSet(250, GfParamName::delay, GdParamType::base); 

//Set a random deviation from 0-1000 ms that is applied on grain reset
myGrain.ParamSet(1000, GfParamName::delay, GdParamType::random);

//Apply an 50 times the grains index value to this grain
myGrain.ParamSet(50, GfParamName::delay, GdParamType::offset);
```
Parameters set their value field when a `GrainReset` occures. This happens when the Grainclock passes zero (the deviation is negitive).

Buffers grainflow reads should be set using the `SetBuffer()` method and later accessed using `GetBuffer()` 
