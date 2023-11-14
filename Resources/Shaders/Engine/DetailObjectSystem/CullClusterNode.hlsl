#ifndef _CullClusterNode__
#define _CullClusterNode__

#include "StaticInstanceHelper.hlsl"
#include "../GPUDrivenTerrain/TerrainHelper.hlsl"

struct PassData {
	uint  clusterNodeBufferIndex;				// 所有的Cluster
	uint  visibleClusterNodeIndexBufferIndex;	// 可见Cluster索引
	uint  clusterNodeBufferSize;
	float instanceVisibleDistance;
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
* 计算包围盒上离testPoint最近的点
*/
float4 GetClosestPoint(BoundingBox boundingBox, float4 testPoint) {
	float4 result = float4(testPoint.x, testPoint.y, testPoint.z, 0.0f);
	float4 min = boundingBox.minPosition;
	float4 max = boundingBox.maxPosition;

	result.x = (result.x < min.x) ? min.x : result.x;
	result.y = (result.y < min.x) ? min.y : result.y;
	result.z = (result.z < min.x) ? min.z : result.z;

	result.x = (result.x > max.x) ? max.x : result.x;
	result.y = (result.y > max.x) ? max.y : result.y;
	result.z = (result.z > max.x) ? max.z : result.z;

	return result;
}

[numthreads(8, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
	StructuredBuffer<ClusterNode> clusterNodeBuffer = ResourceDescriptorHeap[PassDataCB.clusterNodeBufferIndex];
	AppendStructuredBuffer<uint>  visibleClusterNodeIndexBuffer = ResourceDescriptorHeap[PassDataCB.visibleClusterNodeIndexBufferIndex];

	uint threadIndex = dispatchThreadID.x;
	if(threadIndex < PassDataCB.clusterNodeBufferSize) {
		ClusterNode clusterNode = clusterNodeBuffer[threadIndex];

		// 摄像机剔除
		BoundingBox boundingBox;
		boundingBox.minPosition = clusterNode.minBoundingBoxPosition;
		boundingBox.maxPosition = clusterNode.maxBoundingBoxPosition;

		if(FrustumCull(FrameDataCB.CurrentRenderCamera.Planes, boundingBox)) {
			return;
		}

		// 计算包围盒与摄像机的最短距离
		float4 closestPoint = GetClosestPoint(boundingBox, FrameDataCB.CurrentRenderCamera.Position);
		float  closestDistance = distance(closestPoint.xyz, FrameDataCB.CurrentRenderCamera.Position.xyz);
		if(closestDistance > PassDataCB.instanceVisibleDistance) {
			return;
		}

		visibleClusterNodeIndexBuffer.Append(threadIndex);
	}
}

#endif