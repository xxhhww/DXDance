#ifndef _DeferredLightPass__
#define _DeferredLightPass__

struct PassData {
	uint _GBufferAlbedoIndex;
	uint _GBufferPositionIndex;
	uint _GBufferNormalIndex;
	uint _GBufferMREIndex;
	uint _FinalOutputIndex;
	float pad1;
	float pad2;
	float pad3;
};

#define PassDataType PassData

#include "Base/MainEntryPoint.hlsl"

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
	return;
}

#endif