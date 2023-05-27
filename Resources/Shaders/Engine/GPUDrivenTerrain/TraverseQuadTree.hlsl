#ifndef _TraverseQuadTree__
#define _TraverseQuadTree__

struct NodeDescriptor{
	uint isBranch;
	float pad1;
	float pad2;
	float pad3;
};

struct LODDescriptor{
	float nodeSize;         // 该LOD中每一个Node的边长(米)(Node是正方形)
	float nodeStartOffset;  // 该LOD中的第一个Node的开始偏移量
	float pad1;
	float pad2;
};

struct PassData{
	float3 nodeEvaluationC;		// 用户控制的节点评估系数	
	float2 worldSize;			// 世界在XZ轴方向的大小(米)
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

	float nodeSizeInMeter   = currLODDescriptor.nodeSize;	// 单位： /米
	float nodeCountPerRow   = PassDataCB.worldSize.x / currLODDescriptor.nodeSize;	// 单位： /个
	float nodeCountPerCol   = PassDataCB.worldSize.y / currLODDescriptor.nodeSize;	// 单位： /个
    float2 nodeWSPositionXZ = ((float2)nodeLoc - float2((nodeCountPerRow - 1) * 0.5f, (nodeCountPerCol - 1) * 0.5f)) * nodeSizeInMeter;
    return nodeWSPositionXZ;
}

// 获得Node中心在世界空间下的坐标的XYZ分量
float3 GetNodeWSPositionXYZ(uint2 nodeLoc, uint lod) {
	// TODO 等待Node的包围盒构建完毕
	return float3(0.0f, 0.0f, 0.0f);
}

bool EvaluateNode(uint2 nodeLoc, uint lod) {
	StructuredBuffer<LODDescriptor> lodDescriptorList = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];
	LODDescriptor currLODDescriptor = lodDescriptorList[lod];

	// 对Node进行评估，决定是否对Node进行再次划分
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

	// xz平面下的二维坐标
	uint2 nodeLoc = currLODNodeList.Consume();
	// 获取当前处理的Node的全局坐标
	uint nodeGlobalID = GetGlobalNodeId(nodeLoc, PassDataCB.currPassLOD);
	// 根据GID获取NodeDescriptor
	NodeDescriptor currNodeDescriptor = nodeDescriptorList[nodeGlobalID];

	if(PassDataCB.currPassLOD > 0 && EvaluateNode(nodeLoc, PassDataCB.currPassLOD)) {
		// 对Node进行划分
		nextLODNodeList.Append(nodeLoc * 2);
		nextLODNodeList.Append(nodeLoc * 2 + uint(1, 0));
		nextLODNodeList.Append(nodeLoc * 2 + uint(0, 1));
		nextLODNodeList.Append(nodeLoc * 2 + uint(1, 1));
		currNodeDescriptor.isBranch = true;
	}
	else {
		// 不对Node进行划分，则是最终确定需要渲染的Node
		finalNodeList.Append(uint3(nodeLoc, PassDataCB.currPassLOD));
		currNodeDescriptor.isBranch = false;
	}
	nodeDescriptorList[nodeGlobalID] = currNodeDescriptor;
}

#endif