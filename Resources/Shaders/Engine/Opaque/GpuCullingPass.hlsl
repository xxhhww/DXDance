#ifndef _GpuCullingPass__
#define _GpuCullingPass__

struct PassData {
	uint opaqueItemIndirectArgsIndex;
	uint opaqueItemDataArrayIndex;
	float pad1;
	float pad2;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

[numthreads(1, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
}

#endif