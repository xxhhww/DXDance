#ifndef _TerrainQuadTreeBuilder__
#define _TerrainQuadTreeBuilder__

#include "TerrainHeader.hlsl"

struct PassData {
	float4 nodeEvaluationC;			// 用户控制的节点评估系数

	float2 terrainMeterSize;		// 世界在XZ轴方向的大小(米)
	float  terrainHeightScale;		// Y轴缩放大小
	uint   useFrustumCull;

	uint   currPassLOD;
	uint   currNodeListIndex;
	uint   nextNodeListIndex;
	uint   finalNodeListIndex;

	uint   nodeDescriptorListIndex;
	uint   lodDescriptorListIndex;
	uint   culledPatchListIndex;
	float  pad;

	uint   nearCulledPatchListIndex;
	uint   farCulledPatchListIndex;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Math/BoundingBox.hlsl"

// 根据Node的索引获得Node的全局ID
uint GetGlobalNodeId(uint2 nodeLoc, uint lod) {
	StructuredBuffer<TerrainLodDescriptor> lodDescriptorList = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];
	TerrainLodDescriptor currLodDescriptor = lodDescriptorList[lod];
	
	uint nodeCountPerRow = PassDataCB.terrainMeterSize.x / currLodDescriptor.nodeMeterSize;

    return currLodDescriptor.nodeStartOffset + nodeLoc.y * nodeCountPerRow + nodeLoc.x;
}

// 获得Node中心在世界空间下的坐标的XZ分量
float2 GetNodeWSPositionXZ(uint2 nodeLoc, uint lod) {
	StructuredBuffer<TerrainLodDescriptor> lodDescriptorList = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];
	TerrainLodDescriptor currLodDescriptor = lodDescriptorList[lod];

	float nodeMeterSize     = currLodDescriptor.nodeMeterSize;
	float nodeCountPerAxis  = PassDataCB.terrainMeterSize.x / nodeMeterSize;
	float2 nodeWSPositionXZ = float2(0.0f, 0.0f);
	nodeWSPositionXZ.x = ((float)nodeLoc.x - (float)((nodeCountPerAxis - 1.0f) * 0.5f)) * nodeMeterSize;
	nodeWSPositionXZ.y = ((float)((nodeCountPerAxis - 1.0f) * 0.5f) - (float)nodeLoc.y) * nodeMeterSize;
    return nodeWSPositionXZ;
}

// 获得Node中心在世界空间下的坐标的XYZ分量
float3 GetNodeWSPositionXYZ(uint2 nodeLoc, uint lod) {
	StructuredBuffer<TerrainNodeDescriptor> nodeDescriptorList = ResourceDescriptorHeap[PassDataCB.nodeDescriptorListIndex];
	// 获取节点全局ID
	uint globalID = GetGlobalNodeId(nodeLoc, lod);
	TerrainNodeDescriptor currNodeDescriptor = nodeDescriptorList[globalID];

	// 计算XZ轴上的世界坐标
	float2 wsPositionXZ = GetNodeWSPositionXZ(nodeLoc, lod);
    float  wsPositionY  = (currNodeDescriptor.minHeight + currNodeDescriptor.maxHeight) * 0.5f * PassDataCB.terrainHeightScale;
	return float3(wsPositionXZ.x, wsPositionY, wsPositionXZ.y);
}

bool EvaluateNode(uint2 nodeLoc, uint lod) {
	// 当前评估的节点Lod已是最小，不需要再进行分割判断
	if(lod == 0) { return false; }

	StructuredBuffer<TerrainNodeDescriptor> nodeDescriptorList = ResourceDescriptorHeap[PassDataCB.nodeDescriptorListIndex];
	StructuredBuffer<TerrainLodDescriptor>  lodDescriptorList  = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];

	TerrainLodDescriptor currLodDescriptor = lodDescriptorList[lod];

	// 首先判断该节点的子节点的资源是否被加载，未加载则直接返回
	uint2 childNodeLoc  = nodeLoc;
	uint  childNodeLod  = lod - 1;
	uint  childGlobalID = 0;

	// 左上子节点
	childNodeLoc  = nodeLoc * 2u + uint2(0, 0);
	childGlobalID = GetGlobalNodeId(childNodeLoc, childNodeLod); 
	if(nodeDescriptorList[childGlobalID].tilePosX == 255) { return false; }

	// 右上子节点
	childNodeLoc  = nodeLoc * 2u + uint2(1, 0);
	childGlobalID = GetGlobalNodeId(childNodeLoc, childNodeLod); 
	if(nodeDescriptorList[childGlobalID].tilePosX == 255) { return false; }	

	// 左下子节点
	childNodeLoc  = nodeLoc * 2u + uint2(0, 1);
	childGlobalID = GetGlobalNodeId(childNodeLoc, childNodeLod); 
	if(nodeDescriptorList[childGlobalID].tilePosX == 255) { return false; }

	// 右下子节点
	childNodeLoc  = nodeLoc * 2u + uint2(1, 1);
	childGlobalID = GetGlobalNodeId(childNodeLoc, childNodeLod); 
	if(nodeDescriptorList[childGlobalID].tilePosX == 255) { return false; }

	// 对当前节点的距离进行评估
    float3 wsPositionXYZ = GetNodeWSPositionXYZ(nodeLoc, lod);
	wsPositionXYZ.y = 0.0f;
	float3 wsCameraPosition = FrameDataCB.CurrentEditorCamera.Position.xyz;
	wsCameraPosition.y = 0.0f;
    float dis = distance(wsCameraPosition, wsPositionXYZ.xyz);
    float f = dis / (currLodDescriptor.nodeMeterSize * PassDataCB.nodeEvaluationC.x);
    if(f < 1) { return true; }
    return false;
}

