#ifndef _GrassBladeBaker__
#define _GrassBladeBaker__

struct PassData {
	float2 terrainWorldMeterSize;
	float  heightScale;
	uint   terrainHeightMapAtlasIndex;

	uint   terrainNodeDescriptorListIndex;
	uint   terrainLodDescriptorListIndex;
	uint   grasslandNodeRequestTaskListIndex;
	uint   grasslandMapIndex;

	uint   grasslandNodeDescriptorListIndex;
	uint   grasslandLinearBufferIndex;
	uint   grassResolution;
	float  jitterStrength;

	uint   clumpMapIndex;
	float  clumpMapScale;
	uint   clumpParameterBufferIndex;
	uint   clumpParameterNums;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

[numthreads(1, 8, 8)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
	
}

#endif