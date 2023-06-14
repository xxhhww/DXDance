#ifndef _TerrainBuilder__
#define _TerrainBuilder__

//һ��Node���8x8��Patch
#define PATCH_COUNT_PER_NODE_PER_AXIS 8

struct NodeDescriptor {
	uint isBranch;
	float pad1;
	float pad2;
	float pad3;
};

struct LODDescriptor {
	uint nodeSize;         // ��LOD��ÿһ��Node�ı߳�(��)(Node��������)
	uint nodeStartOffset;  // ��LOD�еĵ�һ��Node�Ŀ�ʼƫ����
	uint nodeCount;
	float pad2;
};

struct RenderPatch {
	float2 position;
	uint lod;
	float pad1;
};

struct PassData {
	float4 nodeEvaluationC;		// �û����ƵĽڵ�����ϵ��
	float2 worldSize;			// ������XZ�᷽��Ĵ�С(��)
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

// ����Node���������Node��ȫ��ID
uint GetGlobalNodeId(uint2 nodeLoc, uint lod) {
	StructuredBuffer<LODDescriptor> lodDescriptorList = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];
	LODDescriptor currLODDescriptor = lodDescriptorList[lod];
	
	uint nodeCountPerRow = PassDataCB.worldSize.x / currLODDescriptor.nodeSize;

    return currLODDescriptor.nodeStartOffset + nodeLoc.y * nodeCountPerRow + nodeLoc.x;
}

// ���Node����������ռ��µ������XZ����
float2 GetNodeWSPositionXZ(uint2 nodeLoc, uint lod) {
	StructuredBuffer<LODDescriptor> lodDescriptorList = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];
	LODDescriptor currLODDescriptor = lodDescriptorList[lod];

	float nodeMeterSize   = currLODDescriptor.nodeSize;	// ��λ�� /��
	float nodeCountPerRow   = PassDataCB.worldSize.x / currLODDescriptor.nodeSize;	// ��λ�� /��
	float nodeCountPerCol   = PassDataCB.worldSize.y / currLODDescriptor.nodeSize;	// ��λ�� /��
    float2 nodeWSPositionXZ = ((float2)nodeLoc - float2((nodeCountPerRow - 1.0f) * 0.5f, (nodeCountPerCol - 1.0f) * 0.5f)) * nodeMeterSize;
    return nodeWSPositionXZ;
}

// ���Node����������ռ��µ������XYZ����
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

	// ��Node���������������Ƿ��Node�����ٴλ���
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
							
	// xzƽ���µĶ�ά����
	uint2 nodeLoc = currLODNodeList.Consume();
	// ��ȡ��ǰ�����Node��ȫ������
	uint nodeGlobalID = GetGlobalNodeId(nodeLoc, PassDataCB.currPassLOD);

	// ����GID��ȡNodeDescriptor
	NodeDescriptor currNodeDescriptor = nodeDescriptorList[nodeGlobalID];

	bool needBranch = EvaluateNode(nodeLoc, PassDataCB.currPassLOD);
	if(PassDataCB.currPassLOD > 0 && needBranch) {
		// ��Node���л���
		nextLODNodeList.Append(nodeLoc * 2);
		nextLODNodeList.Append(nodeLoc * 2 + uint2(1, 0));
		nextLODNodeList.Append(nodeLoc * 2 + uint2(0, 1));
		nextLODNodeList.Append(nodeLoc * 2 + uint2(1, 1));
		currNodeDescriptor.isBranch = true;
	}
	else {
		// ����Node���л��֣���������ȷ����Ҫ��Ⱦ��Node
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
    //����Patch
    RenderPatch patch = CreatePatch(nodeLoc, patchOffset);

    culledPatchList.Append(patch);
}

#endif