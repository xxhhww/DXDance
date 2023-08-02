#ifndef _GrassBladeGenerator__
#define _GrassBladeGenerator__

#include "GrassHelper.hlsl"

struct PassData {
	uint grassBladesBufferIndex0;	// Grass LOD0
	uint grassBladesBufferIndex1;	// Grass LOD1
	uint terrainHeightMapIndex;
	uint terrainNormalMapIndex;

	float2 worldMeterSize;
	uint   heightScale;
	uint   grassBladeSizePerAxis;

	uint   useFrustumCull;
	uint   useDistanceCull;
	float  distanceCullStartDist;
	float  distanceCullEndDist;

	float  distanceCullMinimumGrassAmount;
	float  jitterStrength;
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

/*
施加高度偏移
*/
float3 ApplyHeightOffset(float3 position){
	float height = GetHeightFromHeightMap(position.xz);
	position.y += height;

	return position;
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

bool DistanceCull(float3 worldPos, float hash){
	if(PassDataCB.useDistanceCull){
		//Distance culling
		float d = distance(worldPos, FrameDataCB.CurrentRenderCamera.Position.xyz);

		float distanceSmoothStep = 1.0f - smoothstep(PassDataCB.distanceCullStartDist, PassDataCB.distanceCullEndDist, d);

		distanceSmoothStep = (distanceSmoothStep * (1.0f - PassDataCB.distanceCullMinimumGrassAmount)) + PassDataCB.distanceCullMinimumGrassAmount;

		return hash > 1.0f - distanceSmoothStep ? false : true; // 1.0f : 0.0f
	}
	return false;
}

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
	AppendStructuredBuffer<GrassBlade> grassBladeBuffer = ResourceDescriptorHeap[PassDataCB.grassBladesBufferIndex0];

	// Calculate XZPosition
	uint   offsetX = dispatchThreadID.x;	// 从0开始计数
	uint   offsetY = dispatchThreadID.y;	// 从0开始计数

	float  distanceBetweenPlacements = PassDataCB.worldMeterSize.x / PassDataCB.placementSizePerAxis;

	float  absPositionX = distanceBetweenPlacements * offsetX;
	float  absPositionZ = distanceBetweenPlacements * offsetY;
	
	float  truePositionX = absPositionX - (PassDataCB.worldMeterSize.x / 2.0f);
	float  truePositionZ = absPositionZ - (PassDataCB.worldMeterSize.x / 2.0f);

	float3 position = float3(truePositionX, 0.0f, truePositionZ);

	// Apply Jitter
	float2 hash = hashwithoutsine22(dispatchThreadID.xy);
	float2 jitter = ((hash * 2.0f) - 1.0f) * PassDataCB.jitterStrength;

	position.xz += jitter;

	// Apply Height
	float3 raisedPosition = ApplyHeightOffset(position);

	// DistanceCull
	if(DistanceCull(raisedPosition, hash.x)) {
		return;
	}

	// FrustumCull
	BoundingBox boundingBox;
	boundingBox.minPosition = float4(raisedPosition, 0.0f) - float4(1.0f, 0.0f, 1.0f, 0.0f);
	boundingBox.maxPosition = float4(raisedPosition, 0.0f) + float4(1.0f, 2.0f, 1.0f, 0.0f);
	if(Cull(boundingBox)) {
		return;
	}

	// Normal Check
	float3 terrainNormal = GetNormalFromNormalMap(position.xz);
	if(terrainNormal.y <= 0.9f) {
		return;
	}

	// Building GrassBlade
	GrassBlade grassBlade;
	grassBlade.position = raisedPosition;
	grassBlade.facing = normalize(hashwithoutsine22(dispatchThreadID.xy) * 2.0f - 1.0f);
	grassBlade.windStrength = 0.0f;
	grassBlade.hash = rand(dispatchThreadID.xyx);
	grassBlade.type = 0u;

	float baseHeight = 1.0f;
	float heightRandom = 0.5f;
	float baseWidth = 0.02f;
	float widthRandom = 0.02f;
	float baseTilt = 0.8f;
	float tiltRandom = 0.5f;
	float baseBend = 0.1f;
	float bendRandom = 0.05f;

	grassBlade.height = baseHeight + remap01_neg11(rand(dispatchThreadID.xxy)) * heightRandom;
    grassBlade.width = baseWidth + remap01_neg11(rand(dispatchThreadID.yxx)) * widthRandom;
    //0-1 value, controlling the vertical component of the p3 point in the bezier curve, horizontal can be derived from pythag.
    grassBlade.tilt = baseTilt + remap01_neg11(rand(dispatchThreadID.xyx * float3(1.12f, 3.3f, 17.6f)) ) * tiltRandom;
    grassBlade.bend = baseBend + remap01_neg11(rand(dispatchThreadID.xyy * float3(12.32f, 0.23f, 3.39f)) ) * bendRandom;

	float3 posToCam =  normalize(FrameDataCB.CurrentRenderCamera.Position.xyz - raisedPosition);
	float viewAlignment = abs(dot(grassBlade.facing, normalize(posToCam.xz)));
    float sideCurve = smoothstep(0.3f, 0.0f, viewAlignment) * 1.5f;

    grassBlade.sideCurve = sideCurve;

	grassBladeBuffer.Append(grassBlade);
}

#endif