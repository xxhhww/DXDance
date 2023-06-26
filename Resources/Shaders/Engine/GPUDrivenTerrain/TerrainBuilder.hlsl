#ifndef _TerrainBuilder__
#define _TerrainBuilder__

#include "TerrainHelper.hlsl"

struct PassData {
	float4 nodeEvaluationC;		// 用户控制的节点评估系数
	float2 worldSize;			// 世界在XZ轴方向的大小(米)
	uint heightScale;
	uint currPassLOD;
	uint currLODNodeListIndex;
	uint nextLODNodeListIndex;
	uint finalNodeListIndex;
	uint nodeDescriptorListIndex;
	uint lodDescriptorListIndex;
	uint culledPatchListIndex;
	uint minMaxHeightMapIndex;
	uint useFrustumCull;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

// 根据Node的索引获得Node的全局ID
uint GetGlobalNodeId(uint2 nodeLoc, uint lod) {
	StructuredBuffer<LODDescriptor> lodDescriptorList = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];
	LODDescriptor currLODDescriptor = lodDescriptorList[lod];
	
	uint nodeCountPerRow = PassDataCB.worldSize.x / currLODDescriptor.nodeSize;

    return currLODDescriptor.nodeStartOffset + nodeLoc.y * nodeCountPerRow + nodeLoc.x;
}

// 获得Node中心在世界空间下的坐标的XZ分量
float2 GetNodeWSPositionXZ(uint2 nodeLoc, uint lod) {
	StructuredBuffer<LODDescriptor> lodDescriptorList = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];
	LODDescriptor currLODDescriptor = lodDescriptorList[lod];

	float nodeMeterSize   = currLODDescriptor.nodeSize;	// 单位： /米
	float nodeCountPerRow   = PassDataCB.worldSize.x / currLODDescriptor.nodeSize;	// 单位： /个
	float nodeCountPerCol   = PassDataCB.worldSize.y / currLODDescriptor.nodeSize;	// 单位： /个
    float2 nodeWSPositionXZ = ((float2)nodeLoc - float2((nodeCountPerRow - 1.0f) * 0.5f, (nodeCountPerCol - 1.0f) * 0.5f)) * nodeMeterSize;
    return nodeWSPositionXZ;
}

// 获得Node中心在世界空间下的坐标的XYZ分量
float3 GetNodeWSPositionXYZ(uint2 nodeLoc, uint lod) {
	Texture2D<float4> minMaxHeightMap = ResourceDescriptorHeap[PassDataCB.minMaxHeightMapIndex];

	float2 wsPositionXZ = GetNodeWSPositionXZ(nodeLoc, lod);
	float2 minMaxHeight = minMaxHeightMap.mips[lod + 3][nodeLoc].xy;
    float y = (minMaxHeight.x + minMaxHeight.y) * 0.5f * PassDataCB.heightScale;
	return float3(wsPositionXZ.x, y, wsPositionXZ.y);
}

bool EvaluateNode(uint2 nodeLoc, uint lod) {
	StructuredBuffer<LODDescriptor> lodDescriptorList = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];
	LODDescriptor currLODDescriptor = lodDescriptorList[lod];

	// 对Node进行评估，决定是否对Node进行再次划分
    float3 wsPositionXYZ = GetNodeWSPositionXYZ(nodeLoc, lod);
    float dis = distance(FrameDataCB.CurrentRenderCamera.Position.xyz, wsPositionXYZ.xyz);
    float nodeMeterSize = currLODDescriptor.nodeSize;
    float f = dis / (nodeMeterSize * PassDataCB.nodeEvaluationC.x);
    if(f < 1){
        return true;
    }
    return false;
}

