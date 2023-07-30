#ifndef _FoliagePlacer__
#define _FoliagePlacer__

#include "FoliageHelper.hlsl"

struct PassData {
	uint   placementBufferIndex;
    uint   heightMapIndex;
	uint   normalMapIndex;
	uint   rotateToCamera;        // Foliage在渲染时，是否强制朝向摄像机
	float2 worldMeterSize;
    uint   heightScale;
    uint   placementSizePerAxis;
	uint   useFrustumCull;
	float  pad1;
	float  pad2;
	float  pad3;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

float2 GetHeightUV(float2 wsPositionXZ) {
	float2 heightUV = (wsPositionXZ + (PassDataCB.worldMeterSize * 0.5f) + 0.5f) / (PassDataCB.worldMeterSize + 1.0f);
	heightUV *= 1.0f;

	return heightUV;
}

float GetHeightFromHeightMap(float2 wsPositionXZ) {
	Texture2D<float4> heightMap = ResourceDescriptorHeap[PassDataCB.heightMapIndex];

	float2 heightUV = GetHeightUV(wsPositionXZ);

	float  height = heightMap.SampleLevel(SamplerLinearWrap, heightUV, 0u).r;

	return height;
}

float3 GetNormalFromNormalMap(float2 wsPositionXZ) {
	Texture2D<float4> normalMap = ResourceDescriptorHeap[PassDataCB.normalMapIndex];

	float2 heightUV = GetHeightUV(wsPositionXZ);

	float3 normal = float3(0.0f, 0.0f, 0.0f);
    normal.xz = normalMap.SampleLevel(SamplerLinearWrap, heightUV, 0u).xy * 2.0f - 1.0f;
    normal.y = sqrt(max(0u, 1u - dot(normal.xz,normal.xz)));

	return normal;
}

uint murmurHash12(uint2 src) {
    const uint M = 0x5bd1e995u;
    uint h = 1190494759u;
    src *= M; src ^= src>>24u; src *= M;
    h *= M; h ^= src.x; h *= M; h ^= src.y;
    h ^= h>>13u; h *= M; h ^= h>>15u;
    return h;
}

float rand(float2 co){
    return frac(sin(dot(co, float2(12.9898, 78.233))) * 43758.5453);
}

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

bool Cull(BoundingBox boundingBox) {
	if(PassDataCB.useFrustumCull){
		if(FrustumCull(FrameDataCB.CurrentRenderCamera.Planes, boundingBox)) {
			return true;
		}
	}
    return false;
}

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
	AppendStructuredBuffer<Placement> placementBuffer = ResourceDescriptorHeap[PassDataCB.placementBufferIndex];

	// 计算安置点的XZ位置
	uint2  placementIndex = dispatchThreadID.xy;
	uint   offsetX = placementIndex.x;	// 从0开始计数
	uint   offsetY = placementIndex.y;	// 从0开始计数

	float  distanceBetweenPlacements = PassDataCB.worldMeterSize.x / PassDataCB.placementSizePerAxis;

	float  absPositionX = distanceBetweenPlacements * offsetX;
	float  absPositionZ = distanceBetweenPlacements * offsetY;
	
	float  truePositionX = absPositionX - (PassDataCB.worldMeterSize.x / 2.0f);
	float  truePositionZ = absPositionZ - (PassDataCB.worldMeterSize.x / 2.0f);

	float jitterX = rand(dispatchThreadID.xy);
    float jitterZ = rand(dispatchThreadID.xz);

	float3 position = float3(truePositionX + jitterX, 0.0f, truePositionZ + jitterZ);

	float  height = GetHeightFromHeightMap(position.xz);
	float3 normal = GetNormalFromNormalMap(position.xz);

	float  finalHeight = height * PassDataCB.heightScale;

	// 生成Placement
	Placement placement;
	placement.position = float4(position.x, finalHeight, position.z, 0.0f);
	placement.normal = float4(normal, 0.0f);
	placement.albedoMapIndex = 0u;
	placement.normalMapIndex = 0u;
	placement.pad1 = 0.0f;
	placement.pad2 = 0.0f;

	BoundingBox boundingBox;
	boundingBox.minPosition = placement.position - float4(1.0f, 0.0f, 1.0f, 0.0f);
	boundingBox.maxPosition = placement.position + float4(1.0f, 2.0f, 1.0f, 0.0f);

	if(normal.y > 0.9f && !Cull(boundingBox)) {
		placementBuffer.Append(placement);
	}
}

#endif