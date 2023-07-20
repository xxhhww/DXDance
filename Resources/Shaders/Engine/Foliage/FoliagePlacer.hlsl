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
    float  pad1;
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

	return height * PassDataCB.heightScale;
}

float3 GetNormalFromNormalMap(float2 wsPositionXZ) {
	Texture2D<float4> normalMap = ResourceDescriptorHeap[PassDataCB.normalMapIndex];

	float2 heightUV = GetHeightUV(wsPositionXZ);

	float3 normal = float3(0.0f, 0.0f, 0.0f);
    normal.xz = normalMap.SampleLevel(SamplerLinearWrap, heightUV, 0u).xy * 2.0f - 1.0f;
    normal.y = sqrt(max(0u, 1u - dot(normal.xz,normal.xz)));

	return normal;
}

[numthreads(1, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
	RWStructuredBuffer<Placement> placementBuffer = ResourceDescriptorHeap[PassDataCB.placementBufferIndex];

	Placement placement = placementBuffer[dispatchThreadID.x];

	float  height = GetHeightFromHeightMap(placement.position.xz);
	float3 normal = GetNormalFromNormalMap(placement.position.xz);

	placement.position.y = height;
	placement.normal.xyz = normal;
}

#endif