[numthreads(1, 1, 1)]
void TraverseQuadTree(uint3 DTid : SV_DispatchThreadID) {
	ConsumeStructuredBuffer<uint2>     currLODNodeList    = ResourceDescriptorHeap[PassDataCB.currLODNodeListIndex];
	AppendStructuredBuffer<uint2>      nextLODNodeList    = ResourceDescriptorHeap[PassDataCB.nextLODNodeListIndex];
	AppendStructuredBuffer<uint3>      finalNodeList      = ResourceDescriptorHeap[PassDataCB.finalNodeListIndex];
	RWStructuredBuffer<NodeDescriptor> nodeDescriptorList = ResourceDescriptorHeap[PassDataCB.nodeDescriptorListIndex];
	RWStructuredBuffer<LODDescriptor>  lodDescriptorList  = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];
							
	// xz平面下的二维坐标
	uint2 nodeLoc = currLODNodeList.Consume();
	// 获取当前处理的Node的全局坐标
	uint nodeGlobalID = GetGlobalNodeId(nodeLoc, PassDataCB.currPassLOD);

	// 根据GID获取NodeDescriptor
	NodeDescriptor currNodeDescriptor = nodeDescriptorList[nodeGlobalID];

	bool needBranch = EvaluateNode(nodeLoc, PassDataCB.currPassLOD);
	if(PassDataCB.currPassLOD > 0 && needBranch) {
		// 对Node进行划分
		nextLODNodeList.Append(nodeLoc * 2);
		nextLODNodeList.Append(nodeLoc * 2 + uint2(1, 0));
		nextLODNodeList.Append(nodeLoc * 2 + uint2(0, 1));
		nextLODNodeList.Append(nodeLoc * 2 + uint2(1, 1));
		currNodeDescriptor.isBranch = true;
	}
	else {
		// 不对Node进行划分，则是最终确定需要渲染的Node
		finalNodeList.Append(uint3(nodeLoc, PassDataCB.currPassLOD));
		currNodeDescriptor.isBranch = false;
	}
	nodeDescriptorList[nodeGlobalID] = currNodeDescriptor;
}

RenderPatch CreatePatch(uint3 nodeLoc, uint2 patchOffset) {
    uint lod = nodeLoc.z;

	StructuredBuffer<LODDescriptor> lodDescriptorList = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];
	Texture2D<float4> minMaxHeightMap = ResourceDescriptorHeap[PassDataCB.minMaxHeightMapIndex];
	LODDescriptor currLODDescriptor = lodDescriptorList[lod];
	
    float nodeMeterSize = currLODDescriptor.nodeSize;
    float patchMeterSize = nodeMeterSize / PATCH_COUNT_PER_NODE_PER_AXIS;
    float2 nodeWSPositionXZ = GetNodeWSPositionXZ(nodeLoc.xy,lod);

    uint2 patchLoc = nodeLoc.xy * PATCH_COUNT_PER_NODE_PER_AXIS + patchOffset;

    RenderPatch patch;
    patch.lod = lod;
    patch.position = nodeWSPositionXZ + ((float2)patchOffset - (PATCH_COUNT_PER_NODE_PER_AXIS - 1.0f) * 0.5f) * patchMeterSize;
    patch.minMaxHeight = minMaxHeightMap.mips[lod][patchLoc].rg * PassDataCB.heightScale + float2(-5.0f, 5.0f);
	patch.pad1 = 0.0f;
	patch.pad2 = 0.0f;
	patch.pad3 = 0.0f;
	return patch;
}

/*
* 计算Patch的包围盒
*/
BoundingBox GetPatchBoundingBox(RenderPatch patch) {
	uint lod = patch.lod;

	StructuredBuffer<LODDescriptor> lodDescriptorList = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];
	LODDescriptor currLODDescriptor = lodDescriptorList[lod];

	float nodeMeterSize = currLODDescriptor.nodeSize;
    float patchExtentSize = nodeMeterSize / (PATCH_COUNT_PER_NODE_PER_AXIS * 2.0f);

    BoundingBox boundingBox;
    float4 minPosition = float4(0.0f, 0.0f, 0.0f, 0.0f); 
	float4 maxPosition = float4(0.0f, 0.0f, 0.0f, 0.0f);

    minPosition.xz = patch.position - patchExtentSize;
    maxPosition.xz = patch.position + patchExtentSize;
    minPosition.y = patch.minMaxHeight.x;
    maxPosition.y = patch.minMaxHeight.y;

    boundingBox.minPosition = minPosition;
    boundingBox.maxPosition = maxPosition;
    return boundingBox;
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
void BuildPatches(uint3 id : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
	StructuredBuffer<uint3>             finalNodeList   = ResourceDescriptorHeap[PassDataCB.finalNodeListIndex];
	AppendStructuredBuffer<RenderPatch> culledPatchList = ResourceDescriptorHeap[PassDataCB.culledPatchListIndex];

	uint3 nodeLoc = finalNodeList[groupId.x];
    uint2 patchOffset = groupThreadId.xy;
    // 生成Patch
    RenderPatch patch = CreatePatch(nodeLoc, patchOffset);

	// 计算Patch的包围盒
	BoundingBox boundingBox = GetPatchBoundingBox(patch);

	// 裁剪Patch
	if(Cull(boundingBox)) {
		return;
	}

    culledPatchList.Append(patch);
}

#endif