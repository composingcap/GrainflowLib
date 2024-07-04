GrainflowLib is a lightweight header only library for granulation.

To use the library you first need to include `./include` in your project and write an interface that extends IGrain for your application.

## Setting up grainflow for your project

An example of how to implement grainflowLib as a Max external can be found in the [Grainflow project](https://github.com/composingcap/grainflow/tree/master/source/projects/grainflow_tilde).

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

Here is an example of these virtual functions being [implemented in Max](https://github.com/composingcap/grainflow/blob/master/source/projects/grainflow_tilde/MspGrain.h)

These functions are used internally within IGrain's Process method.

### Processing Code
#### Overall the structure of the code with be something like: 

1. Initialize Fields:
```
gfIoConfig ioConfig;
int maxGrains = 8; //This contains the size of each output array.
int inputChannels[4]= {8,8,8,8}; // This array contains the number of channels within each input array. The order is grainClock, traversalPhasor, fm, am. 

MyGrain<16> *grains;

//Inputs and outputs are generally provided by the audio application the plugin is written in.
double **inputs;
double **outputs;
```
2. Setup outputs (uniform for each grain)
```
void SetupOutputs(gfIoConfig& ioConfig, double** outputs, int maxGrainsThisFrame) {

	// Outputs are constant because they are based on the max grain count
	ioConfig.grainOutput = &outputs[0 * maxGrainsThisFrame];
	ioConfig.grainState = &outputs[1 * maxGrainsThisFrame];
	ioConfig.grainProgress = &outputs[2 * maxGrainsThisFrame];
	ioConfig.grainPlayhead = &outputs[3 * maxGrainsThisFrame];
	ioConfig.grainAmp = &outputs[4 * maxGrainsThisFrame];
	ioConfig.grainEnvelope = &outputs[5 * maxGrainsThisFrame];
	ioConfig.grainStreamChannel = &outputs[6 * maxGrainsThisFrame];
	ioConfig.grainBufferChannel = &outputs[7 * maxGrainsThisFrame];
}
```
3. Setup inputs for each grain
```
void SetupInputs(int grainIndex, gfIoConfig& ioConfig, int* inputChannels, double** inputs) {

	// These vars indicate the starting indices of each mc parameter
	auto grainClockCh = 0;
	auto traversalPhasorCh = inputChannels[0];
	auto fmCh = traversalPhasorCh + inputChannels[1];
	auto amCh = fmCh + inputChannels[2];

	ioConfig.grainClock = inputs[grainClockCh + (grainIndex % inputChannels[0])];
	ioConfig.traversalPhasor = inputs[traversalPhasorCh + (grainIndex % inputChannels[1])];
	ioConfig.fm = inputs[fmCh + (grainIndex % inputChannels[2])];
	ioConfig.am = inputs[amCh + (grainIndex % inputChannels[3])];
}
```
4. Process the audio block
```
void ProcessBlock(double **inputs, double **outputs){
    output array
    SetupOutputs(ioConfig, outputs, 8);
    for (int g = 0; g < grains.length; g++){
        SetupInputs(g, ioConfig, inputPositions, inputs);
        grain.Process(ioConfig);
    }
}

ProcessBlock(inputs, outputs);
```
Here is an example of the [implementation in Max](https://github.com/composingcap/grainflow/blob/master/source/projects/grainflow_tilde/grainflow_tilde.cpp)

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