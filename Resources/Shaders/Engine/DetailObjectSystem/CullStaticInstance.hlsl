#ifndef _CullStaticInstance__
#define _CullStaticInstance__

#include "StaticInstanceHelper.hlsl"
#include "../GPUDrivenTerrain/TerrainHelper.hlsl"

struct PassData {
	uint clusterNodeBufferIndex;				// 所有的ClusterNode
	uint transformedBoundingBoxBufferIndex;		// 每一个Instance对应的TransformedBoundingBox
	uint sortedInstancesBufferIndex;			// 次级索引
	uint visibleClusterNodeIndexBufferIndex;	// 可见Cluster索引

	uint visibleLod0InstanceIndexBufferIndex;	// 可见Instance直接索引(Lod0)
	uint visibleLod1InstanceIndexBufferIndex;	// 可见Instance直接索引(Lod1)
	uint visibleLod2InstanceIndexBufferIndex;	// 可见Instance直接索引(Lod2)
	uint instanceCountPerCluster;

	float lod0InstanceVisibleDistance;
	float lod1InstanceVisibleDistance;
	float lod2InstanceVisibleDistance;
	float instanceVisibleDistance;

	uint  totalInstanceCount;
	float pad1;
	float pad2;
	float pad3;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

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

/*
* 使用摄像机的视锥体进行裁剪
*/
bool FrustumCull(float4 plane[6], BoundingBox boundingBox) {
	return
	IsAABBOutSidePlane(plane[0], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[1], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[2], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[3], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[4], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[5], boundingBox.minPosition, boundingBox.maxPosition);
}

/*
*	X: Index For VisibleClusterNodeIndexBuffer
*	Y: Index For InstanceCountPerCluster
*	Z: 无意义
*/
[numthreads(1, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
	StructuredBuffer<ClusterNode> clusterNodeBuffer = ResourceDescriptorHeap[PassDataCB.clusterNodeBufferIndex];
	StructuredBuffer<BoundingBox> transformedBoundingBoxBuffer = ResourceDescriptorHeap[PassDataCB.transformedBoundingBoxBufferIndex];
	StructuredBuffer<uint> sortedInstancesBuffer = ResourceDescriptorHeap[PassDataCB.sortedInstancesBufferIndex];
	StructuredBuffer<uint> visibleClusterNodeIndexBuffer = ResourceDescriptorHeap[PassDataCB.visibleClusterNodeIndexBufferIndex];
	AppendStructuredBuffer<uint> visibleLod0InstanceIndexBuffer = ResourceDescriptorHeap[PassDataCB.visibleLod0InstanceIndexBufferIndex];
	AppendStructuredBuffer<uint> visibleLod1InstanceIndexBuffer = ResourceDescriptorHeap[PassDataCB.visibleLod1InstanceIndexBufferIndex];
	AppendStructuredBuffer<uint> visibleLod2InstanceIndexBuffer = ResourceDescriptorHeap[PassDataCB.visibleLod2InstanceIndexBufferIndex];

	uint visibleClusterNodeIndex = visibleClusterNodeIndexBuffer[dispatchThreadID.x];
	ClusterNode clusterNode = clusterNodeBuffer[visibleClusterNodeIndex];

	// 计算SortedInstancesIndex，获得直接索引
	uint sortedInstancesIndex = clusterNode.firstInstanceIndex + dispatchThreadID.y;
	if(sortedInstancesIndex <= clusterNode.lastChildIndex) {
		uint directIndex = sortedInstancesBuffer[sortedInstancesIndex];
		if(directIndex < PassDataCB.totalInstanceCount) {
			BoundingBox boundingBox = transformedBoundingBoxBuffer[directIndex];

			// 摄像机剔除
			if(FrustumCull(FrameDataCB.CurrentRenderCamera.Planes, boundingBox)) {
				return;
			}

			// 距离剔除
			float3 centerPosition = (boundingBox.minPosition.xyz + boundingBox.maxPosition.xyz) / 2.0f;
			float closestDistance = distance(FrameDataCB.CurrentRenderCamera.Position.xyz, centerPosition);
			if(closestDistance > PassDataCB.instanceVisibleInstance) {
				return;
			}
			
			// 根据距离分Lod
			if(closestDistance <= PassDataCB.lod0InstanceVisibleDistance) {
				visibleLod0InstanceIndexBuffer.Append(directIndex);
				return;
			}

			if(closestDistance <= PassDataCB.lod1InstanceVisibleDistance) {
				visibleLod1InstanceIndexBuffer.Append(directIndex);
				return;
			}

			if(closestDistance <= PassDataCB.lod2InstanceVisibleDistance) {
				visibleLod2InstanceIndexBuffer.Append(directIndex);
				return;
			}

		}
	}
}

#endif