#ifndef _CullStaticInstance__
#define _CullStaticInstance__

#include "StaticInstanceHelper.hlsl"
#include "../GPUDrivenTerrain/TerrainHelper.hlsl"

struct PassData {
	uint clusterNodeBufferIndex;				// ���е�ClusterNode
	uint transformedBoundingBoxBufferIndex;		// ÿһ��Instance��Ӧ��TransformedBoundingBox
	uint sortedInstancesBufferIndex;			// �μ�����
	uint visibleClusterNodeIndexBufferIndex;	// �ɼ�Cluster����

	uint visibleLod0InstanceIndexBufferIndex;	// �ɼ�Instanceֱ������(Lod0)
	uint visibleLod1InstanceIndexBufferIndex;	// �ɼ�Instanceֱ������(Lod1)
	uint visibleLod2InstanceIndexBufferIndex;	// �ɼ�Instanceֱ������(Lod2)
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

//�����Ƿ���ƽ������
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
* ʹ�����������׶����вü�
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
*	Z: ������
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

	// ����SortedInstancesIndex�����ֱ������
	uint sortedInstancesIndex = clusterNode.firstInstanceIndex + dispatchThreadID.y;
	if(sortedInstancesIndex <= clusterNode.lastChildIndex) {
		uint directIndex = sortedInstancesBuffer[sortedInstancesIndex];
		if(directIndex < PassDataCB.totalInstanceCount) {
			BoundingBox boundingBox = transformedBoundingBoxBuffer[directIndex];

			// ������޳�
			if(FrustumCull(FrameDataCB.CurrentRenderCamera.Planes, boundingBox)) {
				return;
			}

			// �����޳�
			float3 centerPosition = (boundingBox.minPosition.xyz + boundingBox.maxPosition.xyz) / 2.0f;
			float closestDistance = distance(FrameDataCB.CurrentRenderCamera.Position.xyz, centerPosition);
			if(closestDistance > PassDataCB.instanceVisibleInstance) {
				return;
			}
			
			// ���ݾ����Lod
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