[numthreads(1, 1, 1)]
void TraverseQuadTree(uint3 DTid : SV_DispatchThreadID) {
	ConsumeStructuredBuffer<uint2> currNodeList  = ResourceDescriptorHeap[PassDataCB.currNodeListIndex];
	AppendStructuredBuffer<uint2>  nextNodeList  = ResourceDescriptorHeap[PassDataCB.nextNodeListIndex];
	AppendStructuredBuffer<uint3>  finalNodeList = ResourceDescriptorHeap[PassDataCB.finalNodeListIndex];

	// xz平面下的二维坐标
	uint2 nodeLoc = currNodeList.Consume();
	// 判断当前节点是否可以四叉划分
	bool needBranch = EvaluateNode(nodeLoc, PassDataCB.currPassLOD);

	if(needBranch) {
		// 对节点进行四叉划分
		nextNodeList.Append(nodeLoc * 2);
		nextNodeList.Append(nodeLoc * 2 + uint2(1, 0));
		nextNodeList.Append(nodeLoc * 2 + uint2(0, 1));
		nextNodeList.Append(nodeLoc * 2 + uint2(1, 1));
	}
	else {
		// 不对节点进行划分，则是最终确定需要渲染的节点
		finalNodeList.Append(uint3(nodeLoc, PassDataCB.currPassLOD));
	}
}

RenderPatch CreatePatch(uint3 nodeLoc, uint2 patchOffset) {
	uint2 nodeLocXY = nodeLoc.xy;
    uint  nodeLod   = nodeLoc.z;

	StructuredBuffer<TerrainNodeDescriptor> nodeDescriptorList = ResourceDescriptorHeap[PassDataCB.nodeDescriptorListIndex];
	StructuredBuffer<TerrainLodDescriptor>  lodDescriptorList  = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];

	// 获取节点全局ID
	uint globalID = GetGlobalNodeId(nodeLocXY, nodeLod);
	
	TerrainNodeDescriptor currNodeDescriptor = nodeDescriptorList[globalID];
	TerrainLodDescriptor  currLodDescriptor  = lodDescriptorList[nodeLod];
	
	// 获取Patch长度
    float patchMeterSize = currLodDescriptor.nodeMeterSize / PATCH_COUNT_PER_NODE_PER_AXIS;
    
	// 获取节点中心点XZ轴的世界坐标
	float2 nodeWSPositionXZ = GetNodeWSPositionXZ(nodeLocXY, nodeLod);

	// 计算Patch中心点XZ轴的世界坐标
    RenderPatch patch;
    patch.nodeLoc = nodeLoc;
	patch.patchOffset = patchOffset;
	patch.position.x = nodeWSPositionXZ.x + ((float)patchOffset.x - (float)((PATCH_COUNT_PER_NODE_PER_AXIS - 1.0f) * 0.5f)) * patchMeterSize;
    patch.position.y = nodeWSPositionXZ.y - ((float)patchOffset.y - (float)((PATCH_COUNT_PER_NODE_PER_AXIS - 1.0f) * 0.5f)) * patchMeterSize;
	patch.minmaxHeight = float2(currNodeDescriptor.minHeight, currNodeDescriptor.maxHeight) * PassDataCB.terrainHeightScale + float2(-5.0f, 5.0f);
	patch.pad1 = 0.0f;
	patch.pad2 = 0.0f;
	patch.pad3 = 0.0f;
	return patch;
}

// 计算Patch的包围盒
BoundingBox GetPatchBoundingBox(RenderPatch patch) {
	StructuredBuffer<TerrainLodDescriptor> lodDescriptorList = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];
	TerrainLodDescriptor currLodDescriptor = lodDescriptorList[patch.nodeLoc.z];

    float patchExtentSize = currLodDescriptor.nodeMeterSize / (PATCH_COUNT_PER_NODE_PER_AXIS * 2.0f);

    BoundingBox boundingBox;
    float4 minPosition = float4(0.0f, 0.0f, 0.0f, 0.0f); 
	float4 maxPosition = float4(0.0f, 0.0f, 0.0f, 0.0f);

    minPosition.xz = patch.position - patchExtentSize;
    maxPosition.xz = patch.position + patchExtentSize;
    minPosition.y = patch.minmaxHeight.x;
    maxPosition.y = patch.minmaxHeight.y;

    boundingBox.minPosition = minPosition;
    boundingBox.maxPosition = maxPosition;
    return boundingBox;
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
void BuildPatches(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID) {
	StructuredBuffer<uint3>             finalNodeList   = ResourceDescriptorHeap[PassDataCB.finalNodeListIndex];
	AppendStructuredBuffer<RenderPatch> culledPatchList = ResourceDescriptorHeap[PassDataCB.culledPatchListIndex];

	uint3 nodeLoc = finalNodeList[groupID.x];
    uint2 patchOffset = groupThreadID.xy;
    // 生成Patch
    RenderPatch patch = CreatePatch(nodeLoc, patchOffset);

	// 计算Patch的包围盒
	BoundingBox boundingBox = GetPatchBoundingBox(patch);

	// 裁剪Patch
	if(Cull(boundingBox)) { return; }

    culledPatchList.Append(patch);
}

#endif