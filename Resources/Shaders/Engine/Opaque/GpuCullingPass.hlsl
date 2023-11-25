#ifndef _GpuCullingPass__
#define _GpuCullingPass__

struct PassData {
	uint deferredItemDataBufferIndex;
	uint deferredItemIndirectDrawIndexedDataBufferIndex;
	uint culledDeferredItemIndirectArgsIndex;
	uint itemNumsPerFrame;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../GPUDrivenTerrain/TerrainHelper.hlsl"

//测试是否在平面的外侧
bool IsPositionOutSidePlane(float4 plane, float3 position) {
    return dot(plane.xyz, position) + plane.w < 0; 
}

bool IsAABBOutSidePlane(float4 plane, float4 minPosition, float4 maxPosition) {
    return 
	IsPositionOutSidePlane(plane, minPosition.xyz) &&
    IsPositionOutSidePlane(plane, maxPosition.xyz) &&
    IsPositionOutSidePlane(plane, float3(minPosition.x, minPosition.y, maxPosition.z)) &&
    IsPositionOutSidePlane(plane, float3(minPosition.x, maxPosition.y, minPosition.z)) &&
    IsPositionOutSidePlane(plane, float3(minPosition.x, maxPosition.y, maxPosition.z)) &&
    IsPositionOutSidePlane(plane, float3(maxPosition.x, minPosition.y, maxPosition.z)) &&
    IsPositionOutSidePlane(plane, float3(maxPosition.x, maxPosition.y, minPosition.z)) &&
    IsPositionOutSidePlane(plane, float3(maxPosition.x, minPosition.y, minPosition.z));
}

bool FrustumCull(float4 plane[6], BoundingBox boundingBox) {
	return
	IsAABBOutSidePlane(plane[0], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[1], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[2], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[3], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[4], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[5], boundingBox.minPosition, boundingBox.maxPosition);
}

[numthreads(8, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
	StructuredBuffer<ItemData> deferredItemDataBuffer = ResourceDescriptorHeap[PassDataCB.deferredItemDataBufferIndex];
	StructuredBuffer<ItemIndirectDrawIndexedData> deferredItemIndirectDrawIndexedDataBuffer = ResourceDescriptorHeap[PassDataCB.deferredItemIndirectDrawIndexedDataBufferIndex];
	AppendStructuredBuffer<ItemIndirectDrawIndexedData> culledDeferredItemIndirectArgs = ResourceDescriptorHeap[PassDataCB.culledDeferredItemIndirectArgsIndex];

	uint threadIndex = dispatchThreadID.x;
	if(threadIndex < PassDataCB.itemNumsPerFrame) {
		ItemData currItemData = deferredItemDataBuffer[threadIndex];
		ItemIndirectDrawIndexedData currIndirectDrawData = deferredItemIndirectDrawIndexedDataBuffer[threadIndex];

		// FrustumCull
		BoundingBox boundingBox;
		boundingBox.minPosition = currItemData.minBoundingBoxPosition;
		boundingBox.maxPosition = currItemData.maxBoundingBoxPosition;
	
		if(FrustumCull(FrameDataCB.CurrentEditorCamera.Planes, boundingBox)) {
			return;
		}

		culledDeferredItemIndirectArgs.Append(currIndirectDrawData);
	}
}

#endif