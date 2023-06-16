#ifndef _BaseEngineLayout__
#define _BaseEngineLayout__

// The maximum size of a root signature is 64 DWORDs :
// Descriptor tables cost 1 DWORD each.
// Root constants cost 1 DWORD each, since they are 32 - bit values.
// Root descriptors(64 - bit GPU virtual addresses) cost 2 DWORDs each.
// Static samplers do not have any cost in the size of the root signature.

struct Dummy {
    int D;
};

#ifndef GlobalDataType
#define GlobalDataType Dummy
#endif

#ifndef FrameDataType
#define FrameDataType Dummy
#endif

#ifndef PassDataType
#define PassDataType Dummy
#endif

#ifndef LightDataType
#define LightDataType Dummy
#endif

ConstantBuffer<FrameDataType>   FrameDataCB : register(b0, space10);
ConstantBuffer<PassDataType>    PassDataCB  : register(b1, space10);
StructuredBuffer<LightDataType> LightDataSB : register(t0, space10);

SamplerState SamplerPointWrap : register(s0);
SamplerState SamplerPointClamp : register(s1);
SamplerState SamplerLinearWrap : register(s2);
SamplerState SamplerLinearClamp : register(s3);
SamplerState SamplerAnisotropicWrap : register(s4);
SamplerState SamplerAnisotropicClamp : register(s5);


#endif