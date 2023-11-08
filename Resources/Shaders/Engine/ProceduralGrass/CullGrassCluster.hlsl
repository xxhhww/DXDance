#ifndef _CullGrassCluster__
#define _CullGrassCluster__

#include "ProceduralGrassHelper.hlsl"
#include "../GPUDrivenTerrain/TerrainHelper.hlsl"

struct PassData {
	uint  needCulledGrassClusterListIndex;			// 需要执行剔除的草群列表
	uint  visibleGrassClusterListIndex;				// 可见的GrassCluster列表索引
	uint  minmaxHeightMapIndex;
	uint  minmaxHeightMapWorldResolutionPerPixel;	// MinMapHeightMap中单个像素所描述的地形大小(一般为8m * 8m)
				
	float2 terrainWorldMeterSize;					// You Konw
	float  terrainHeightScale;
	float  pad2;
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

[numthreads(8, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
	StructuredBuffer<GrassCluster>       needCulledGrassCluster = ResourceDescriptorHeap[PassDataCB.needCulledGrassClusterListIndex];
	Texture2D<float4>                    minmaxHeightMap = ResourceDescriptorHeap[PassDataCB.minmaxHeightMapIndex];
	AppendStructuredBuffer<GrassCluster> visibleGrassClusterList = ResourceDescriptorHeap[PassDataCB.visibleGrassClusterListIndex];
	
	// 当前线程组处理的GrassCluster
	GrassCluster grassCluster = needCulledGrassCluster[dispatchThreadID.x];
	float4 grassClusterRect = grassCluster.grassClusterRect;

	// 计算左上角点的世界坐标和右上角点的世界坐标
	float2 lbGrassClusterLoc = grassClusterRect.xy;
	float2 ltGrassClusterLoc = float2(lbGrassClusterLoc.x, lbGrassClusterLoc.y + grassClusterRect.w);
	float2 rtGrassClusterLoc = float2(lbGrassClusterLoc.x + grassClusterRect.z, lbGrassClusterLoc.y + grassClusterRect.w);

	// 将世界坐标转换为minmaxHeightMap中的索引
	uint2 minmaxHeightMapLoc = uint2(
		(ltGrassClusterLoc.x + PassDataCB.terrainWorldMeterSize.x / 2.0f) / grassClusterRect.z,
		(PassDataCB.terrainWorldMeterSize.y - (ltGrassClusterLoc.y + PassDataCB.terrainWorldMeterSize.y / 2.0f)) / grassClusterRect.w
	);

	uint grassClusterMeterSize = grassClusterRect.z;
	uint mipmapIndex = log2(grassClusterMeterSize / PassDataCB.minmaxHeightMapWorldResolutionPerPixel);
	float2 minmaxHeight = minmaxHeightMap.mips[mipmapIndex][minmaxHeightMapLoc].xy * PassDataCB.terrainHeightScale;

	BoundingBox boundingBox;
    float4 minPosition = float4(lbGrassClusterLoc.x, minmaxHeight.x, lbGrassClusterLoc.y, 0.0f);
	float4 maxPosition = float4(rtGrassClusterLoc.x, minmaxHeight.y, rtGrassClusterLoc.y, 0.0f);

    boundingBox.minPosition = minPosition;
    boundingBox.maxPosition = maxPosition;

	if(FrustumCull(FrameDataCB.CurrentRenderCamera.Planes, boundingBox)) {
		return;
	}

	visibleGrassClusterList.Append(grassCluster);
}

#endif