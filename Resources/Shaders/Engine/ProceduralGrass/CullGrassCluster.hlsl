#ifndef _CullGrassCluster__
#define _CullGrassCluster__

#include "ProceduralGrassHelper.hlsl"
#include "../GPUDrivenTerrain/TerrainHelper.hlsl"

struct PassData {
	uint  needCulledGrassClusterListIndex;			// ��Ҫִ���޳��Ĳ�Ⱥ�б�
	uint  visibleGrassClusterListIndex;				// �ɼ���GrassCluster�б�����
	uint  minmaxHeightMapIndex;
	uint  minmaxHeightMapWorldResolutionPerPixel;	// MinMapHeightMap�е��������������ĵ��δ�С(һ��Ϊ8m * 8m)
				
	float2 terrainWorldMeterSize;					// You Konw
	float  terrainHeightScale;
	float  pad2;
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

[numthreads(8, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
	StructuredBuffer<GrassCluster>       needCulledGrassCluster = ResourceDescriptorHeap[PassDataCB.needCulledGrassClusterListIndex];
	Texture2D<float4>                    minmaxHeightMap = ResourceDescriptorHeap[PassDataCB.minmaxHeightMapIndex];
	AppendStructuredBuffer<GrassCluster> visibleGrassClusterList = ResourceDescriptorHeap[PassDataCB.visibleGrassClusterListIndex];
	
	// ��ǰ�߳��鴦���GrassCluster
	GrassCluster grassCluster = needCulledGrassCluster[dispatchThreadID.x];

	// �������Ͻǵ��������������Ͻǵ����������
	uint2 lbGrassClusterLoc = grassCluster.grassClusterRect.xy;
	uint2 ltGrassClusterLoc = uint2(lbGrassClusterLoc.x, lbGrassClusterLoc.y + grassCluster.w);
	uint2 rtGrassClusterLoc = uint2(lbGrassClusterLoc.x + grassCluster.z, lbGrassClusterLoc.y + grassCluster.w);

	// ����������ת��ΪminmaxHeightMap�е�����
	uint2 minmaxHeightMapLoc = uint2(
		(ltGrassClusterLoc.y + PassDataCB.terrainWorldMeterSize.y / 2.0f) / grassCluster.w, 
		(ltGrassClusterLoc.x + PassDataCB.terrainWorldMeterSize.x / 2.0f) / grassCluster.z
	);
	minmaxHeightMapLoc.x = (PassDataCB.terrainWorldMeterSize.y / grassCluster.w) - (minmaxHeightMapLoc.x + 1);

	uint grassClusterMeterSize = grassCluster.grassClusterRect.z;
	uint mipmapIndex = log2(grassClusterMeterSize / minmaxHeightMapWorldResolutionPerPixel);
	float2 minmaxHeight = minMaxHeightMap.mips[mipmapIndex][minmaxHeightMapLoc].xy * PassDataCB.heightScale + float2(-5.0f, 5.0f);;

	BoundingBox boundingBox;
    float4 minPosition = float4(0.0f, 0.0f, 0.0f, 0.0f); 
	float4 maxPosition = float4(0.0f, 0.0f, 0.0f, 0.0f);

	minPosition.xz = float2(lbGrassClusterLoc.x, lbGrassClusterLoc.y);
    maxPosition.xz = float2(rtGrassClusterLoc.x, rtGrassClusterLoc.y);
    minPosition.y = minMaxHeight.x;
    maxPosition.y = minMaxHeight.y;

    boundingBox.minPosition = minPosition;
    boundingBox.maxPosition = maxPosition;

	if(FrustumCull(FrameDataCB.CurrentRenderCamera.Planes, boundingBox)) {
		return;
	}

	visibleGrassClusterList.Append(grassCluster);
}

#endif