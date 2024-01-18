#ifndef _TerrainNodeDescriptorUpdater__
#define _TerrainNodeDescriptorUpdater__

#include "TerrainHeader.hlsl"

struct PassData {
	uint terrainNodeDescriptorBufferIndex;
	uint updateTerrainNodeDescriptorRequestBufferIndex;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

[numthreads(8, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
	RWStructuredBuffer<TerrainNodeDescriptor> terrainNodeDescriptorBuffer = ResourceDescriptorHeap[PassDataCB.terrainNodeDescriptorBufferIndex];
	StructuredBuffer<GpuUpdateTerrainNodeDescriptorRequest> updateTerrainNodeDescriptorRequestBuffer = ResourceDescriptorHeap[PassDataCB.updateTerrainNodeDescriptorRequestBufferIndex];

	uint threadIndex = dispatchThreadID.x;

	GpuUpdateTerrainNodeDescriptorRequest request = updateTerrainNodeDescriptorRequestBuffer[threadIndex];
	terrainNodeDescriptorBuffer[request.terrainNodeIndex] = request.terrainNodeDescriptor;
}

#endif