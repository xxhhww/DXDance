#ifndef _ClumpMapGenerator
#define _ClumpMapGenerator

struct PassData{
	uint2 clumpMapSize;
	uint  numClumps;
	float pad1;
}

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
	
}

#endif