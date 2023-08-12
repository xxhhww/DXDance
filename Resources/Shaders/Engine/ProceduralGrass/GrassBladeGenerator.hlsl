#ifndef _GrassBladeGenerator__
#define _GrassBladeGenerator__

#include "GrassHelper.hlsl"
#include "../GPUDrivenTerrain/TerrainHelper.hlsl"

struct PassData {
	uint grassBladesBufferIndex0;	// Grass LOD0
	uint grassBladesBufferIndex1;	// Grass LOD1
	uint terrainHeightMapIndex;
	uint terrainNormalMapIndex;

	float2 worldMeterSize;
	uint   heightScale;
	uint   grassBladeSizePerAxisPerTile;

	uint   useFrustumCull;
	uint   useDistanceCull;
	float  distanceCullStartDist;
	float  distanceCullEndDist;

	float  distanceCullMinimumGrassAmount;
	float  jitterStrength;
	uint   nearbyNodeListIndex;
	uint   currentNodeIndexBufferIndex;

	uint   nodeDescriptorListIndex;
	uint   lodDescriptorListIndex;
	float  pad1;
	float  pad2;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

float2 GetHeightUV(float2 wsPositionXZ) {
	float2 heightUV = (wsPositionXZ + (PassDataCB.worldMeterSize * 0.5f) + 0.5f) / (PassDataCB.worldMeterSize + 1.0f);

	return heightUV;
}

float GetHeightFromHeightMap(float2 wsPositionXZ) {
	Texture2D<float4> heightMap = ResourceDescriptorHeap[PassDataCB.terrainHeightMapIndex];

	float2 heightUV = GetHeightUV(wsPositionXZ);

	float  height = heightMap.SampleLevel(SamplerLinearWrap, heightUV, 0u).r;

	return height;
}

float3 GetNormalFromNormalMap(float2 wsPositionXZ) {
	Texture2D<float4> normalMap = ResourceDescriptorHeap[PassDataCB.terrainNormalMapIndex];

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
	float height = GetHeightFromHeightMap(position.xz) * PassDataCB.heightScale;
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

[numthreads(1, 8, 8)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
	AppendStructuredBuffer<GrassBlade> grassBladeBuffer       = ResourceDescriptorHeap[PassDataCB.grassBladesBufferIndex0];
	StructuredBuffer<uint3>            nearbyNodeList         = ResourceDescriptorHeap[PassDataCB.nearbyNodeListIndex];
	RWStructuredBuffer<uint>           currentNodeIndexBuffer = ResourceDescriptorHeap[PassDataCB.currentNodeIndexBufferIndex];
	StructuredBuffer<NodeDescriptor>   nodeDescriptorList     = ResourceDescriptorHeap[PassDataCB.nodeDescriptorListIndex];
	StructuredBuffer<LODDescriptor>    lodDescriptorList      = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];

	/*
	// 每一次Dispatch时，起的线程个数
	uint threadSizePerDispatch = PassDataCB.grassBladeSizePerAxisPerTile * PassDataCB.grassBladeSizePerAxisPerTile;
	// 当前Dispatch正在处理的Node索引
	uint  currNodeIndex = floor(currentNodeIndexBuffer[0] / threadSizePerDispatch);
	InterlockedAdd(currentNodeIndexBuffer[0], 1u);
	*/

	uint3 nodeLoc = nearbyNodeList[groupId.x];	// x,y,lod
	LODDescriptor  currLODDescriptor = lodDescriptorList[nodeLoc.z];
	// 此线程的偏移量
	uint offsetX = groupId.y * 8 + groupThreadId.y; // groupThreadId.x;		// 从0开始计数
	uint offsetZ = groupId.z * 8 + groupThreadId.z;// groupThreadId.y;		// 从0开始计数
	// 每一个GrassBlade之间的距离
	float distanceBetweenGrassBlade = (float)currLODDescriptor.nodeSize / (float)PassDataCB.grassBladeSizePerAxisPerTile;
	// 计算GrassBlade的相对坐标(相对于当前Tile原点，即Tile左下角的点)
	float fakePositionX = distanceBetweenGrassBlade * (float)offsetX;
	float fakePositionZ = distanceBetweenGrassBlade * (float)offsetZ;
	// 计算当前Tile的原点位置
	float nodeMeterSize     = currLODDescriptor.nodeSize;	// 单位： /米
	float nodeCountPerRow   = PassDataCB.worldMeterSize.x / currLODDescriptor.nodeSize;	// 单位： /个
	float nodeCountPerCol   = PassDataCB.worldMeterSize.y / currLODDescriptor.nodeSize;	// 单位： /个
    float2 nodeCenterWSPositionXZ = ((float2)nodeLoc.xy - float2((nodeCountPerRow - 1.0f) * 0.5f, (nodeCountPerCol - 1.0f) * 0.5f)) * nodeMeterSize;
	float2 nodeOriginWSPositionXZ = nodeCenterWSPositionXZ - float2(nodeMeterSize, nodeMeterSize) * 0.5f;
	// 计算GrassBlade的世界坐标
	float truePositionX = nodeOriginWSPositionXZ.x + fakePositionX;
	float truePositionZ = nodeOriginWSPositionXZ.y + fakePositionZ;
	float3 truePosition = float3(truePositionX, 0.0f, truePositionZ);

	/*
	// Calculate XZPosition
	uint   offsetX = dispatchThreadID.x;	// 从0开始计数
	uint   offsetY = dispatchThreadID.y;	// 从0开始计数

	float  distanceBetweenGrassBlade = PassDataCB.worldMeterSize.x / (float)PassDataCB.grassBladeSizePerAxisPerTile;

	float  absPositionX = distanceBetweenGrassBlade * offsetX;
	float  absPositionZ = distanceBetweenGrassBlade * offsetY;
	
	float  truePositionX = absPositionX - (PassDataCB.worldMeterSize.x / 2.0f);
	float  truePositionZ = absPositionZ - (PassDataCB.worldMeterSize.x / 2.0f);

	float3 position = float3(truePositionX, 0.0f, truePositionZ);
	*/
	// Apply Jitter

	float2 hash = hashwithoutsine22(dispatchThreadID.yz);
	float2 jitter = ((hash * 2.0f) - 1.0f) * PassDataCB.jitterStrength;

	truePosition.xz += jitter;

	// Apply Height
	float3 raisedPosition = ApplyHeightOffset(truePosition);

	// DistanceCull
	// if(DistanceCull(raisedPosition, hash.x)) {
		// return;
	// }

	// FrustumCull
	BoundingBox boundingBox;
	boundingBox.minPosition = float4(raisedPosition, 0.0f) - float4(1.0f, 0.0f, 1.0f, 0.0f);
	boundingBox.maxPosition = float4(raisedPosition, 0.0f) + float4(1.0f, 2.0f, 1.0f, 0.0f);
	if(Cull(boundingBox)) {
		return;
	}

	// Normal Check
	float3 terrainNormal = GetNormalFromNormalMap(truePosition.xz);
	if(terrainNormal.y <= 0.9f) {
		return;
	}

	// Building GrassBlade
	GrassBlade grassBlade;
	grassBlade.position = raisedPosition;
	grassBlade.facing = normalize(hashwithoutsine22(dispatchThreadID.yz) * 2.0f - 1.0f);;
	grassBlade.windStrength = 0.5f;
	grassBlade.hash = rand(dispatchThreadID.yzy);
	grassBlade.type = 0u;

	float baseHeight = 1.0f;
	float heightRandom = 0.5f;
	float baseWidth = 0.02f;
	float widthRandom = 0.02f;
	float baseTilt = 0.9f;
	float tiltRandom = 0.01f;
	float baseBend = 0.1f;
	float bendRandom = 0.05f;

	grassBlade.height = baseHeight + remap01_neg11(rand(dispatchThreadID.yyz)) * heightRandom;
    grassBlade.width = baseWidth + remap01_neg11(rand(dispatchThreadID.zyy)) * widthRandom;
    //0-1 value, controlling the vertical component of the p3 point in the bezier curve, horizontal can be derived from pythag.
    grassBlade.tilt = baseTilt + remap01_neg11(rand(dispatchThreadID.yzy * float3(1.12f, 3.3f, 17.6f))) * tiltRandom;
    grassBlade.bend = baseBend + remap01_neg11(rand(dispatchThreadID.yzz * float3(12.32f, 0.23f, 3.39f)) ) * bendRandom;

	float3 posToCam =  normalize(FrameDataCB.CurrentRenderCamera.Position.xyz - raisedPosition);
	float viewAlignment = abs(dot(grassBlade.facing, normalize(posToCam.xz)));
    float sideCurve = smoothstep(0.3f, 0.0f, viewAlignment) * 1.5f;

    grassBlade.sideCurve = sideCurve;

	grassBladeBuffer.Append(grassBlade);
}

#endif