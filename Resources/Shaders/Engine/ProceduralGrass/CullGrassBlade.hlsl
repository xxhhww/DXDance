#ifndef _CullGrassBlade__
#define _CullGrassBlade__

#include "ProceduralGrassHelper.hlsl"
#include "../GPUDrivenTerrain/TerrainHelper.hlsl"

struct PassData {
	uint visibleGrassClusterListIndex;			// 可见的GrassCluster列表索引
	uint bakedGrassBladeListIndex;				// 烘焙的GrassBlade列表
	uint visibleLOD0GrassBladeIndexListIndex;	// 可见的GrassBlade索引
	uint visibleLOD1GrassBladeIndexListIndex;	// LOD1

	float grassResolution;
	float distanceCullStartDist;
	float distanceCullEndDist;
	float distanceCullMinimumGrassAmount;
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

bool FrustumCull(float4 plane[6], BoundingBox boundingBox) {
	return
	IsAABBOutSidePlane(plane[0], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[1], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[2], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[3], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[4], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[5], boundingBox.minPosition, boundingBox.maxPosition);
}

bool DistanceCull(GrassBlade blade, float3 cameraPos) {
	//Distance culling
	float d = distance(blade.position, cameraPos);

	float distanceSmoothStep = 1.0f - smoothstep(PassDataCB.distanceCullStartDist, PassDataCB.distanceCullEndDist, d);

	distanceSmoothStep = (distanceSmoothStep * (1.0f - PassDataCB.distanceCullMinimumGrassAmount)) + PassDataCB.distanceCullMinimumGrassAmount;

	return (blade.hash > 1.0f - distanceSmoothStep) ? false : true; // 1.0f : 0.0f
}

[numthreads(1, 8, 8)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
	StructuredBuffer<GrassCluster> visibleGrassClusterList = ResourceDescriptorHeap[PassDataCB.visibleGrassClusterListIndex];
	StructuredBuffer<GrassBlade>   bakedGrassBladeList = ResourceDescriptorHeap[PassDataCB.bakedGrassBladeListIndex];
	AppendStructuredBuffer<uint>   visibleLOD0GrassBladeIndexList = ResourceDescriptorHeap[PassDataCB.visibleLOD0GrassBladeIndexListIndex];
	AppendStructuredBuffer<uint>   visibleLOD1GrassBladeIndexList = ResourceDescriptorHeap[PassDataCB.visibleLOD1GrassBladeIndexListIndex];

	// 当前线程组处理的GrassCluster
	GrassCluster grassCluster = visibleGrassClusterList[dispatchThreadID.x];

	uint readIndex = grassCluster.grassBladeBufferIndex + (dispatchThreadID.y * PassDataCB.grassResolution + dispatchThreadID.z);
	GrassBlade blade = bakedGrassBladeList[readIndex];

	// 无效的GrassBlade
	if(IsEmptyGrassBlade(blade)) return;

	// FrustumCull
	BoundingBox boundingBox;
	boundingBox.minPosition = float4(blade.position, 0.0f) - float4(1.0f, 0.0f, 1.0f, 0.0f);
	boundingBox.maxPosition = float4(blade.position, 0.0f) + float4(1.0f, 2.0f, 1.0f, 0.0f);
	
	if(FrustumCull(FrameDataCB.CurrentRenderCamera.Planes, boundingBox)) {
		return;
	}

	// DistanceCull
	if(DistanceCull(blade, FrameDataCB.CurrentRenderCamera.Position.xyz)) {
		return;
	}

	// 划分LOD
	float bladeDistance = distance(blade.position, FrameDataCB.CurrentRenderCamera.Position.xyz);

	// visibleLOD0GrassBladeIndexList.Append(readIndex);
	if(bladeDistance <= PassDataCB.distanceCullStartDist) {
		visibleLOD0GrassBladeIndexList.Append(readIndex);
	}
	else {
		visibleLOD1GrassBladeIndexList.Append(readIndex);
	}
}

#endif