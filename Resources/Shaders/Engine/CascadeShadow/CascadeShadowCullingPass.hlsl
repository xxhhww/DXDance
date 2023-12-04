#ifndef _CascadeShadowCullingPass__
#define _CascadeShadowCullingPass__

struct PassData {
	uint   opaqueItemDataBufferIndex;
	uint   opaqueItemIndirectDrawIndexedDataBufferIndex;
	uint   opaqueItemNumsPerFrame;
	float  pad1;
	
	uint   cascadeShadow0CulledIndirectArgsIndex;	// 级联层级0的可视Item的间接绘制参数
	uint   cascadeShadow1CulledIndirectArgsIndex;
	uint   cascadeShadow2CulledIndirectArgsIndex;
	uint   cascadeShadow3CulledIndirectArgsIndex;

	float4 cascadeShadow0FrustumPlanes[6];			// 级联层级0的光源可视平截头体
	float4 cascadeShadow1FrustumPlanes[6];
	float4 cascadeShadow2FrustumPlanes[6];
	float4 cascadeShadow3FrustumPlanes[6];

	uint2  cascadeShadow0TargetPassDataAddress;
	uint2  cascadeShadow1TargetPassDataAddress;
	uint2  cascadeShadow2TargetPassDataAddress;
	uint2  cascadeShadow3TargetPassDataAddress;
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
	StructuredBuffer<ItemData> opaqueItemDataBuffer = ResourceDescriptorHeap[PassDataCB.opaqueItemDataBufferIndex];
	StructuredBuffer<ItemIndirectDrawIndexedData> opaqueItemIndirectDrawIndexedDataBuffer = ResourceDescriptorHeap[PassDataCB.opaqueItemIndirectDrawIndexedDataBufferIndex];
	AppendStructuredBuffer<ItemIndirectDrawIndexedData> cascadeShadow0IndirectArgs = ResourceDescriptorHeap[PassDataCB.cascadeShadow0CulledIndirectArgsIndex];
	AppendStructuredBuffer<ItemIndirectDrawIndexedData> cascadeShadow1IndirectArgs = ResourceDescriptorHeap[PassDataCB.cascadeShadow1CulledIndirectArgsIndex];
	AppendStructuredBuffer<ItemIndirectDrawIndexedData> cascadeShadow2IndirectArgs = ResourceDescriptorHeap[PassDataCB.cascadeShadow2CulledIndirectArgsIndex];
	AppendStructuredBuffer<ItemIndirectDrawIndexedData> cascadeShadow3IndirectArgs = ResourceDescriptorHeap[PassDataCB.cascadeShadow3CulledIndirectArgsIndex];

	uint threadIndex = dispatchThreadID.x;
	if(threadIndex < PassDataCB.opaqueItemNumsPerFrame) {
		ItemData currItemData = opaqueItemDataBuffer[threadIndex];
		ItemIndirectDrawIndexedData currIndirectDrawData = opaqueItemIndirectDrawIndexedDataBuffer[threadIndex];

		// FrustumCull
		BoundingBox boundingBox;
		boundingBox.minPosition = currItemData.minBoundingBoxPosition;
		boundingBox.maxPosition = currItemData.maxBoundingBoxPosition;

		/*
		currIndirectDrawData.passDataAddress = PassDataCB.cascadeShadow0TargetPassDataAddress;
		cascadeShadow0IndirectArgs.Append(currIndirectDrawData);

		currIndirectDrawData.passDataAddress = PassDataCB.cascadeShadow1TargetPassDataAddress;
		cascadeShadow1IndirectArgs.Append(currIndirectDrawData);

		currIndirectDrawData.passDataAddress = PassDataCB.cascadeShadow2TargetPassDataAddress;
		cascadeShadow2IndirectArgs.Append(currIndirectDrawData);

		currIndirectDrawData.passDataAddress = PassDataCB.cascadeShadow3TargetPassDataAddress;
		cascadeShadow3IndirectArgs.Append(currIndirectDrawData);
		*/
		
		// Cascade 0
		if(!FrustumCull(PassDataCB.cascadeShadow0FrustumPlanes, boundingBox)) {
			currIndirectDrawData.passDataAddress = PassDataCB.cascadeShadow0TargetPassDataAddress;
			cascadeShadow0IndirectArgs.Append(currIndirectDrawData);
		}
		// Cascade 1
		if(!FrustumCull(PassDataCB.cascadeShadow1FrustumPlanes, boundingBox)) {
			currIndirectDrawData.passDataAddress = PassDataCB.cascadeShadow1TargetPassDataAddress;
			cascadeShadow1IndirectArgs.Append(currIndirectDrawData);
		}
		// Cascade 2
		if(!FrustumCull(PassDataCB.cascadeShadow2FrustumPlanes, boundingBox)) {
			currIndirectDrawData.passDataAddress = PassDataCB.cascadeShadow2TargetPassDataAddress;
			cascadeShadow2IndirectArgs.Append(currIndirectDrawData);
		}
		// Cascade 3
		if(!FrustumCull(PassDataCB.cascadeShadow3FrustumPlanes, boundingBox)) {
			currIndirectDrawData.passDataAddress = PassDataCB.cascadeShadow3TargetPassDataAddress;
			cascadeShadow3IndirectArgs.Append(currIndirectDrawData);
		}
	}
}

#endif