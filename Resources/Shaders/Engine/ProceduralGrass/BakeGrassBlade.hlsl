#ifndef _BakeGrassBlade__
#define _BakeGrassBlade__

#include "ProceduralGrassHelper.hlsl"

struct PassData {
    float2 terrainWorldMeterSize;
	uint   terrainHeightMapIndex;
	float  heightScale;

	uint   clumpMapIndex;
	float  clumpMapScale;
	uint   clumpParameterBufferIndex;
	uint   clumpParameterNums;

	uint   needBakedGrassClusterListIndex;
	uint   grassLayerMaskIndex;
    uint   grassBladeBakedBufferIndex;
	uint   grassResolution;

    float  jitterStrength;
    float  pad1;
    float  pad2;
	float  pad3;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

[numthreads(1, 8, 8)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
	
}

#endif