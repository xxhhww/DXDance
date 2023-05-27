#ifndef _TraverseQuadTree__
#define _TraverseQuadTree__

struct NodeDescriptor{
	uint isBranch;
	float pad1;
	float pad2;
	float pad3;
};

struct LODDescriptor{
	float nodeSize;         // ��LOD��ÿһ��Node�ı߳�(��)(Node��������)
	float nodeStartOffset;  // ��LOD�еĵ�һ��Node�Ŀ�ʼƫ����
	float pad1;
	float pad2;
};

struct PassData{
	float3 nodeEvaluationC;		// �û����ƵĽڵ�����ϵ��	
	float2 worldSize;			// ������XZ�᷽��Ĵ�С(��)
	uint currPassLOD;
	uint currLODNodeListIndex;
	uint nextLODNodeListIndex;
	uint finalNodeListIndex;
	uint nodeDescriptorListIndex;
	uint lodDescriptorListIndex;
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

	float nodeSizeInMeter   = currLODDescriptor.nodeSize;	// ��λ�� /��
	float nodeCountPerRow   = PassDataCB.worldSize.x / currLODDescriptor.nodeSize;	// ��λ�� /��
	float nodeCountPerCol   = PassDataCB.worldSize.y / currLODDescriptor.nodeSize;	// ��λ�� /��
    float2 nodeWSPositionXZ = ((float2)nodeLoc - float2((nodeCountPerRow - 1) * 0.5f, (nodeCountPerCol - 1) * 0.5f)) * nodeSizeInMeter;
    return nodeWSPositionXZ;
}

// ���Node����������ռ��µ������XYZ����
float3 GetNodeWSPositionXYZ(uint2 nodeLoc, uint lod) {
	// TODO �ȴ�Node�İ�Χ�й������
	return float3(0.0f, 0.0f, 0.0f);
}

bool EvaluateNode(uint2 nodeLoc, uint lod) {
	StructuredBuffer<LODDescriptor> lodDescriptorList = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];
	LODDescriptor currLODDescriptor = lodDescriptorList[lod];

	// ��Node���������������Ƿ��Node�����ٴλ���
    float3 wsPositionXZ = GetNodeWSPositionXZ(nodeLoc,lod);
    float dis = distance(FrameDataCB.CurrentRenderCamera.Position.xyz, wsPositionXZ);
    float nodeSizeInMeter = currLODDescriptor.nodeSize;
    float f = dis / (nodeSizeInMeter * PassDataCB.nodeEvaluationC.x);
    if( f < 1){
        return true;
    }
    return false;
}

[numthreads(1, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	ConsumeStructuredBuffer<uint2>     currLODNodeList    = ResourceDescriptorHeap[PassDataCB.currLODNodeListIndex];
	AppendStructuredBuffer<uint2>      nextLODNodeList    = ResourceDescriptorHeap[PassDataCB.nextLODNodeListIndex];
	AppendStructuredBuffer<uint3>      finalNodeList      = ResourceDescriptorHeap[PassDataCB.finalNodeListIndex];
	RWStructuredBuffer<NodeDescriptor> nodeDescriptorList = ResourceDescriptorHeap[PassDataCB.nodeDescriptorListIndex];
	StructuredBuffer<LODDescriptor>    lodDescriptor      = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];

	// xzƽ���µĶ�ά����
	uint2 nodeLoc = currLODNodeList.Consume();
	// ��ȡ��ǰ�����Node��ȫ������
	uint nodeGlobalID = GetGlobalNodeId(nodeLoc, PassDataCB.currPassLOD);
	// ����GID��ȡNodeDescriptor
	NodeDescriptor currNodeDescriptor = nodeDescriptorList[nodeGlobalID];

	if(PassDataCB.currPassLOD > 0 && EvaluateNode(nodeLoc, PassDataCB.currPassLOD)) {
		// ��Node���л���
		nextLODNodeList.Append(nodeLoc * 2);
		nextLODNodeList.Append(nodeLoc * 2 + uint(1, 0));
		nextLODNodeList.Append(nodeLoc * 2 + uint(0, 1));
		nextLODNodeList.Append(nodeLoc * 2 + uint(1, 1));
		currNodeDescriptor.isBranch = true;
	}
	else {
		// ����Node���л��֣���������ȷ����Ҫ��Ⱦ��Node
		finalNodeList.Append(uint3(nodeLoc, PassDataCB.currPassLOD));
		currNodeDescriptor.isBranch = false;
	}
	nodeDescriptorList[nodeGlobalID] = currNodeDescriptor;
}

#endif