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
	terrainNodeDescriptorBuffer[request.dstTerrainNodeIndex].tilePosX = request.tilePosX;
	terrainNodeDescriptorBuffer[request.dstTerrainNodeIndex].tilePosY = request.tilePosY;

	// ��srcTerrainNodeIndex��Ӧ��tilePos��Ϊ��Ч
	if(request.srcTerrainNodeIndex != 65536) {
		terrainNodeDescriptorBuffer[request.srcTerrainNodeIndex].tilePosX = 255;
		terrainNodeDescriptorBuffer[request.srcTerrainNodeIndex].tilePosY = 255;
	}
}

#endif