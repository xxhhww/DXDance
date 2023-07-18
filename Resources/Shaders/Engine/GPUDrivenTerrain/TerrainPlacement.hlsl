#ifndef _TerrainPlacement__
#define _TerrainPlacement__

#include "TerrainPlacement.hlsl"

struct PassData {
	float2 worldSize;
	uint   heightScale;
	float  heightBias;

	uint   placedObjectCount;		// 分布个数
	uint   placementBufferIndex;	// CPU计算的分布点，需要对Y值进行修改
	uint   heightMapIndex;
	uint   normalMapIndex;
}

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

[numthreads(512, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
	RWStructuredBuffer<Placement> placementBuffer = ResourceDescriptorHeap[PassDataCB.placementBufferIndex];
	Texture2D<float4>             heightMap       = ResourceDescriptorHeap[PassDataCB.heightMapIndex];
	Texture2D<float4>             normalMap       = ResourceDescriptorHeap[PassDataCB.normalMapIndex];

	Placement placement = placementBuffer[dispatchThreadID.x];

	
}

#endif