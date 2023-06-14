#ifndef _TerrainBuilder__
#define _TerrainBuilder__

//一个Node拆成8x8个Patch
#define PATCH_COUNT_PER_NODE_PER_AXIS 8

struct NodeDescriptor {
	uint isBranch;
	float pad1;
	float pad2;
	float pad3;
};

struct LODDescriptor {
	uint nodeSize;         // 该LOD中每一个Node的边长(米)(Node是正方形)
	uint nodeStartOffset;  // 该LOD中的第一个Node的开始偏移量
	uint nodeCount;
	float pad2;
};

struct RenderPatch {
	float2 position;
	uint lod;
	float pad1;
};

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
	uint minmaxHeightMapIndex;
	float pad1;
};

#define PassDataType PassData

#include "../MainEntryPoint.hlsl"

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
	Texture2D<float> minmaxHeightMap = ResourceDescriptorHeap[PassDataCB.minmaxHeightMapIndex];

	float2 wsPositionXZ = GetNodeWSPositionXZ(nodeLoc, lod);
	float2 minMaxHeight = MinMaxHeightTexture.mips[lod + 3][nodeLoc].xy;
    float y = (minMaxHeight.x + minMaxHeight.y) * 0.5 * PassDataCB.heightScale;
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
void TraverseQuadTree(uint3 DTid : SV_DispatchThreadID)
{
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

RenderPatch CreatePatch(uint3 nodeLoc, uint2 patchOffset){
    uint lod = nodeLoc.z;

	StructuredBuffer<LODDescriptor> lodDescriptorList = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];
	LODDescriptor currLODDescriptor = lodDescriptorList[lod];

    float nodeMeterSize = currLODDescriptor.nodeSize;
    float patchMeterSize = nodeMeterSize / PATCH_COUNT_PER_NODE_PER_AXIS;
    float2 nodeWSPositionXZ = GetNodeWSPositionXZ(nodeLoc.xy,lod);

    uint2 patchLoc = nodeLoc.xy * PATCH_COUNT_PER_NODE_PER_AXIS + patchOffset;

    RenderPatch patch;
    patch.lod = lod;
    patch.position = nodeWSPositionXZ + ((float2)patchOffset - (PATCH_COUNT_PER_NODE_PER_AXIS - 1.0f) * 0.5f) * patchMeterSize;
    patch.pad1 = 0.0f;
	return patch;
}

[numthreads(8, 8, 1)]
void BuildPatches(uint3 id : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID)
{
	StructuredBuffer<uint3>             finalNodeList   = ResourceDescriptorHeap[PassDataCB.finalNodeListIndex];
	AppendStructuredBuffer<RenderPatch> culledPatchList = ResourceDescriptorHeap[PassDataCB.culledPatchListIndex];

	uint3 nodeLoc = finalNodeList[groupId.x];
    uint2 patchOffset = groupThreadId.xy;
    //生成Patch
    RenderPatch patch = CreatePatch(nodeLoc, patchOffset);

    culledPatchList.Append(patch);
}

#endif