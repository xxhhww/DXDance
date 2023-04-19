#ifndef _BaseEngineLayout__
#define _BaseEngineLayout__

// The maximum size of a root signature is 64 DWORDs :
// Descriptor tables cost 1 DWORD each.
// Root constants cost 1 DWORD each, since they are 32 - bit values.
// Root descriptors(64 - bit GPU virtual addresses) cost 2 DWORDs each.
// Static samplers do not have any cost in the size of the root signature.

struct Dummy
{
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

ConstantBuffer<FrameDataType> FrameDataCB : register(b0, space10);



#endif